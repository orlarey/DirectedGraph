// Dump pour les illustrations du combine (branche combine-lab) :
// bank(4,6), la découpe en chaînes, les reconstructions sans/avec le
// terme iso, et l'empaquetage (cycle de chaque nœud, même algorithme
// que squality). Sortie texte, lue par figs/labviz.py.
#include <functional>
#include <iostream>
#include "DirectedGraph/DirectedGraph.hh"
#include "DirectedGraph/DirectedGraphAlgorythm.hh"

static long bankshape(const int& n) { return n % 1000; }

static void dumpseq(const char* tag, const digraph<int>& g, const std::vector<int>& s,
                    unsigned U)
{
    // même empaquetage que squality : cycle par nœud
    digraph<int> rg = reverse(g);
    std::map<int, int> cyc;
    int cur = 0, slots = 0;
    std::cout << tag;
    for (size_t i = 0; i < s.size(); i++) {
        int n = s[i], lo = 0;
        for (const auto& d : g.destinations(n)) {
            auto it = cyc.find(d.first);
            if (it != cyc.end()) lo = std::max(lo, it->second + 1);
        }
        if (lo > cur) { cur = lo; slots = 0; }
        else if (slots == int(U)) { cur++; slots = 0; }
        cyc[n] = cur; slots++;
        std::cout << ' ' << n << ':' << cur;
    }
    std::cout << '\n';
}

int main()
{
    const int k = 4, d = 6; const unsigned U = 4, R = 4;
    digraph<int> g; std::vector<int> opt;
    for (int c = 0; c < k; c++)
        for (int t = 1; t < d; t++) g.add(c * 1000 + t, c * 1000 + t - 1);
    for (int t = 0; t < d; t++)
        for (int c = 0; c < k; c++) opt.push_back(c * 1000 + t);
    digraph<int> rg = reverse(g);

    std::vector<std::vector<int>> chains;
    for (int c = 0; c < k; c++) {
        std::vector<int> p;
        for (int t = 0; t < d; t++) p.push_back(c * 1000 + t);
        chains.push_back(p);
    }
    std::function<long(const int&)> iso = bankshape;
    std::function<long(const int&)> noiso;

    // le combine par paire, deux chaînes (pour la figure du DP)
    dumpseq("PAIR_NOISO", g, dpcombine(g, rg, chains[0], chains[1], R, noiso), U);
    dumpseq("PAIR_ISO  ", g, dpcombine(g, rg, chains[0], chains[1], R, iso), U);

    // le pliage des 4 chaînes
    auto fold = [&](std::function<long(const int&)> sh) {
        std::vector<int> acc;
        for (const auto& p : chains) acc = dpcombine(g, rg, std::move(acc), p, R, sh);
        return acc;
    };
    dumpseq("OPT   ", g, opt, U);
    dumpseq("NOISO ", g, fold(noiso), U);
    dumpseq("ISO   ", g, fold(iso), U);
    return 0;
}
