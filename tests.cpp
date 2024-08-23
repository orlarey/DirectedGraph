//
//  tests.cpp
//  graphlib
//
//  Created by Yann Orlarey on 06/02/2022.
//  Copyright © 2023 Yann Orlarey. All rights reserved.
//
#include <algorithm>

#include "DirectedGraph.hh"
#include "DirectedGraphAlgorythm.hh"
#include "tests.hh"

void test0(std::ostream& ss)
{
    digraph<char> g;
    g.add('A', 'B')
        .add('B', 'C', 1)
        .add('C', 'A')
        .add('D', 'B')
        .add('D', 'C')
        .add('D', 'E', 1)
        .add('E', 'D')
        .add('E', 'F')
        .add('F', 'G')
        .add('G', 'F', 1)
        .add('H', 'G')
        .add('H', 'E')
        .add('H', 'H', 1);

    ss << "g = " << g;
}

std::string res0()
{
    return "g = Graph {A-set{0}->B, B-set{1}->C, C-set{0}->A, D-set{0}->B, D-set{0}->C, "
           "D-set{1}->E, E-set{0}->D, "
           "E-set{0}->F, F-set{0}->G, G-set{1}->F, H-set{0}->E, H-set{0}->G, H-set{1}->H}";
}

bool check0()
{
    std::stringstream ss;
    test0(ss);
    bool ok = (0 == ss.str().compare(res0()));
    if (ok) {
        std::cout << "test0 OK " << '\n';
    } else {
        std::cout << "test0 FAIL " << '\n';
        std::cout << "We got     " << ss.str() << '\n';
        std::cout << "instead of " << res0() << '\n';
    }
    return ok;
}

void test1(std::ostream& ss)
{
    digraph<char> g;
    g.add('A', 'B')
        .add('B', 'C')
        .add('C', 'A')
        .add('D', 'B')
        .add('D', 'C')
        .add('D', 'E')
        .add('E', 'D')
        .add('E', 'F')
        .add('F', 'G')
        .add('G', 'F')
        .add('H', 'G')
        .add('H', 'E')
        .add('H', 'H');

    ss << "Tarjan partition of g = ";
    Tarjan tarj(g);
    for (const auto& cycle : tarj.partition()) {
        ss << "group{ ";
        for (const auto& n : cycle) {
            ss << n << " ";
        }
        ss << "} ";
    }
}

std::string res1()
{
    return "Tarjan partition of g = group{ A B C } group{ D E } group{ F G } group{ H } ";
}

bool check1()
{
    std::stringstream ss;
    test1(ss);
    bool ok = (0 == ss.str().compare(res1()));
    if (ok) {
        std::cout << "test1 OK " << '\n';
    } else {
        std::cout << "test1 FAIL " << '\n';
        std::cout << "We got     " << ss.str() << '\n';
        std::cout << "instead of " << res1() << '\n';
    }
    return ok;
}

void test2(std::ostream& ss)
{
    digraph<char> g;
    g.add('A', 'B')
        .add('B', 'C', 1)
        .add('C', 'A')
        .add('D', 'B')
        .add('D', 'C')
        .add('D', 'E', 1)
        .add('E', 'D')
        .add('E', 'F')
        .add('F', 'G')
        .add('G', 'F', 1)
        .add('H', 'G')
        .add('H', 'E')
        .add('H', 'H', 1);

    ss << "dag of g = " << graph2dag(g);
}

std::string res2()
{
    return "dag of g = Graph {Graph {A-set{0}->B, B-set{1}->C, C-set{0}->A}, Graph {D-set{1}->E, "
           "E-set{0}->D}-set{0}->Graph {A-set{0}->B, B-set{1}->C, C-set{0}->A}, Graph "
           "{D-set{1}->E, "
           "E-set{0}->D}-set{0}->Graph {F-set{0}->G, G-set{1}->F}, Graph {F-set{0}->G, "
           "G-set{1}->F}, Graph "
           "{H-set{1}->H}-set{0}->Graph {D-set{1}->E, E-set{0}->D}, Graph "
           "{H-set{1}->H}-set{0}->Graph {F-set{0}->G, "
           "G-set{1}->F}}";
}

bool check2()
{
    std::stringstream ss;
    test2(ss);
    bool ok = (0 == ss.str().compare(res2()));
    if (ok) {
        std::cout << "test2 OK " << '\n';
    } else {
        std::cout << "test2 FAIL " << '\n';
        std::cout << "We got     " << ss.str() << '\n';
        std::cout << "instead of " << res2() << '\n';
    }
    return ok;
}

