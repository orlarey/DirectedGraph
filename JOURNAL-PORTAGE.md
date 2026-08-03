# Journal de portage — chantier DirectedGraph

Chantier ouvert le 2026-08-02 (session faust loop-splitting). Trois
dossiers convergents, tous nés des campagnes de mesure des 1-2 août ;
le travail se fait ICI (dépôt standalone), puis se recopie dans les
projets porteurs (faust/compiler/DirectedGraph — `gitdag compiler`
vérifie la synchronisation).

## Les trois dossiers

### 1. mcschedule — le model scheduling (R, U) comme algorithme de la librairie

Le compilateur faust (branche loop-merging) possède un ordonnanceur
« model » : liste des prêts, U émissions par cycle, score à deux
régimes — hauteur critique tant que les valeurs vivantes tiennent
sous R registres, libération de registres au-delà. Mesuré : il bat
df/bf sur les corps sous pression (fdnRev 0.877 vs classique), et
c'est la stratégie du moteur de fusion. Tout ce qu'il consomme se
dérive du graphe (prêts, hauteur, vivacité) : rien n'est spécifique
aux signaux. Sa place est donc ici, comme `mcschedule<N>(G, R, U)`
aux côtés de df/bf/sp/rb — et le gain immédiat côté faust est
`-ss 6` : la stratégie model au grain des instructions classiques
(la campagne des ordres du 2026-08-02 mesure ~17 % laissés sur la
table par l'ordre d'émission classique, sans stratégie fixe
dominante).

### 2. Déterminisme — digraph<N, Compare>

Découvert le 2026-08-02 : l'émission classique de faust est
non-déterministe d'un run à l'autre sur les gros programmes
(6332 lignes réordonnées entre deux invocations identiques sur
fdnRev). Cause : `digraph<N>` ordonne ses nœuds et ses connexions
par `std::less<N>` — pour N = Tree (un pointeur), c'est l'ordre des
adresses, et les ex-æquo des ordonnancements se départagent à
l'ASLR. C'est la même classe de bug que la spécialisation
`std::less<CTree*>` corrigée dans la tlib le 2026-07-31 (comparateur
nommé `treeorder`), mais dans la librairie générique. Correctif
visé : `template <typename N, typename C = std::less<N>>` threadé
dans digraph ET dans les conteneurs locaux des algorithmes
(dfschedule::V, spschedule::V, parallelize, graph2dag…) ; les
projets porteurs instancient alors `digraph<Tree, treeorder>`.
Attention au cas `graph2dag` : les sous-graphes deviennent des clés
— leur propre operator< doit lui aussi devenir paramétrable pour que
le déterminisme soit de bout en bout.

### 3. Passage à l'échelle de spschedule

Mesuré le 2026-08-02 (campagne costlyexamples) : l'ordre `sp`
explose sur les gros graphes — virtualAnalog : 112 s et 2.7 Go de
RSS (contre 3.1 s / 0.9 Go en df) ; piano1 : 120 s sans produire de
code. `recschedule` (liste avec doublons, relue à l'envers) est le
suspect : la liste des doublons croît vraisemblablement de façon
superlinéaire. À profiler, puis borner ou réécrire.

## Entrées

### 2026-08-02 — ouverture, mcschedule v1

- Journal créé ; état des lieux ci-dessus.
- `mcschedule<N>(G, R, U)` implémenté dans Schedule.hh : liste des
  prêts par cycles de largeur U (latence 1 : une valeur émise dans un
  cycle n'est opérande qu'au cycle suivant), score à deux régimes
  (hauteur critique sous R vivants, libération de registres au-delà),
  ancrage des ex-æquo sur l'ordre dfschedule (stabilité relative ;
  le déterminisme absolu attend le dossier 2). Les graphes cycliques
  sont tolérés par un déblocage df (comme dfschedule les tolère) —
  le contrat plein n'est garanti que sur DAG.
- Test check29 : contrat de validité, plafonnement de la crête de
  vivacité vs bfschedule sur un banc de chaînes parallèles, totalité
  sur graphe cyclique. (Premier jet du banc réfuté par lui-même : une
  somme finale impose un plancher de 4 vivants à TOUT ordonnancement
  — le banc de pression doit être des chaînes indépendantes.)
- Recopié dans faust/compiler/DirectedGraph (gitdag : synced) et
  câblé comme `-ss 6` dans l'émission classique ocpp (R et U pris à
  `-ls-R` / `-ls-U`).
- Premières mesures, fdnRev classique (bencharch, min de 3 tours
  alternés, secteur) : mc au R physique (20) PERD — 12.96 ms contre
  df 12.38 et sp 11.66. Mais le grain n'est pas le même : un nœud de
  l'émission classique est une expression imbriquée entière, pas une
  opération — peu de valeurs « larges » vivantes suffisent. Au
  balayage : **mc R=4 : 11.45 ms — meilleur ordre mesuré** (sp 11.66,
  df 12.38, soit −7.5 % vs df) ; R=8 : 11.89 ; R=40 : 11.69. Leçon :
  le budget R se calibre PAR GRAIN (4 au grain instruction classique,
  20 au grain op du moteur -ls). Coût de compilation : ~3 s sur
  fdnRev (le O(V·cycles) de v1), à surveiller sur les costly.

### 2026-08-02 (après-midi) — l'hypothèse (R, U), réfutée avec profit

Hypothèse (Yann) : pour tout programme scalaire il existe (R, U) tel
que mc bat ou égale la meilleure stratégie fixe. Campagne : corpus
des 22, baseline fraîche des 5 fixes + grille 10 R × 3 U, 3 tours
alternés (`campaign-mcgrid-20260802` dans loop-splitting/measures).

