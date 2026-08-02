# Illustrations du processus d'entrelacement (branche combine-lab),
# à partir du dump réel de labviz.cpp (labviz.txt).
import os
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle, FancyArrowPatch

HERE = os.path.dirname(os.path.abspath(__file__))
CHCOL = ["#4477cc", "#66aa55", "#cc7733", "#aa55aa"]

def parse(path):
    seqs = {}
    for line in open(path):
        parts = line.split()
        tag = parts[0]
        seq = [(int(x.split(':')[0]), int(x.split(':')[1])) for x in parts[1:]]
        seqs[tag] = seq
    return seqs

S = parse(os.path.join(HERE, "..", "labviz.txt"))
k, d, U = 4, 6, 4

def draw_grid(ax, seq, title):
    ncyc = max(c for _, c in seq) + 1
    used = {}
    for n, c in seq:
        used.setdefault(c, []).append(n)
    for c in range(ncyc):
        col = used.get(c, [])
        for s in range(U):
            filled = s < len(col)
            if filled:
                n = col[s]
                chain, stage = n // 1000, n % 1000
                fc = CHCOL[chain]
            else:
                fc = "#eeeeee"
            ax.add_patch(Rectangle((c, s), 0.92, 0.92, facecolor=fc,
                                   edgecolor="#888888", linewidth=0.6))
            if filled:
                ax.text(c + 0.46, s + 0.46, str(stage), ha="center", va="center",
                        color="white", fontsize=9, fontweight="bold")
    holes = ncyc * U - len(seq)
    ax.set_title(f"{title} — {ncyc} cycles, {holes} trous", fontsize=10.5)
    ax.set_xlim(-0.3, max(11, ncyc) + 0.3)
    ax.set_ylim(-0.4, U + 0.2)
    ax.set_xticks(range(ncyc), [str(i + 1) for i in range(ncyc)], fontsize=7)
    ax.set_yticks([])

# ---- figure 1 : le processus (pièces -> sans iso -> avec iso = optimum)
fig, axes = plt.subplots(4, 1, figsize=(9.5, 9.2))
ax = axes[0]
for c in range(k):
    for t in range(d):
        ax.add_patch(Rectangle((t + c * (d + 1), 0), 0.92, 0.92,
                               facecolor=CHCOL[c], edgecolor="#888888", linewidth=0.6))
        ax.text(t + c * (d + 1) + 0.46, 0.46, str(t), ha="center", va="center",
                color="white", fontsize=9, fontweight="bold")
ax.set_title("Les pièces : 4 chaînes isomorphes de 6 étapes "
             "(les sous-expressions du banc)", fontsize=10.5)
ax.set_xlim(-0.3, k * (d + 1) + 0.3)
ax.set_ylim(-0.4, 1.4)
ax.axis("off")
draw_grid(axes[1], S["OPT"], "L'optimum certifié (grille U=4 remplie, niveau-major)")
draw_grid(axes[2], S["NOISO"], "Pliage des 4 chaînes SANS terme iso "
                               "(pression + stalles seuls)")
draw_grid(axes[3], S["ISO"], "Pliage AVEC le terme d'adjacence isomorphe "
                             "(= l'optimum, retrouvé)")
fig.suptitle("L'entrelacement compositionnel sur bank(4,6) — données réelles de dpcombine",
             fontsize=12)
fig.tight_layout(rect=[0, 0, 1, 0.97])
fig.savefig(os.path.join(HERE, "combine-process.png"), dpi=200)
print("écrit figs/combine-process.png")

# ---- figure 2 : le chemin du DP pour une paire de chaînes
fig, ax = plt.subplots(figsize=(6.4, 6.0))
pair = S["PAIR_ISO"]
i = j = 0
path = [(0, 0)]
for n, _ in pair:
    if n < 1000:
        i += 1
    else:
        j += 1
    path.append((i, j))
for x in range(d + 1):
    ax.axhline(x, color="#dddddd", linewidth=0.7)
    ax.axvline(x, color="#dddddd", linewidth=0.7)
ax.plot([p[0] for p in path], [p[1] for p in path], color="#227744",
        linewidth=2.5, marker="o", markersize=4, label="chemin choisi (tressage)")
ax.plot([0, d, d], [0, 0, d], color="#999999", linewidth=1.8, linestyle="--",
        label="concaténation (df) : A puis B")
ax.set_xlabel("position dans A (chaîne bleue)", fontsize=10)
ax.set_ylabel("position dans B (chaîne verte)", fontsize=10)
ax.set_title("Le combine est un chemin dans la grille |A| × |B|\n"
             "chaque pas droit = avancer dans A, chaque pas haut = avancer dans B\n"
             "(l'escalier tue les stalles et crée l'adjacence isomorphe)", fontsize=10.5)
ax.legend(fontsize=9, loc="upper left")
ax.set_xlim(-0.4, d + 0.4)
ax.set_ylim(-0.4, d + 0.4)
ax.set_aspect("equal")
fig.tight_layout()
fig.savefig(os.path.join(HERE, "combine-dp-path.png"), dpi=200)
print("écrit figs/combine-dp-path.png")