void test3(std::ostream& ss)
{
    digraph<char> g;
    g.add('A', 'B')
        .add('B', 'C', 1)
        .add('C', 'A')
        .add('D', 'B')
        .add('D', 'C')
        .add('D', 'E', 1)
        .add('E', 'D')
        .add('E', 'F')
        .add('F', 'G')
        .add('G', 'F', 1)
        .add('H', 'G')
        .add('H', 'E')
        .add('H', 'H', 1);

    auto h1 = cut(g, 64);     // cut vectorsize connections
    auto h2 = graph2dag(h1);  // find cycles
    auto h3 = mapnodes<digraph<char>, digraph<char>>(
        h2, [](const digraph<char>& gr) { return cut(gr, 1); });
    ss << "test3: h3= " << h3;
}

std::string res3()
{
    return "test3: h3= Graph {Graph {A-set{0}->B, B, C-set{0}->A}, Graph {D, "
           "E-set{0}->D}-set{0}->Graph {A-set{0}->B, "
           "B, "
           "C-set{0}->A}, Graph {D, E-set{0}->D}-set{0}->Graph {F-set{0}->G, G}, Graph "
           "{F-set{0}->G, G}, Graph "
           "{H}-set{0}->Graph {D, E-set{0}->D}, Graph {H}-set{0}->Graph {F-set{0}->G, G}}";
}

bool check3()
{
    std::stringstream ss;
    test3(ss);
    bool ok = (0 == ss.str().compare(res3()));
    if (ok) {
        std::cout << "test3 OK " << '\n';
    } else {
        std::cout << "test3 FAIL " << '\n';
        std::cout << "We got     \"" << ss.str() << '"' << '\n';
        std::cout << "instead of \"" << res3() << '"' << '\n';
    }
    return ok;
}
void test4(std::ostream& ss)
{
    digraph<char> g1, g2, g3;
    g1.add('A', 'A', 2);
    g1.add('A', 'D', 1);
    g1.add('A', 'B', 0);

    g2.add('A', 'B', 2);
    g2.add('B', 'B', 1);
    g2.add('A', 'C', 0);

    g3.add(g1).add(g2);

    ss << "test4: g3.add(g1).add(g2) = " << g3;
    // std::cout << "test4: g1= " << g1 << '\n';
    // std::cout << "test4: g2= " << g2 << '\n';
    // std::cout << "test4: g3= " << g3 << '\n';
}

std::string res4()
{
    return "test4: g3.add(g1).add(g2) = Graph {A-set{2}->A, A-set{0, 2}->B, A-set{0}->C, "
           "A-set{1}->D, B-set{1}->B, C, "
           "D}";
};

bool check4()
{
    std::stringstream ss;
    test4(ss);
    bool ok = (0 == ss.str().compare(res4()));
    if (ok) {
        std::cout << "test4 OK " << '\n';
    } else {
        std::cout << "test4 FAIL " << '\n';
        std::cout << "We got     \"" << ss.str() << '"' << '\n';
        std::cout << "instead of \"" << res4() << '"' << '\n';
    }
    return ok;
}

// Test separation of graphs
void test5(std::ostream& ss)
{
    digraph<char> g1, g2, g3;

    g1.add('A', 'Z');
    g1.add('A', 'B');
    g1.add('B', 'C');
    g1.add('C', 'A', 1);

    g1.add('Z', 'W');
    g1.add('W', 'Z', 1);

    g1.add('A', 'Z');
    g1.add('W', 'C', 1);

    splitgraph<char>(
        g1, [](const char& c) { return c < 'K'; }, g2, g3);

    ss << "test5: g1 = " << g1 << "; g2 = " << g2 << "; g3 = " << g3;
}

std::string res5()
{
    return "test5: g1 = Graph {A-set{0}->B, A-set{0}->Z, B-set{0}->C, C-set{1}->A, W-set{1}->C, "
           "W-set{1}->Z, "
           "Z-set{0}->W}; g2 = Graph {A-set{0}->B, B-set{0}->C, C-set{1}->A}; g3 = Graph "
           "{W-set{1}->Z, Z-set{0}->W}";
};

