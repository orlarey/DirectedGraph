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

### À suivre

- Dossier 2 (déterminisme) : digraph<N, Compare> de bout en bout.
- Dossier 3 : profiler spschedule/recschedule sur virtualAnalog.
- mcschedule : banc de calibration de R au grain classique sur le
  corpus (la campagne des ordres a l'infrastructure) ; envisager un
  paramètre de poids par nœud (les expressions n'ont pas toutes la
  même largeur).