**Réfutée : 5/22** (geomean meilleur-mc/meilleure-fixe 1.580) — mais
le motif d'échec identifie la dimension manquante. mc gagne toute la
classe sous pression (bowedString 0.796, karplus32 0.850, dbmeter
0.866, fdnRev 0.986) ; il échoue ailleurs avec 9/17 optima collés au
bord R=40 : son extrême-largeur ne retrouve pas bf, car le modèle
(R, U) ne voit pas la **localité** (bf groupe les niveaux, df les
expressions ; la proximité des opérandes est invisible à la vivacité
— c'est le `schedulingcost` de la librairie). Pire cas : vocoder
×6.6, le plus grand gagnant d'ordre du corpus (bf ×0.28), dispersé
par mc.

Hypothèse v2 au tableau : (a) départage des ex-æquo par RÉCENCE des
opérandes (remplace l'ancrage df, gratuit) ; (b) hybride
mc-dans-les-niveaux (l'ordre bf, la police (R, U) dedans). Aussi :
l'enveloppe fixes+mc vaut 0.803 vs df — la sélection par programme
reste le gisement (~20 %).

### 2026-08-02 (soir) — la localité décomposée, v2a réfutée

L'anecdote (rapportée par Yann) : historiquement l'ordre de
DÉCLARATION des champs de la classe DSP suivait le deep-first, et un
reclassement des champs avait fortement dégradé les performances.
Reproduite et quantifiée sur vocoder : trier les 205 déclarations
coûte **+69 % à bf** (1.136 → 1.925 ms) à ordre d'instructions
identique — la disposition mémoire des états est un effet de premier
ordre, et elle SUIT l'ordre d'émission (réparer l'ordre répare les
deux). df trié : +1.7 % seulement ; mc trié : neutre (son layout est
déjà dispersé, et son ordre d'instructions reste 4x derrière un bf
saboté — mc a les deux problèmes sur vocoder).

v2a (récence des opérandes en DÉPARTAGE des deux régimes) : essayée,
**réfutée par A/B même session** — elle régresse le meilleur gagnant
de la classe pression (bowedString R8U1 : 5.82 → 6.47, +11 %) pour
des gains marginaux sur les échecs (vocoder −5 %, insects −3 %). Le
départage perturbe les ordres du régime-libération qui font gagner
mc. Revert : Schedule.hh reste en v1.

Leçon pour v2b : la localité ne peut pas être un critère SECONDAIRE
du même parcours — c'est une STRUCTURE (les niveaux de bf, les
groupes d'expressions de df) dans laquelle la police (R, U) doit
travailler. Candidat suivant : mc-dans-les-niveaux (l'ordre des
niveaux de bf conservé, la police (R, U) appliquée à l'intérieur de
chaque niveau), qui hérite du layout-balayage de bf par
construction.

### 2026-08-02 (nuit) — csschedule v1 : le fold trahit la récursion

Formulation de Yann : le scheduling comme REMPLISSAGE d'une grille
U × cycles sous contrainte R (qualité = cases vides), défini
récursivement sur l'expression — scheduler les arguments, COMBINER
leurs schedulings sous R en minimisant les trous, ajouter le
combinateur ; le cœur est l'opérateur de combinaison de deux
schedulings. Trois amendements actés : la récursion se fait sur le
DAG-à-lets (le partage hoisté), les queues de vivacité bornent
l'idéal (l'arité plancher), et le combine par paire est calculable
exactement (DP sur la grille |A|×|B|, ordres internes préservés,
dépendances croisées, sur-pression minimisée, alternance préférée).

Implémenté : `csschedule<N>(G, R, U)` v1 = régions df (localité par
blocs) + FOLD LINÉAIRE des lets en ordre topo, chaque région fusionnée
dans l'accumulateur par le DP. check30 OK. Câblé `-ss 7` côté faust
(via graph2dag pour les cycles).

Écran (6 programmes, même session) : **cs v1 ≡ mc** — égale les
gagnants de la classe pression (bowedString 5.95 vs 7.5 fixe, dbmeter
1.075, karplus 1.14) et échoue À L'IDENTIQUE sur la classe légère
(vocoder 7.67 ≈ mc 7.64 ; insects 13.7 ; fire 4.24). Diagnostic : le
fold linéaire APLATIT la récursion — sur les programmes à fort
partage, les régions sont des singletons et le fold dégénère en
entrelacement global glouton, c'est-à-dire mc. La hiérarchie est le
porteur de la localité : la combinaison doit suivre l'ARBRE DE
DOMINANCE du DAG (chaque let combiné chez son dominateur immédiat,
les blocs frères combinés localement puis remontés comme unités) —
csschedule v2.

### 2026-08-02 (tard) — v2 dominance, v3 round-robin, et le vrai nom de la localité : SLP

csschedule v2 (association par l'arbre de dominance, terme de stalle,
départage concaténation) : les gagnants pression s'améliorent encore
(dbmeter 1.055, bowedString 5.82 — records), les échecs INCHANGÉS au
centième. v3 (round-robin régulier par lots ≤ R pour les enfants
indépendants — la théorie du balayage préchargeable, convergente avec
les paquets m99 du moteur -ls) : idem. Trois architectures, le même
échec exact → la variable n'était pas l'algorithme.

**Ablation décisive** : vocoder en bf passe de 1.155 à 4.08-4.15 ms
sous `-fno-slp-vectorize` (et -fno-vectorize n'ajoute rien). Les ¾ de
l'avantage de bf sont de la **vectorisation superword** : bf juxtapose
l'étape k de toutes les bandes — des instructions isomorphes
indépendantes adjacentes, que le SLP packe en NEON. Tous nos ordres
par blocs (df-régions, dominance, rr de groupes) détruisent cette
adjacence : via graph2dag, le round-robin tournait sur des groupes-SCC
entiers, chaque bande restant un bloc df interne — le bon geste au
mauvais grain. La « localité » manquante = l'adjacence isomorphe,
c'est le mouvement Bank redécouvert (3e fois).

v4 spécifiée : entrelacement AU GRAIN INSTRUCTION des groupes frères
indépendants (l'expansion des lots ne concatène plus les df internes,
elle les entrelace position par position) ; et le score du combine
peut récompenser la juxtaposition d'opérations isomorphes. Résiduel
après SLP (bf-noslp 4.15 vs nos 7.7) : à instruire ensuite.

### 2026-08-02 — ouverture de la branche combine-lab

Le banc d'étalonnage du combine (méthode de Yann : optima CONSTRUITS,
découpés en sous-expressions, le combine doit y retomber — invariance
à la découpe, qualité Q = (trous, sur-pression, adjacence isomorphe))
se développe sur la branche `combine-lab`. main reste la ligne stable
multi-machines ; le merge se fait quand une version du combine passe
les familles certifiées ET l'écran faust. Pendant la vie de la
branche, les copies (faust/tlib/signals) suivent combine-lab.

### 2026-08-02 — le banc d'étalonnage converge en une itération

`squality` (cycles/trous par empaquetage latence-1, sur-pression,
adjacence isomorphe par foncteur de forme), `dpcombine` extrait en
fonction publique avec le nouveau terme ISOMISS (2, contre STALL 3,
pression PBIG), familles+découpes dans check31. Résultat immédiat sur
bank(4,6) : sans le terme iso, TOUTES les découpes échouent (chaînes :
11 cycles, 20 trous ; niveaux : idem) ; AVEC le terme iso, les trois
découpes (chaînes, moitiés, niveaux) retombent EXACTEMENT sur
l'optimum certifié (6 cycles, 0 trou, isoadj 18). L'invariance à la
découpe est acquise sur la famille banc. La méthode
optimum-construit→découpes→reconstruction a désigné et calé le terme
en une itération. Suivant : brancher la forme des nœuds côté faust
(symbole de l'op du Tree) dans csschedule, et re-passer vocoder.

Illustrations (données réelles de dpcombine, `labviz.cpp` +
`figs/labviz.py`) : [figs/combine-process.png](figs/combine-process.png)
— les 4 pièces, l'optimum certifié, le pliage sans terme iso (11
cycles, 20 trous, où l'on VOIT l'histoire du pliage par paires) et le
pliage avec terme iso (l'optimum retrouvé) ; et
[figs/combine-dp-path.png](figs/combine-dp-path.png) — le combine
comme chemin dans la grille |A|×|B|, l'escalier du tressage contre le
L de la concaténation.

### 2026-08-03 — les compteurs d'usages fondent l'algèbre ; les lois tiennent

Point de Yann : la pression exige un mapping global t -> nombre
d'usages (les consommateurs distincts dans le graphe entier) — sans
lui, la vivacité repose sur des invariants d'ordre de pliage et
l'algèbre est infondée. Refonte de dpcombine : vivacité EXACTE par
compteurs d'usages (une valeur meurt à son vrai dernier consommateur
GLOBAL ; les morts approximées de la v1 disparaissent), contraintes
BILATÉRALES (minI et minJ : chaque côté attend ses opérandes de
l'autre — la commutativité partielle du monoïde de traces restaurée),
csschedule rebranché sur l'opérateur unique (une seule source).
Nuance documentée : usage = consommateurs DISTINCTS (degré entrant),
pas occurrences textuelles — x*x n'use x qu'une fois au sens de la
vivacité ; côté faust c'est OccMarkup/getSharingCount.

check32, les LOIS : bank(4,6) — 4 permutations des pièces + pliage en
arbre équilibré -> Q(optimum) partout (commutativité et associativité
EN QUALITÉ). Famille à partage diamant(4,5) (source S, usage 4,
lue par 4 chaînes iso) : optimum certifié (6 cycles, 3 trous
incompressibles = U-1, peak 4, isoadj 15) atteint par toutes les
permutations, y compris S plié EN DERNIER — les contraintes
bilatérales le replacent en tête, les compteurs le font mourir à sa
4e consommation. Le tableau check31 est inchangé (la refonte préserve
les acquis).

Cible atteinte sur ces familles : (schedules mod Q, +) se comporte en
monoïde partiellement commutatif. Suivant : familles à partage plus
riches (coefficients communs, m25), vivacité de queue multi-niveaux
dans csschedule, et le branchement de la forme des Trees côté faust.

### 2026-08-03 — L'ARTEFACT : tout l'écran mc/cs mesurait l'émission -ls

Découvert en inspectant un .cpp généré : les options-paramètres
faust `-ls-R`/`-ls-U` (et toutes les -ls-*) activaient silencieusement
gLoopSplit — CHAQUE mesure `-ss 6/7 -ls-R X` depuis l'ouverture du
chantier benchmarkait l'émission loop-split, pas l'ordre classique.
Le « mur des 7.6 » de vocoder identique à travers quatre architectures
= le -ls tout court (7.77 la nuit d'avant) ; la « victoire pression »
de bowedString (5.87) = -ls aussi ; la réfutation 5/22 de l'hypothèse
(R,U) = RETIRÉE, jamais testée. Survivent : les 5 ordres fixes,
l'ablation SLP de bf (bf/vocoder passe de 1.16 à 4.1 sans superword),
et tout le travail de la librairie (banc, lois, compteurs — mesuré sur
optima construits, pas sur faust).

Correctif : les paramètres ne commutent plus les modes (seuls -ls et
-ls-fuse activent). PREMIÈRES VRAIES MESURES des ordres (même session,
6 programmes) : cs GAGNE karplus32 (1.28 vs df 1.32), mc ÉGALE dbmeter
(1.23 vs bf 1.24), vocoder cs 2.66 — le terme iso au grain instruction
paie (−35 % sous df), bf (1.16) garde l'avance SLP ; fire cs 1.60 (sp
1.44), insects cs 6.83 (bf 5.78), bowedString ~8.3 (sp 7.56).
Hypothèse (R,U) ROUVERTE — les vrais ordres sont compétitifs, ni
catastrophe ni miracle. Re-campagne grille à refaire proprement.

Leçon d'instrument (la troisième du chantier, après fast-math et le
grain des groupes) : une option qui commute un mode en réglant un
paramètre est un piège de mesure — et « attribuer exige d'ablater »
vaut aussi pour ses propres options.

### 2026-08-03 (soir) — mcgrid2 : l'hypothèse jugée sur pièces, 14/22

Campagne propre (garde anti-contamination : zéro rejet). **14/22
confirmés** à ≤1.02, 11 victoires nettes — clarinet cs R1U4 : −8.7 %
sous le meilleur fixe ; violin, paradigma, karplus32, filterBank,
churchOrgan, freeverb (le réfractaire, battu par mc R40), dbmeter.
cs bat mc 13 duels à 9 : le compositionnel (dominance + iso au grain
instruction) ajoute au spectre pur. Geomean meilleur-grille /
meilleure-fixe : 1.053. Les 8 échecs = la classe bf/rb (SLP) :
vocoder 2.64 (l'iso-terme fait la moitié du chemin depuis df 4.08,
bf 1.35 garde l'avance de régularité totale), bells 1.19, brass 1.16,
insects 1.14. R optima étalés de 1 à 40 : la sélection par programme
reste le gisement (enveloppe totale 0.820 vs df).

Suites : durcir les familles bank du banc (le résidu SLP du combine),
pondération de l'iso-terme, et la sélection (stratégie, R, U) par
programme jugée compute_stack.

### 2026-08-03 (nuit) — v5 : csschedule direct au grain Tree ; clarinet ×4.8

L'indicateur de remplissage de Yann (cases occupées / cycles×U, machine
d'évaluation commune) a tout déclenché : le résidu SLP est un déficit
de remplissage (nos vainqueurs 40-46 % vs bf 88-99 %), et la pression
du modèle compte moins que modélisé sur le classique (bf gagne à crête
299-779 : le renommage OoO absorbe).

Banc durci (check33) : blocs(4,4,4) — reconstruction = optimum, les
étapes-blocs innocentées ; large(16,6,R4) — le pliage par paires perd
15 % de remplissage, mais csschedule DIRECT (son round-robin par lots
= le tuilage) atteint l'optimum exact. Conclusion : la machinerie de
la librairie était complète, le déficit était dans le câblage faust
(association aux groupes seulement, repli par paires au grain
instruction).

v5 : `-ss 7` = csschedule directement sur le graphe de Trees (les
arêtes arrière ignorées par ses structures comme df les ignore).
Résultats, bit-exacts vérifiés strict : **clarinet 0.374 ms — ×4.8
sous le meilleur ordre fixe** (sp 1.782) et ×4.3 sous l'ancien cs ;
karplus32 1.260 ; filterBank 5.102 ; vocoder : remplissage 43→73 %,
isoadj 493→930, temps 2.62 (bf 1.14 : la monnaie réelle n'est pas le
TAUX d'adjacence mais la LONGUEUR des séries isomorphes — le SLP
packe par 4) ; churchOrgan −1 % ; génération 4.5× plus rapide (0.97 s
sur vocoder). Prochaine marche : allonger les séries (lots par forme,
pas seulement par indépendance) ; et re-campagne grille complète sous
v5.

### 2026-08-04 — les formes : le calcul sans les données

Proposition de Yann : la forme d'une expression = l'expression où l'on
« oublie » les données pour ne garder que le calcul. Implémenté côté
faust (ocppShape) : la forme est ELLE-MÊME un arbre hash-consé —
feuilles de données -> trous typés, enfants ordonnancés -> trous de
référence (la frontière de troncature), offsets de délais constants
oubliés par la règle générique, cycles rec coupés par garde. Deux
instructions sont isomorphes ssi même pointeur de forme. Branché dans
-ss 7 et l'indicateur qualité (isoadj honnête : vocoder 627).

Verdicts : clarinet ×4.8 préservé (0.374 bit-exact) ; vocoder inchangé
— confirmation que le verrou est la LONGUEUR des runs. Le tri des
frères par forme (lots homogènes) : isoadj inchangé, les bandes ne
sont PAS frères dans l'arbre de dominance — reverté. Conclusion de
conception : allonger les runs = regroupement GLOBAL par forme à
travers le graphe = le mouvement BANK proprement dit, dont shape()
est désormais la primitive de reconnaissance. C'est le prochain
chantier de fond, avec la re-campagne v6 sur le corpus.

### 2026-08-04 — statistiques de formes : le gisement Bank est quantifié

FAUST_SS_SHAPES (avec un correctif de grain décisif : les opcodes de
binop restent LITTÉRAUX dans la forme — un sélecteur d'opération n'est
pas une donnée, mul et add ne partagent pas de forme). Sur le corpus :
**84-95 % des nœuds ordonnancés vivent dans des formes de multiplicité
≥ 4** (le seuil de packing SLP) presque partout — echo 48 % (trop
petit), jprev dégénéré (2048 de ses 2049 nœuds sont le contenu entier
d'une waveform explosée dans le graphe : à regarder en soi). vocoder :
469 mul, 261 lectures de délai, 225 add, 164 div — des bancs de
centaines d'instances. Nuance : c'est une borne SUPÉRIEURE (les
instances d'une forme peuvent dépendre entre elles) ; le raffinement
suivant = tailles de bancs ajustées par indépendance (antichaînes par
classe de forme). Le mouvement Bank a sa matière première mesurée.

### 2026-08-03 — alignschedule v1 : l'alignement des formes (Yann)

Formalisation de l'idée de Yann (les formes comme couleurs, regroupées
en rangs monochromes en respectant l'ordre topologique) : intervalles
de mobilité [ASAP, ALAP], classes par fréquence décroissante, stabbing
d'intervalles par classe (polynomial), rangs-cibles comme PRIORITÉS
d'un Kahn (validité par construction). Cadeau structurel : un rang est
une antichaîne — l'indépendance des bancs est gratuite.

Banc : large(16,6,R4) -> fill 100 %, isoadj 90, AU-DELÀ de l'optimum
tuilé (72) — rangs monochromes pleine largeur, crête 16 que la machine
OoO réelle ne facture pas. Graphe réel : vocoder v1 ÉCHOUE (fill 49 %,
crête 259, 5.49 vs df 4.09 intra-run) — l'accaparement couleur-d'abord
cale sur le tissu conjonctif et les dépendances partagées ; clarinet
0.63 (×2.8 sous les fixes, cs 0.374 reste maître). Méthode : famille
banc+tissu au banc d'étalonnage pour itérer (placement ASAP-side vs
ALAP-side du stabbing, interaction classes-tissu). NOTE dérive : bf
vocoder 1.14 le matin, 1.87 le soir — thermique ; ratios intra-run
seulement.

### À suivre

- Dossier 2 (déterminisme) : digraph<N, Compare> de bout en bout.
- Dossier 3 : profiler spschedule/recschedule sur virtualAnalog.
- mcschedule : banc de calibration de R au grain classique sur le
  corpus (la campagne des ordres a l'infrastructure) ; envisager un
  paramètre de poids par nœud (les expressions n'ont pas toutes la
  même largeur).

## 2026-08-03 — campagne finale, et le tirage rare qui divergeait

La campagne définitive (22 programmes × 26 variantes, même session) :
enveloppe {align, grille cs} contre meilleur fixe, geomean 1.016,
15/22 dans la marge 1.02, 7 gains nets. Couronnes : align 7, cs 15.
La chute du mur vocoder (align = bf, 1.003) est confirmée en campagne
propre. Partition structurelle du corpus : l'alignement possède les
bancs (SLP), le compositionnel possède les récurrences (karplus32 :
align ×3.9 mais cs 0.978).

Le fait du jour est ailleurs. La campagne a mesuré clarinet align à
1.730 ms là où le record était 0.372. Sonde : 20 générations
fraîches → 6 md5 distincts, TOUTES à 0.370–0.378 et bit-exactes. Le
tirage de la campagne est rare — et il DIVERGE : 44097/44101 lignes,
des zéros dès l'échantillon 3 (la boucle d'anche ne sonne pas), 510
déclarations contre 117. Un tirage d'adresses peut donc produire un
ordre INVALIDE : la rupture d'interblocage des cycles récursifs
(Kahn en panne sèche → nœud pris dans l'ordre de l'ancre) ne casse
pas toujours le cycle là où l'émission classique l'attend, et la
sémantique du sample précédent bascule. Dix-neuf fois sur vingt
l'ancre df mène au même point de rupture ; la vingtième, non.

Conséquence doctrinale : le dossier déterminisme (digraph<N,Compare>
+ ordre structurel) cesse d'être de l'hygiène de mesure — c'est un
dossier de correction. Et la contrainte est plus forte que « un ordre
stable » : il faut que la rupture de cycles d'alignschedule soit
ALIGNÉE sur celle de l'émission (mêmes arêtes arrière), pas seulement
déterministe. Piste : hériter les points de rupture de l'ancre df au
lieu de les redécouvrir en panne sèche.

## 2026-08-03 (suite) — rectificatif : il n'y avait pas de bug, il y avait deux find

L'entrée précédente annonçait un « tirage rare divergent »
d'alignschedule. Yann a objecté qu'un cycle à zéro est impossible par
construction dans Faust (retard ≥ 1 dans toute boucle récursive) —
et l'objection a tout démonté, dans l'ordre :

1. Sonde SS_CHECK (nouvelle instrumentation faust) : le graphe
   immédiat est un DAG sur TOUT le corpus (sccs>1 = 0 partout — les
   arêtes de retard dmin ≥ 1 ne sont jamais ajoutées par
   sigDependenciesGraph). Les chemins « cycles » de Schedule.hh ne se
   déclenchent donc jamais sur les vrais programmes.
2. Les 12 tirages contrôlés d'alignschedule sont tous des ordres
   topologiquement valides (0 violation d'arête).
3. L'artefact « divergent » de la campagne : 1288 lignes, tampons de
   16384 — ce n'était pas un autre ordre, c'était UN AUTRE PROGRAMME.
   Le corpus a des homonymes (3 clarinet.dsp, 2 bells, 3 brass,
   2 karplus32) et la résolution `find | head -1` du pilote ne choisit
   pas le même fichier selon le contexte : dans les scripts zsh c'est
   /usr/bin/find (ordre readdir, faust-stk/clarinet en tête) ; dans
   les sondes interactives c'est une fonction shell du harnais
   (physicalModeling/clarinet en tête). La « divergence » comparait
   faust-stk/clarinet à la référence df de physicalModeling/clarinet.
   Preuve par recoupement : tous les programmes sans homonyme
   concordent entre contextes (dbmeter 1.242/1.242), tous les
   homonymes divergent.

Conclusions. (a) Aucun bug de correction : 32 générations de
physicalModeling/clarinet toutes bit-exactes à 0.370–0.378, le ×4.8
tient. (b) Le non-déterminisme d'alignschedule (6 md5 sur 20) reste
réel mais n'a montré aucun coût : dossier performance, plus urgence.
(c) La ligne clarinet des campagnes scriptées = faust-stk/clarinet
(align 0.899, honorable) — programme distinct du record interactif.
(d) Hygiène : corpus par CHEMINS EXPLICITES désormais ; interdiction
de comparer sonde interactive et campagne script sur un nom ambigu.
(e) La doctrine « un fix doit être dérivable du contrat de sa
couche » a joué : le correctif envisagé (graph2dag systématique)
n'était dérivable d'aucun contrat violé — et pour cause, rien n'était
violé. L'objection de construction de Yann valait toutes les sondes.

## 2026-08-03 (midi) — bankschedule : l'opposition align/cs dissoute en une famille

Yann : « je ne comprends pas pourquoi on oppose les deux... toutes les
compositions se font sous contrainte, simplement on le fait à partir
d'un DAG préalablement aligné. » C'était sa formulation d'origine, que
l'implémentation avait trahie en faisant de l'alignement une stratégie
terminale. bankschedule (-ss 9) la rétablit :

A. alignement comme ANALYSE : mobilités + rangs cibles (plus d'ordre) ;
B. légalisation (chaque arête croît strictement en rang → bancs
   antichaînes, quotient acyclique PAR CONSTRUCTION — aucune passe de
   vérification) puis condensation en bancs plafonnés ;
C. csschedule sur le DAG des bancs : la composition décide OÙ vont les
   bancs, plus jamais s'ils existent.

check34 : les deux couronnes miniatures d'un coup — blocs(4,4,4) =
align = optimum ; large(16,6,R4) = cs = optimum certifié, le tuilage R
émergeant de la composition une fois les unités de pression converties
(Rb = R/cap, approximation uniforme).

Sur les vrais juges (bit-exacts, 0 violation SS_CHECK) : le plafond de
bancs optimal SUIT R — karplus32 : cap 8 = son R, 1.320 contre cs
1.272 ; vocoder : cap 32, 1.174 contre bf 1.158 (l'align pur : 1.165).
cap > R détruit karplus (la rafale déborde le budget de registres),
cap < 32 détruit vocoder (les rafales monochromes hachées). Lecture :
R petit = pôle localité (cs), R grand = pôle rafales (align) — les
deux stratégies rivales sont devenues UNE famille à un paramètre, à
quasi-parité aux deux pôles. Écart résiduel (+1.4 à +3.7 %) : le prix
de l'approximation uniforme Rb ; le raffinement identifié est la
pression PONDÉRÉE par la taille des bancs dans l'étape C (dpcombine et
le tourniquet comptant des valeurs, pas des bancs).

## 2026-08-03 (après-midi) — campagne famille : l'hybride seul passe sous 1

Campagne mono-programme (rapports intermédiaires à Yann après chaque
mesure), 26 entrées à CHEMINS EXPLICITES (les 22 des campagnes
précédentes + les 4 homonymes de l'autre monde), 26 variantes,
SS_CHECK à chaque génération (zéro rejet).

Le verdict : enveloppe geomean 0.958 (première fois sous 1) ;
l'HYBRIDE SEUL fait 0.981 — la famille à un paramètre bat en moyenne
le meilleur fixe sans ses parents (align seul 1.215, cs seul 1.094).
15/26 gains nets, 4 échecs restants (brass 1.209, spectralLevel 1.128
— artefact de grille, R=1 coupé —, fdnRev 1.089, fire 1.054).
Quatre échecs partagés de la veille tombent par l'hybride seul :
bells 0.909, jprev 0.918, thunder 0.875, churchOrgan 0.992. Et
frenchBell : −47 % (3.66 → 1.94 ms), bit-exact, reproduit — la plus
grosse victoire du chantier ; align seul y fait 0.989 : le banc ne
suffit pas, il faut la composition.

Le curseur R interpole comme prévu (pôle localité R=2 : bells, jprev,
zitaRev ; pôle rafales R=16-32 : thunder, brass-pm, vocoder). Les
parents purs gardent les extrêmes : align les monstres à bancs longs
(bells-it 0.883, clarinet-stk 0.896), cs les monstres récursifs
(bowedString, karplus32-it) — le prix des plafonds uniformes et de
Rb = R/cap. Raffinement identifié : pression PONDÉRÉE par la taille
des bancs dans l'étape C.

Note de méthode : clarinet-pm — dfcycles (f4) atteint 0.372 lui-même ;
le « ×5 » est un ordre accessible, pas une exclusivité. Les quatre
paires d'homonymes mesurées côte à côte : la structure décide du
gagnant, pas le nom.

## 2026-08-03 (soir) — la vraie monnaie : ce que SLP empaquette vraiment

Mandat : instrumenter la distribution des longueurs de rafales
(packs4 = paquets complets de 4 iso-indépendants) et tester la
corrélation sur les cas polaires. Résultat en deux temps :

1. La métrique modèle SÉPARE LE CATASTROPHIQUE mais pas le sommet :
   frenchBell et vocoder corrèlent, mais sur la classe stk (brass,
   clarinet-stk, bells-it) le gagnant a MOINS de paquets-modèle que
   l'hybride (brass : bf gagne avec 112 contre 206).
2. L'assembleur tranche : le compte d'instructions SIMD .4s émises par
   clang prédit le gagnant de CHAQUE paire polaire — brass bf 152 vs
   hyb 25 (runtime 1.000 vs 1.286) ; clarinet-stk align 58 vs hyb 14
   (0.896 vs 1.189) ; frenchBell hyb 127 vs align 24 vs df 0
   (0.530 vs 0.989 vs 1.000). Corrélation parfaite, six sur six.

Diagnostic : nos paquets comptent des rafales d'OPCODES isomorphes ;
le SLP de clang empaquette des CÔNES use-def isomorphes aux voies
d'opérandes cohérentes (les opérandes des 4 nœuds doivent eux-mêmes
former des rafales, récursivement — sinon le coût des gathers tue le
pack). bf préserve cette cohérence de phase par ses balayages
niveau-major ; align la préserve quand ses couches de couleurs
restent en phase ; l'étape C de l'hybride la DÉTRUIT en réordonnant
les bancs par régions de dominateurs (les bancs d'une même colonne de
couleur sont séparés).

Conséquences pour l'étape 2 du plan : (a) le prédicteur modèle à
construire est l'adjacence PROFONDE (paires iso dont les opérandes
sont eux-mêmes iso et adjacents — calculable côté faust où l'ordre
des sous-signaux est connu) ; (b) la cible de calibration n'est plus
le runtime seul : le compte SIMD assembleur est un oracle
intermédiaire, rapide et déterministe, qui évite le bruit de mesure ;
(c) le correctif pressenti de l'étape C : chaîner les bancs
verticalement par classe (colonnes de couleurs, la cohérence de phase
de bf) plutôt que par régions de dominateurs seules.

## 2026-08-03 (nuit) — étape 2 : les modes de l'étape C, et la monnaie qui se cache

Mandat : corriger l'étape C à l'oracle assembleur. Trois mouvements et
une leçon d'humilité expérimentale.

1. VERTICAL (stagec=1, df sur le DAG des bancs) : deux records —
   karplus32 1.17 (bat cs 1.27 sur son propre juge), frenchBell 1.77
   (0.530 → ~0.48 en ratio). Mais réfute l'hypothèse « colonnes » sur
   les stk : brass tombe à 13 SIMD.
2. COUCHES (stagec=2, niveaux ASAP naturels + couleurs groupées dans
   le niveau — « bf trié par formes ») : le déphasage est bien le
   coupable du SIMD stk — brass remonte de 25 à 125 instructions
   vectorielles (bf : 152), runtime 2.13 → 1.82 ; vocoder = bf. Le
   groupage par fréquence de l'étape A arrachait les instances à leur
   niveau naturel, déphasant leurs voies d'opérandes.
3. CONTRE-EXEMPLE clarinet-stk : couches 124 SIMD, align 58 — et
   align gagne de 26 %. Preuve par -fno-slp-vectorize : align sans
   SLP = align avec SLP (1.70 ≈ 1.69). Sur la classe sérielle, le
   SIMD ne compte PAS. Puis cinq suspects statiques éliminés un à un :
   spills (anti-corrélé : align gagne avec PLUS de pile), churn de
   tableaux (plat), manipulations de lanes (plat), dilatation du
   chemin critique mono-chaîne (plate ; karplus-couches 4.7× plus
   lent à dilatation égale), schedulingcost Σd² (ANTI-corrélé :
   karplus-couches meilleur score, pire temps). La deuxième monnaie
   est DYNAMIQUE ; suspect principal : le store-to-load forwarding à
   travers les tampons de retard (la récurrence passe par la mémoire,
   l'ordre change la distance écriture→relecture dépendante).
   Prochaine arme : compteurs matériels (xctrace) sur la paire
   clarinet-stk align/couches.

Bilan pragmatique : aucun mode unique ne gagne partout, mais la
FAMILLE {cs, vertical, couches, align} couvre tous les juges mesurés.
L'adaptativité par région reste la cible architecturale ; en
attendant, la sélection empirique par programme (3-4 modes générés,
départagés à l'oracle + mini-bench) est déployable. Tout est commité
et synchronisé (standalone + tlib + signals + faust, banc vert).

## 2026-08-03 (16h) — l'ordre du DAG des super-nœuds : question de Yann, verdict mesuré

« Notre dag de super-nœuds doit lui-même être schedulé. Quelle
stratégie on emploie actuellement ? » — Réponse : AUCUNE, par
accident. La numérotation des blocs suit le parcours de la partition,
puis retopo() renumérote par un Kahn à pile LIFO (tendance df) que
personne n'a choisi, et l'émission suit les indices. Câblé sous
FAUST_LS_ORDER : le quotient passe par la librairie (df/bf/rb/sp).

Verdict expérimental : PAS un levier aujourd'hui. kahn ≈ df ≈ bf au
bruit près sur reverbTank (11 boucles), vocalBP (14), 2dKirchhoff (5),
zitaRev (5), fdnRev (20) et même vocalFOF (140 boucles) ; seul l'ordre
adversarial rb coûte (+12 % reverbTank). Interprétation : les tampons
inter-boucles (count + délais) tiennent en L1 quel que soit l'ordre
deps-first à ces échelles — la localité inter-boucles est déjà servie.
Le câblage reste comme instrumentation (partitions plus fines à
venir). Le levier réel demeure INTRA-boucle — où -ls-sched n'a que
df/bf/model et ignore tout des découvertes du jour (phase, modes).

## 2026-08-03 (17h) — question de Yann sur l'ouverture des boucles longues : audit complet

« Quand on ouvre les boucles récursives longues > vector-size,
tient-on compte de l'ordre producteur/consommateur, car il faut
corriger les accès aux lignes à retard et les allonger ? »

L'audit répond en trois pièces, qui s'alignent :
1. SEUIL : la partition d'émission est construite avec
   freeDelayThreshold = gVecSize — seules les lectures certifiées
   dmin >= chunk perdent leur arête d'ordre (elles ne touchent que
   les chunks précédents).
2. ANNEAUX : taillés sz >= maxD + vs (PAS le pow2limit(maxD+1) du
   classique où IOTA avance par échantillon) — les 32 écritures d'un
   chunk ne peuvent jamais rattraper l'historique lu, quel que soit
   l'ordre des blocs.
3. LINÉAIRE : tampon vs + maxD, écritures au suffixe, lectures libres
   au préfixe — séparation structurelle.
La conception répond par l'INDÉPENDANCE D'ORDRE PAR CONSTRUCTION
(les deux ordres sont sûrs) plutôt que par correction des accès selon
l'ordre choisi. Preuve adversariale : FAUST_LS_ORDER=rb (ordre des
blocs RENVERSÉ, consommateurs libres avant producteurs) est bit-exact
contre l'émission classique sur reverbTank, zitaRev, fdnRev, vocalFOF
et karplus32.

En chemin, la même séance a débusqué et corrigé le vrai trou (voisin
mais distinct) : les lectures d0 via tampon des TAPS ALIASÉS à
décalage nul n'avaient pas d'arête RAW vers le store de leur hôte —
df survivait par coïncidence d'ordre de création, model lisait des
cases périmées (zitaRev divergent dès l'échantillon 33, reverbTank en
résidu 1e-10). Corrigé dans refOperand (lecture enveloppée dans un op
dépendant de fStoreOf[hôte]) ; les trois ordres intra-boucle sont
bit-exacts contre le classique, et le correctif AMÉLIORE les temps :
reverbTank fuse-model 5.16 → 4.58 ms, frenchBell fuse-model
1.46 → 1.30 ms (meilleur frenchBell jamais mesuré, 0.36 du classique)
— model optimisait avec une liberté illégale, et l'alias lu une fois
en registre remplace les relectures du tampon.

Et le croisement -ls-sched layers : résultat NÉGATIF documenté —
les corps de boucles sont en régime de vectorisation de boucle
(sur i), pas SLP ; la monnaie de phase du grain Tree ne s'y transfère
pas. model reste le bon outil intra-boucle.

## 2026-08-03 (fin de soirée) — costly-2 : le correctif RAW validé en campagne

Rejeu de la campagne costly avec le correctif : 10/10 bit-exact, tous
les programmes améliorés ou stables (piano1 0.98→0.91, reverbTank
0.98→0.93, pluckedString 1.51→1.32, vocalBPMIDI 0.79), geomean
enveloppe-ls/classique 1.073 → 1.030. Le bug des alias ne coûtait pas
que la correction : il coûtait de la performance partout.

## 2026-08-03 (nuit) — le modèle à ressources de Yann : bornes calibrées, sélecteur en vue

Proposition de Yann : la bande passante mémoire est une ressource ;
un modèle de contraintes approximatif ne peut fonder une stratégie
optimale ; faut-il un paramètre d'E/S simultanées ? Réponse en trois
implémentations (toutes committées, banc vert) :

1. Machine à DEUX ressources dans squality (U slots calcul, M ports
   mémoire, classif des nœuds Tree) — le cadre modulo-scheduling
   II = max(ResMII par ressource, RecMII) [Lam 88, Rau 94] ; roofline
   et decoupled access/execute cités au dossier.
2. Coûts calibrés par op (mul/add 3, load 4, div 10, libm 25). Le
   chemin critique pondéré du squelette complet s'est révélé
   NON-discriminant (les épines feed-forward se recouvrent d'un
   échantillon à l'autre — seuls les nids serrés bornent).
3. recMII = profondeur pondérée des nids de récurrence à distance 1
   (cut(fullGraph, 2)). LE CLASSEMENT REJOUE LA CARTE DES CAMPAGNES :
   recMII >= 50 (clarstk/brass/zita 112, fdnRev 193, violin 67,
   bowedS 53) = jeu de localité, gagnants cs/align-compact ;
   recMII <= 34 (vocoder/fbell/karplus 19, bellsit 34) = jeu de
   débit, gagnants bf/align/rafales (fbell -47 %).

Candidat sélecteur statique pour l'adaptativité de l'étape C :
recMII/aluMII. Limites connues : précision d'intervalle sur les
minima de retard (fdnRev : certificats dmin=1 pour des lignes de
~100), et karplus dont l'affinité localité vient du cache (3e
ressource, flux) et non de la récurrence. Prochaine marche : brancher
le sélecteur dans bankschedule (régime par région), et Karp
(ratio de cycle max) si la précision des nids doit monter.