bool check5()
{
    std::stringstream ss;
    test5(ss);
    bool ok = (0 == ss.str().compare(res5()));
    if (ok) {
        std::cout << "test5 OK " << '\n';
    } else {
        std::cout << "test5 FAIL " << '\n';
        std::cout << "We got     \"" << ss.str() << '"' << '\n';
        std::cout << "instead of \"" << res5() << '"' << '\n';
    }
    return ok;
}

void test6(std::ostream& ss)
{
    digraph<char> g;
    g.add('A', 'B')
        .add('B', 'C', 1)
        .add('C', 'A')
        .add('D', 'B')
        .add('D', 'C')
        .add('D', 'E', 1)
        .add('E', 'D')
        .add('E', 'F')
        .add('F', 'G')
        .add('G', 'F', 1)
        .add('H', 'G')
        .add('H', 'E')
        .add('H', 'H', 1);

    ss << "test6:        g = " << g << "; ";
    ss << "number of cycles: " << cycles(g) << "; ";
    auto gc = cut(g, 1);
    ss << "gc = " << gc << "; ";
    auto h = graph2dag(g);  // on fait un dag dont les noeuds sont les cycles du graphe g
    ss << "graph2dag(g)    = " << h << "; ";
    auto p = parallelize(h);  //
    ss << "parallelize(h)  = " << p << "; ";
    auto rp = rparallelize(h);  //
    ss << "rparallelize(h)  = " << rp << "; ";
    auto s = serialize(h);  //
    ss << "serialize(h)    = " << s << "; ";
}

std::string res6()
{
    return "test6:        g = Graph {A-set{0}->B, B-set{1}->C, C-set{0}->A, D-set{0}->B, "
           "D-set{0}->C, D-set{1}->E, "
           "E-set{0}->D, E-set{0}->F, F-set{0}->G, G-set{1}->F, H-set{0}->E, H-set{0}->G, "
           "H-set{1}->H}; number of "
           "cycles: 4; gc = Graph {A-set{0}->B, B, C-set{0}->A, D-set{0}->B, D-set{0}->C, "
           "E-set{0}->D, E-set{0}->F, "
           "F-set{0}->G, G, H-set{0}->E, H-set{0}->G}; graph2dag(g)    = Graph {Graph "
           "{A-set{0}->B, B-set{1}->C, "
           "C-set{0}->A}, Graph {D-set{1}->E, E-set{0}->D}-set{0}->Graph {A-set{0}->B, "
           "B-set{1}->C, C-set{0}->A}, "
           "Graph {D-set{1}->E, E-set{0}->D}-set{0}->Graph {F-set{0}->G, G-set{1}->F}, Graph "
           "{F-set{0}->G, "
           "G-set{1}->F}, Graph {H-set{1}->H}-set{0}->Graph {D-set{1}->E, E-set{0}->D}, Graph "
           "{H-set{1}->H}-set{0}->Graph {F-set{0}->G, G-set{1}->F}}; parallelize(h)  = "
           "std::vector{std::vector{Graph "
           "{A-set{0}->B, B-set{1}->C, C-set{0}->A}, Graph {F-set{0}->G, G-set{1}->F}}, "
           "std::vector{Graph "
           "{D-set{1}->E, E-set{0}->D}}, std::vector{Graph {H-set{1}->H}}}; rparallelize(h)  = "
           "std::vector{std::vector{Graph {H-set{1}->H}}, std::vector{Graph {D-set{1}->E, "
           "E-set{0}->D}}, "
           "std::vector{Graph {A-set{0}->B, B-set{1}->C, C-set{0}->A}, Graph {F-set{0}->G, "
           "G-set{1}->F}}}; "
           "serialize(h)    = std::vector{Graph {A-set{0}->B, B-set{1}->C, C-set{0}->A}, Graph "
           "{F-set{0}->G, "
           "G-set{1}->F}, Graph {D-set{1}->E, E-set{0}->D}, Graph {H-set{1}->H}}; ";
}

bool check6()
{
    std::stringstream ss;
    test6(ss);
    bool ok = (0 == ss.str().compare(res6()));
    if (ok) {
        std::cout << "test6 OK " << '\n';
    } else {
        std::cout << "test6 FAIL " << '\n';
        std::cout << "We got     \"" << ss.str() << '"' << '\n';
        std::cout << "instead of \"" << res6() << '"' << '\n';
    }
    return ok;
}

