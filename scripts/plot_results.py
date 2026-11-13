"""Plot hit ratio vs cache capacity for each workload and eviction policy.

Usage (from the project root, after running the simulator):
    python scripts/plot_results.py                      # reads results/results.csv
    python scripts/plot_results.py results.csv out.png  # custom paths

Needs only matplotlib:  pip install matplotlib
"""

import csv
import sys
from collections import defaultdict

import matplotlib.pyplot as plt

CSV_PATH = sys.argv[1] if len(sys.argv) > 1 else "results/results.csv"
OUT_PATH = sys.argv[2] if len(sys.argv) > 2 else "docs/hit_ratio.png"

WORKLOADS = ["sequential", "random", "repeated"]
# Fixed colour + marker + line style per policy, so lines are distinguishable
# even without colour (colour-blind readers, black-and-white print).
POLICIES = {
    "LRU":    {"color": "#2a78d6", "marker": "o", "linestyle": "-"},
    "FIFO":   {"color": "#eb6834", "marker": "s", "linestyle": "--"},
    "Random": {"color": "#1baf7a", "marker": "^", "linestyle": ":"},
}
TEXT = "#0b0b0b"
MUTED = "#52514e"
GRID = "#e4e3df"

# data[workload][policy] -> list of (capacity, hit_ratio %)
data = defaultdict(lambda: defaultdict(list))
with open(CSV_PATH, newline="") as f:
    for row in csv.DictReader(f):
        data[row["workload"]][row["policy"]].append(
            (int(row["capacity"]), 100 * float(row["hit_ratio"]))
        )

fig, axes = plt.subplots(1, 3, figsize=(13, 4.2), sharey=True)
fig.patch.set_facecolor("#fcfcfb")

for ax, workload in zip(axes, WORKLOADS):
    ax.set_facecolor("#fcfcfb")
    for policy, style in POLICIES.items():
        points = sorted(data[workload][policy])
        if not points:
            continue
        xs, ys = zip(*points)
        ax.plot(xs, ys, label=policy, linewidth=2, markersize=6, **style)

    ax.set_title(f"{workload.capitalize()} access", color=TEXT, fontsize=12, loc="left")
    ax.set_xlabel("Cache capacity (entries)", color=MUTED)
    ax.set_ylim(-3, 103)
    ax.grid(True, color=GRID, linewidth=0.8)
    ax.tick_params(colors=MUTED)
    for side in ("top", "right"):
        ax.spines[side].set_visible(False)
    for side in ("left", "bottom"):
        ax.spines[side].set_color(GRID)

axes[0].set_ylabel("Hit ratio (%)", color=MUTED)
axes[0].legend(frameon=False, loc="upper left")
fig.suptitle("Hit ratio vs cache capacity (1,000 keys, 100,000 accesses)",
             color=TEXT, fontsize=13, x=0.01, ha="left")
fig.tight_layout()
fig.savefig(OUT_PATH, dpi=150)
print(f"Saved {OUT_PATH}")