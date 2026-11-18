#!/usr/bin/env python3
"""Столбчатые диаграммы по графам: время и ускорение по числу потоков.

    python3 analysis/scaling.py results/<прогон> [--eps 0.03]

Для каждого графа — страница с двумя диаграммами. По оси x — k, внутри
каждого k — столбец на каждое число потоков:
    сверху  время, мс (медиана по повторам, логарифмическая шкала);
    снизу   ускорение T(1) / T(t); черная черта над столбцом — линейное
            ускорение t, то есть каким столбец был бы в идеале.
Все страницы — в scaling_<решатель>.pdf в папке прогона.
"""

import argparse
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
import numpy as np  # noqa: E402
from matplotlib.backends.backend_pdf import PdfPages  # noqa: E402

from common import load  # noqa: E402


def plot_graph(pdf, g, threads, colors):
    ks = sorted(g["k"].unique())
    x = np.arange(len(ks))
    width = 0.8 / len(threads)

    fig, (ax_time, ax_speed) = plt.subplots(2, 1, figsize=(11, 8), sharex=True)
    for i, t in enumerate(threads):
        rows = g[g["threads"] == t].set_index("k")
        # Время 0 мс (меньше разрешения таймера) на логарифмической шкале не
        # рисуется: показываем как 1 мс
        time = [max(rows["time_ms"].get(k, np.nan), 1) for k in ks]
        speedup = [rows["speedup"].get(k, np.nan) for k in ks]
        offset = (i - (len(threads) - 1) / 2) * width
        ax_time.bar(x + offset, time, width, label=f"t = {t}", color=colors[t])
        ax_speed.bar(x + offset, speedup, width, label=f"t = {t}", color=colors[t])
        # Линейное ускорение: черта на высоте t над каждым столбцом
        ax_speed.hlines([t] * len(ks), x + offset - width / 2, x + offset + width / 2,
                        colors="black", linewidth=1.5,
                        label="линейное ускорение" if i == len(threads) - 1 else None)

    first = g.iloc[0]
    fig.suptitle(f"{first['graph']}   (n = {first['n']:,}, m = {first['m']:,})")
    ax_time.set_ylabel("время, мс")
    ax_time.set_yscale("log")
    ax_speed.set_ylabel("ускорение  T(1) / T(t)")
    ax_speed.set_xticks(x, [str(k) for k in ks])
    ax_speed.set_xlabel("k")
    for ax in (ax_time, ax_speed):
        ax.grid(axis="y", alpha=0.3)
    ax_time.legend(ncol=min(len(threads), 6), fontsize=8)
    ax_speed.legend(ncol=min(len(threads) + 1, 7), fontsize=8)
    fig.tight_layout()
    pdf.savefig(fig)
    plt.close(fig)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("run_dirs", nargs="+",
                        help="папки прогонов; графики сохраняются в первую")
    parser.add_argument("--eps", type=float, help="дисбаланс, если в данных их несколько")
    parser.add_argument("--solver", default="yagkp")
    args = parser.parse_args()
    run_dir = Path(args.run_dirs[0])

    df = load(args.run_dirs, args.eps)
    df = df[df["solver"] == args.solver].copy()
    if 1 not in set(df["threads"]):
        raise SystemExit("В прогоне нет запусков на 1 потоке: ускорение не от чего считать")
    base = df[df["threads"] == 1].set_index(["graph", "k"])["time_ms"]
    df = df.join(base, on=["graph", "k"], rsuffix="_1")
    df["speedup"] = df["time_ms_1"] / df["time_ms"]

    threads = sorted(df["threads"].unique())
    palette = plt.get_cmap("viridis")
    colors = {t: palette(i / max(len(threads) - 1, 1)) for i, t in enumerate(threads)}

    out = run_dir / f"scaling_{args.solver}.pdf"
    with PdfPages(out) as pdf:
        # Графы — по возрастанию числа рёбер, как в таблице Уолшоу
        for _, g in sorted(df.groupby("graph"), key=lambda item: item[1]["m"].iloc[0]):
            plot_graph(pdf, g, threads, colors)
    print(f"Сохранено: {out} ({df['graph'].nunique()} стр.)")


if __name__ == "__main__":
    main()