void test7(std::ostream& ss)
{
    digraph<char> g;
    g.add('A', 'B')
        .add('B', 'C', 1)
        .add('C', 'A')
        .add('D', 'B')
        .add('D', 'C')
        .add('D', 'E', 1)
        .add('E', 'D')
        .add('E', 'F')
        .add('F', 'G')
        .add('G', 'F', 1)
        .add('H', 'G')
        .add('H', 'E')
        .add('H', 'H', 1);

    auto h  = graph2dag(g);     // on fait un dag dont les noeuds sont les cycles du graphe g
    auto p  = parallelize(h);   //
    auto rp = rparallelize(h);  //
    auto s  = serialize(h);     //

    ss << "test7:        g = " << g << ";" << '\n';
    ss << "number of cycles: " << cycles(g) << ";" << '\n';

    auto gc = cut(g, 1);
    ss << "0-cycles        : " << cycles(gc) << "; ";
    ss << "graph2dag(g)    = " << h << "; ";
    ss << "parallelize(h)  = " << p << "; ";
    ss << "rparallelize(h)  = " << rp << "; ";
    ss << "serialize(h)    = " << s << "; ";

    dotfile(ss, g);
}

std::string res7()
{
    return "test7:        g = Graph {A-set{0}->B, B-set{1}->C, C-set{0}->A, D-set{0}->B, "
           "D-set{0}->C, D-set{1}->E, "
           "E-set{0}->D, E-set{0}->F, F-set{0}->G, G-set{1}->F, H-set{0}->E, H-set{0}->G, "
           "H-set{1}->H}; number of "
           "cycles: 4; 0-cycles        : 0; graph2dag(g)    = Graph {Graph {A-set{0}->B, "
           "B-set{1}->C, "
           "C-set{0}->A}, "
           "Graph {D-set{1}->E, E-set{0}->D}-set{0}->Graph {A-set{0}->B, B-set{1}->C, "
           "C-set{0}->A}, Graph "
           "{D-set{1}->E, E-set{0}->D}-set{0}->Graph {F-set{0}->G, G-set{1}->F}, Graph "
           "{F-set{0}->G, G-set{1}->F}, "
           "Graph {H-set{1}->H}-set{0}->Graph {D-set{1}->E, E-set{0}->D}, Graph "
           "{H-set{1}->H}-set{0}->Graph "
           "{F-set{0}->G, G-set{1}->F}}; parallelize(h)  = std::vector{std::vector{Graph "
           "{A-set{0}->B, "
           "B-set{1}->C, "
           "C-set{0}->A}, Graph {F-set{0}->G, G-set{1}->F}}, std::vector{Graph {D-set{1}->E, "
           "E-set{0}->D}}, "
           "std::vector{Graph {H-set{1}->H}}}; rparallelize(h)  = std::vector{std::vector{Graph "
           "{H-set{1}->H}}, "
           "std::vector{Graph {D-set{1}->E, E-set{0}->D}}, std::vector{Graph {A-set{0}->B, "
           "B-set{1}->C, "
           "C-set{0}->A}, "
           "Graph {F-set{0}->G, G-set{1}->F}}}; serialize(h)    = std::vector{Graph {A-set{0}->B, "
           "B-set{1}->C, "
           "C-set{0}->A}, Graph {F-set{0}->G, G-set{1}->F}, Graph {D-set{1}->E, E-set{0}->D}, "
           "Graph "
           "{H-set{1}->H}}; "
           "digraph mygraph {\n        \"A\"->\"B\" [label=\"set{0}\"];\n        \"B\"->\"C\" "
           "[label=\"set{1}\"];\n    "
           " "
           " "
           "  "
           "\"C\"->\"A\" [label=\"set{0}\"];\n        \"D\"->\"B\" [label=\"set{0}\"];\n        "
           "\"D\"->\"C\" "
           "[label=\"set{0}\"];\n        \"D\"->\"E\" [label=\"set{1}\"];\n        \"E\"->\"D\" "
           "[label=\"set{0}\"];\n  "
           "      \"E\"->\"F\" [label=\"set{0}\"];\n        \"F\"->\"G\" [label=\"set{0}\"];\n     "
           "   \"G\"->\"F\" "
           "[label=\"set{1}\"];\n        \"H\"->\"E\" [label=\"set{0}\"];\n        \"H\"->\"G\" "
           "[label=\"set{0}\"];\n  "
           "      \"H\"->\"H\" [label=\"set{1}\"];\n}\n";
}

