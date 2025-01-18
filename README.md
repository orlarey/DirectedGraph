![C/C++ CI](https://github.com/orlarey/DirectedGraph/actions/workflows/ubuntu.yml/badge.svg)![C/C++ CI](https://github.com/orlarey/DirectedGraph/actions/workflows/macos.yml/badge.svg)![C/C++ CI](https://github.com/orlarey/DirectedGraph/actions/workflows/windows.yml/badge.svg)

# Directed Graph library
A simple header-only library to create and manipulate directed graphs. Just add the DirectedGraph folder to your project.

## API Documentation

### Class `digraph`

The `digraph` class represents a directed graph with nodes of type `N` and connections between these nodes.

#### Methods

- `add(N n)`: Adds a node `n` to the graph.
- `add(const N& n1, const N& n2, const TWeights& w)`: Adds two nodes with a set of connections of weights `w`.
- `add(const N& n1, const N& n2, int d = 0)`: Adds the nodes `n1` and `n2` and the connection `(n1 -d-> n2)` to the graph.
- `add(const digraph& g)`: Adds an entire graph `g`.
- `nodes() const`: Returns the set of nodes in the graph.
- `connections() const`: Returns the set of connections in the graph.
- `destinations(const N& n) const`: Returns the destinations of node `n` in the graph.
- `weights(const N& n1, const N& n2) const`: Returns the weights of the connections between two nodes.
- `areConnected(const N& n1, const N& n2) const`: Returns `true` if there is a connection between nodes `n1` and `n2`.
- `areConnected(const N& n1, const N& n2, int& d) const`: Returns `true` if there is a connection between nodes `n1` and `n2` and the smallest weight is returned in `d`.

### Class `Tarjan`

The `Tarjan` class partitions a graph into strongly connected components.

#### Methods

- `partition() const`: Returns the partition of the graph into strongly connected components.
- `cycles() const`: Returns the number of cycles in the graph.

### Functions

- `cycles(const digraph<N>& g)`: Counts the number of cycles in a graph.
- `graph2dag(const digraph<N>& g)`: Transforms a graph into a DAG of supernodes.
- `parallelize(const digraph<N>& g)`: Transforms a DAG into a sequential vector of parallel vectors of nodes.
- `serialize(const digraph<N>& G)`: Transforms a DAG into a sequence of nodes using a topological sort.
- `mapnodes(const digraph<N>& g, std::function<M(const N&)> foo)`: Transforms a graph by applying `foo` to each node.
- `reverse(const digraph<N>& g)`: Reverses all the connections of a graph.
- `splitgraph(const digraph<N>& G, std::function<bool(const N&)> left, digraph<N>& L, digraph<N>& R)`: Splits a graph into two subgraphs according to a predicate.
- `subgraph(const digraph<N>& G, const std::set<N>& S)`: Extracts a subgraph of `G` according to a set of nodes `S`.
- `cut(const digraph<N>& G, int dm)`: Cuts all the connections of a graph with weight >= `dm`.
- `chain(const digraph<N>& g, bool strict)`: Keeps only the chain connections of a graph.
- `roots(const digraph<N>& G)`: Returns the roots of a graph.
- `leaves(const digraph<N>& G)`: Returns the leaves of a graph.
- `criticalpath(const digraph<N>& G, const N& n)`: Computes the critical path of a graph.
- `interleave(std::list<N>& list1, std::list<N>& list2)`: Interleaves two lists.
- `recschedulenode(const digraph<N>& G, const N& n)`: Recursive scheduling of a node of a DAG.
- `recschedule(const digraph<N>& G)`: Recursive scheduling of the roots of a DAG.
- `topology(const digraph<N>& g)`: High-level description of a graph.

### Class `schedule`

The `schedule` class represents an order of computation of the nodes of a DAG.

#### Methods

- `size() const`: Returns the number of elements in the schedule.
- `elements() const`: Returns the vector of elements.
- `order(const N& n) const`: Returns the order of an element in the schedule.
- `append(const N& n)`: Adds a new element to the schedule.
- `append(const schedule<N>& S)`: Adds all the elements of a schedule.
- `reverse() const`: Returns a schedule in reverse order.

#### Functions

- `dfschedule(const digraph<N>& G)`: Deep-first scheduling of a DAG.
- `bfschedule(const digraph<N>& G)`: Breadth-first scheduling of a DAG.
- `spschedule(const digraph<N>& G)`: Special schedule for a DAG.
- `schedulingcost(const digraph<N>& G, const schedule<N>& S)`: Computes the cost of a schedule.
- `dfcyclesschedule(const digraph<N>& G)`: Deep-first scheduling of a directed graph with cycles.
- `bfcyclesschedule(const digraph<N>& G)`: Breadth-first scheduling of a directed graph with cycles.
- `rbschedule(const digraph<N>& G)`: Reverse breadth-first schedule for a DAG.