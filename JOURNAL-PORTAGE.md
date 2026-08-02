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

### À suivre

- Dossier 2 (déterminisme) : digraph<N, Compare> de bout en bout.
- Dossier 3 : profiler spschedule/recschedule sur virtualAnalog.
- mcschedule : banc de calibration de R au grain classique sur le
  corpus (la campagne des ordres a l'infrastructure) ; envisager un
  paramètre de poids par nœud (les expressions n'ont pas toutes la
  même largeur).