bool check7()
{
    std::stringstream ss;
    test7(ss);
    bool ok = (0 == ss.str().compare(res7()));
    if (ok) {
        std::cout << "test7 OK " << '\n';
    } else {
        std::cout << "test7 FAIL " << '\n';
        std::cout << "We got     \"" << ss.str() << '"' << '\n';
        std::cout << "instead of \"" << res7() << '"' << '\n';
    }
    return ok;
}

void test8(std::ostream& ss)
{
    digraph<char> g;
    g.add('A', 'B')
        .add('B', 'C')
        .add('C', 'A')
        .add('D', 'B')
        .add('D', 'C')
        .add('D', 'E')
        .add('E', 'D')
        .add('E', 'F')
        .add('F', 'G')
        .add('G', 'F')
        .add('H', 'G')
        .add('H', 'E')
        .add('H', 'H');

    std::set<char> S{'C', 'F'};
    digraph<char>  r = subgraph(g, S);
    ss << "Subgraph(" << g << ", " << S << ") = " << r;
}

std::string res8()
{
    return "Subgraph(Graph {A-set{0}->B, B-set{0}->C, C-set{0}->A, D-set{0}->B, D-set{0}->C, "
           "D-set{0}->E, E-set{0}->D, "
           "E-set{0}->F, F-set{0}->G, G-set{0}->F, H-set{0}->E, H-set{0}->G, H-set{0}->H}, set{C, "
           "F}) = Graph "
           "{A-set{0}->B, B-set{0}->C, C-set{0}->A, F-set{0}->G, G-set{0}->F}";
}

bool check8()
{
    std::stringstream ss;
    test8(ss);
    bool ok = (0 == ss.str().compare(res8()));
    if (ok) {
        std::cout << "test8 OK " << '\n';
    } else {
        std::cout << "test8 FAIL " << '\n';
        std::cout << "We got     " << ss.str() << '\n';
        std::cout << "instead of " << res8() << '\n';
    }
    return ok;
}

void test9(std::ostream& ss)
{
    digraph<char> g;
    g.add('A', 'B')
        .add('B', 'C')
        .add('C', 'A')
        .add('D', 'B')
        .add('D', 'C')
        .add('D', 'E')
        .add('E', 'D')
        .add('E', 'F')
        .add('F', 'G')
        .add('G', 'F')
        .add('H', 'G')
        .add('H', 'E')
        .add('H', 'H');

    std::set<char> S{'H'};
    digraph<char>  r = subgraph(g, S);
    ss << "Subgraph(" << g << ", " << S << ") = " << r;
}

std::string res9()
{
    return "Subgraph(Graph {A-set{0}->B, B-set{0}->C, C-set{0}->A, D-set{0}->B, D-set{0}->C, "
           "D-set{0}->E, E-set{0}->D, "
           "E-set{0}->F, F-set{0}->G, G-set{0}->F, H-set{0}->E, H-set{0}->G, H-set{0}->H}, set{H}) "
           "= Graph "
           "{A-set{0}->B, B-set{0}->C, C-set{0}->A, D-set{0}->B, D-set{0}->C, D-set{0}->E, "
           "E-set{0}->D, E-set{0}->F, "
           "F-set{0}->G, G-set{0}->F, H-set{0}->E, H-set{0}->G, H-set{0}->H}";
}

bool check9()
{
    std::stringstream ss;
    test9(ss);
    bool ok = (0 == ss.str().compare(res9()));
    if (ok) {
        std::cout << "test9 OK " << '\n';
    } else {
        std::cout << "test9 FAIL " << '\n';
        std::cout << "We got     " << ss.str() << '\n';
        std::cout << "instead of " << res9() << '\n';
    }
    return ok;
}

template <typename N>
bool equiv(const digraph<N>& g, const digraph<N>& h)
{
    if (g.nodes() != h.nodes()) {
        return false;
    }
    return std::ranges::all_of(
        g.nodes(), [g, h](const auto& n) { return g.destinations(n) == h.destinations(n); });
}

bool check10()
{
    digraph<char> g;
    g.add('A', 'B')
        .add('B', 'C', 1)
        .add('C', 'A')
        .add('D', 'B')
        .add('D', 'C')
        .add('D', 'E', 1)
        .add('E', 'D')
        .add('E', 'F')
        .add('F', 'G')
        .add('G', 'F', 1)
        .add('H', 'G')
        .add('H', 'E')
        .add('H', 'H', 1);

    digraph<char> r = reverse(reverse(g));
    if (equiv(g, r)) {
        std::cout << "test10 OK " << '\n';
        return true;
    }
    std::cout << "test10 FAIL " << '\n';
    std::cout << "We got      " << r << '\n';
    std::cout << "Instead of  " << g << '\n';
    return false;
}

void test11(std::ostream& ss)
{
    digraph<char> g;
    g.add('A', 'B').add('B', 'C', 3).add('C', 'D').add('D', 'E');
    g.add('A', 'I').add('I', 'J', 4).add('J', 'E', 5);

    digraph<char> r = chain(g, false);
    ss << "chain(" << g << ", false) = " << r;
}

std::string res11()
{
    return "chain(Graph {A-set{0}->B, A-set{0}->I, B-set{3}->C, C-set{0}->D, D-set{0}->E, E, "
           "I-set{4}->J, "
           "J-set{5}->E}, false) = Graph {A, B-set{3}->C, C-set{0}->D, D, E, I-set{4}->J, J}";
}

bool check11()
{
    std::stringstream ss;
    test11(ss);
    bool ok = (0 == ss.str().compare(res11()));
    if (ok) {
        std::cout << "test11 OK " << '\n';
    } else {
        std::cout << "test11 FAIL " << '\n';
        std::cout << "We got     " << ss.str() << '\n';
        std::cout << "instead of " << res11() << '\n';
    }
    return ok;
}

//=====================================================================

void test12(std::ostream& ss)
{
    digraph<char> g;
    g.add('A', 'B').add('B', 'C', 3).add('C', 'D').add('D', 'E');
    g.add('A', 'I').add('I', 'J', 4).add('J', 'E', 5);

    digraph<char> r = chain(g, true);
    ss << "chain(" << g << ", true) = " << r;
}

std::string res12()
{
    return "chain(Graph {A-set{0}->B, A-set{0}->I, B-set{3}->C, C-set{0}->D, D-set{0}->E, E, "
           "I-set{4}->J, "
           "J-set{5}->E}, true) = Graph {B-set{3}->C, C-set{0}->D, D, I-set{4}->J, J}";
}

bool check12()
{
    std::stringstream ss;
    test12(ss);
    bool ok = (0 == ss.str().compare(res12()));
    if (ok) {
        std::cout << "test12 OK " << '\n';
    } else {
        std::cout << "test12 FAIL " << '\n';
        std::cout << "We got     " << ss.str() << '\n';
        std::cout << "instead of " << res12() << '\n';
    }
    return ok;
}

//=====================================================================
void test13(std::ostream& ss)
{
    // roots and leaves
    digraph<char> h;
    h.add('A', 'B').add('B', 'C').add('C', 'D').add('B', 'G');
    h.add('E', 'F').add('F', 'G').add('G', 'H');  //.add('F', 'C');

    ss << "graph h   : " << h << '\n';
    ss << "roots(h)  : " << roots(h) << '\n';
    ss << "leafs(h)  : " << leaves(h) << '\n';
    ss << "DF" << dfschedule(h) << ", cost: " << schedulingcost(h, dfschedule(h)) << '\n';
    ss << "BF" << bfschedule(h) << ", cost: " << schedulingcost(h, bfschedule(h)) << '\n';
}

std::string res13()
{
    return "graph h   : Graph {A-set{0}->B, B-set{0}->C, B-set{0}->G, C-set{0}->D, D, E-set{0}->F, "
           "F-set{0}->G, "
           "G-set{0}->H, H}\n"
           "roots(h)  : std::vector{A, E}\n"
           "leafs(h)  : std::vector{D, H}\n"
           "DFSchedule {1:D, 2:C, 3:H, 4:G, 5:B, 6:A, 7:F, 8:E}, cost: 11\n"
           "BFSchedule {1:D, 2:H, 3:C, 4:G, 5:B, 6:F, 7:A, 8:E}, cost: 13\n";
}

bool check13()
{
    std::stringstream ss;
    test13(ss);
    bool ok = (0 == ss.str().compare(res13()));
    if (ok) {
        std::cout << "test13 OK " << '\n';
    } else {
        std::cout << "test13 FAIL " << '\n';
        std::cout << "We got     " << '[' << ss.str() << ']' << '\n';
        std::cout << "instead of " << '[' << res13() << ']' << '\n';
    }
    return ok;
}

//=====================================================================

void test14(std::ostream& ss)
{
    // roots and leaves
    digraph<char> h;
    h.add('A', 'B').add('B', 'C').add('C', 'D').add('B', 'G', 1).add('C', 'B', 1);
    h.add('E', 'F').add('F', 'G').add('G', 'H').add('G', 'F', 2);

    ss << "graph h   : " << h << '\n';
    ss << dfcyclesschedule(h) << ", cost: " << schedulingcost(h, dfcyclesschedule(h)) << '\n';
}

std::string res14()
{
    return "graph h   : Graph {A-set{0}->B, B-set{0}->C, B-set{1}->G, C-set{1}->B, C-set{0}->D, D, "
           "E-set{0}->F, "
           "F-set{0}->G, G-set{2}->F, G-set{0}->H, H}\n"
           "Schedule {1:D, 2:H, 3:G, 4:F, 5:C, 6:B, 7:A, 8:E}, cost: 17\n";
}

bool check14()
{
    std::stringstream ss;
    test14(ss);
    bool ok = (0 == ss.str().compare(res14()));
    if (ok) {
        std::cout << "test14 OK " << '\n';
    } else {
        std::cout << "test14 FAIL " << '\n';
        std::cout << "We got     " << ss.str() << '\n';
        std::cout << "instead of " << res14() << '\n';
    }
    return ok;
}

//=====================================================================

void test15(std::ostream& ss)
{
    // roots and leaves
    digraph<char> h;
    h.add('A', 'B').add('B', 'C').add('C', 'D').add('B', 'G', 1).add('C', 'B', 1);
    h.add('D', 'U').add('U', 'D', 1);
    h.add('E', 'F').add('F', 'G').add('G', 'H').add('G', 'F', 2);

    ss << "graph h   : " << h << '\n';
    ss << "deep-first    : " << dfcyclesschedule(h)
       << ", cost: " << schedulingcost(h, dfcyclesschedule(h)) << '\n';
    ss << "breadth-first : " << bfcyclesschedule(h)
       << ", cost: " << schedulingcost(h, bfcyclesschedule(h)) << '\n';
}

std::string res15()
{
    return "graph h   : Graph {A-set{0}->B, B-set{0}->C, B-set{1}->G, C-set{1}->B, C-set{0}->D, "
           "D-set{0}->U, "
           "E-set{0}->F, F-set{0}->G, G-set{2}->F, G-set{0}->H, H, U-set{1}->D}\n"
           "deep-first    : Schedule {1:U, 2:D, 3:H, 4:G, 5:F, 6:C, 7:B, 8:A, 9:E}, cost: 19\n"
           "breadth-first : Schedule {1:U, 2:D, 3:H, 4:G, 5:F, 6:C, 7:B, 8:E, 9:A}, cost: 19\n";
}

bool check15()
{
    std::stringstream ss;
    test15(ss);
    bool ok = (0 == ss.str().compare(res15()));
    if (ok) {
        std::cout << "test15 OK " << '\n';
    } else {
        std::cout << "test15 FAIL " << '\n';
        std::cout << "We got     " << ss.str() << '\n';
        std::cout << "instead of " << res15() << '\n';
    }
    return ok;
}

void test16(std::ostream& ss)
{
    digraph<char> g;
    g.add('A', 'B', 0).add('A', 'B', 1).add('A', 'B', 2);

    const std::set<int>& w = g.weights('A', 'B');

    ss << "w = " << w << '\n';
}

std::string res16()
{
    return "w = set{0, 1, 2}\n";
}

bool check16()
{
    std::stringstream ss;
    test16(ss);
    bool ok = (0 == ss.str().compare(res16()));
    if (ok) {
        std::cout << "test16 OK " << '\n';
    } else {
        std::cout << "test16 FAIL " << '\n';
        std::cout << "We got     " << ss.str() << '\n';
        std::cout << "instead of " << res16() << '\n';
    }
    return ok;
}

void test17(std::ostream& ss)
{
    digraph<char> g;
    g.add('A', 'B')
        .add('C', 'A')
        .add('D', 'B')
        .add('D', 'C')
        .add('E', 'D')
        .add('E', 'F')
        .add('F', 'G')
        .add('H', 'G')
        .add('H', 'E');

    ss << "topology of g = " << topology(g) << '\n';
}

std::string res17()
{
    return "topology of g = std::vector{8, 9, 0, 2, 2, 1, 1, 1, 1}\n";
}

bool check17()
{
    std::stringstream ss;
    test17(ss);
    bool ok = (0 == ss.str().compare(res17()));
    if (ok) {
        std::cout << "test17 OK " << '\n';
    } else {
        std::cout << "test17 FAIL " << '\n';
        std::cout << "We got     " << ss.str() << '\n';
        std::cout << "instead of " << res17() << '\n';
    }
    return ok;
}

void test18(std::ostream& ss)
{
    digraph<char> g;
    g.add('A', 'B')
        .add('B', 'C', 1)
        .add('C', 'A')
        .add('D', 'B')
        .add('D', 'C')
        .add('D', 'E', 1)
        .add('E', 'D')
        .add('E', 'F')
        .add('F', 'G')
        .add('G', 'F', 1)
        .add('H', 'G')
        .add('H', 'E')
        .add('H', 'H', 1);

    ss << "topology of g = " << topology(g) << '\n';
}

std::string res18()
{
    return "topology of g = std::vector{8, 13, 4, 2, 1, 1}\n";
}

bool check18()
{
    std::stringstream ss;
    test18(ss);
    bool ok = (0 == ss.str().compare(res18()));
    if (ok) {
        std::cout << "test18 OK " << '\n';
    } else {
        std::cout << "test18 FAIL " << '\n';
        std::cout << "We got     " << ss.str() << '\n';
        std::cout << "instead of " << res18() << '\n';
    }
    return ok;
}

void test19(std::ostream& ss)
{
    digraph<char> g;
    g.add('W', 'C').add('W', 'J').add('J', 'I').add('C', 'B').add('B', 'A');
    ss << "critical path of g = " << criticalpath(g, 'W') << '\n';
    ss << "superschedule of g = " << spschedule(g) << '\n';
}

std::string res19()
{
    return "critical path of g = std::vector{A, B, C, W}\n"
           "superschedule of g = Schedule {1:A, 2:I, 3:B, 4:J, 5:C, 6:W}\n";
}

bool check19()
{
    std::stringstream ss;
    test19(ss);
    bool ok = (0 == ss.str().compare(res19()));
    if (ok) {
        std::cout << "test19 OK " << '\n';
    } else {
        std::cout << "test19 FAIL " << '\n';
        std::cout << "We got     " << ss.str() << '\n';
        std::cout << "instead of " << res19() << '\n';
    }
    return ok;
}

void test20(std::ostream& ss)
{
    digraph<char> g;
    g.add('Z', 'Y')
        .add('Y', 'X')
        .add('Y', 'J')
        .add('X', 'W')
        .add('W', 'C')
        .add('W', 'J')
        .add('J', 'I')
        .add('C', 'B')
        .add('B', 'A')
        .add('Z', 'G')
        .add('G', 'E')
        .add('G', 'F');
    ss << "critical path of g = " << criticalpath(g, 'Z') << '\n';
    ss << "    deepfirst of g = " << dfschedule(g) << '\n';
    ss << "breadth first of g = " << bfschedule(g) << '\n';
    ss << "superschedule of g = " << spschedule(g) << '\n';
}

std::string res20()
{
    return "critical path of g = std::vector{A, B, C, W, X, Y, Z}\n"
           "    deepfirst of g = Schedule {1:E, 2:F, 3:G, 4:I, 5:J, 6:A, 7:B, 8:C, 9:W, 10:X, "
           "11:Y, "
           "12:Z}\n"
           "breadth first of g = Schedule {1:A, 2:E, 3:F, 4:I, 5:B, 6:G, 7:J, 8:C, 9:W, 10:X, "
           "11:Y, "
           "12:Z}\n"
           "superschedule of g = Schedule {1:A, 2:I, 3:B, 4:J, 5:C, 6:W, 7:X, 8:F, 9:E, 10:Y, "
           "11:G, "
           "12:Z}\n";
}

bool check20()
{
    std::stringstream ss;
    test20(ss);
    bool ok = (0 == ss.str().compare(res20()));
    if (ok) {
        std::cout << "test20 OK " << '\n';
    } else {
        std::cout << "test20 FAIL " << '\n';
        std::cout << "We got     " << ss.str() << '\n';
        std::cout << "instead of " << res20() << '\n';
    }
    return ok;
}
