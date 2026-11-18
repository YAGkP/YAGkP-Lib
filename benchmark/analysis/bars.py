#!/usr/bin/env python3
"""Столбчатые диаграммы по графам: разрез и время в зависимости от k.

    python3 analysis/bars.py results/<прогон> [baselines/<набор>/<машина> ...] [--eps 0.03]
                             [--threads 24]

Для каждого графа — страница с двумя диаграммами: сверху разрез, снизу время
(логарифмическая шкала). По оси x — k, внутри каждого k — столбец на каждый
решатель. Если решатель запускался с разным числом потоков, каждое число
потоков — отдельный столбец ("yagkp x8"). Все страницы — в bars.pdf в папке
прогона; с --threads — в bars_t<потоки>.pdf.
"""

import argparse
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
import numpy as np  # noqa: E402
from matplotlib.backends.backend_pdf import PdfPages  # noqa: E402

from common import load  # noqa: E402


def series_labels(df):
    """Столбцы: пары (решатель, потоки) и их подписи."""
    threads_per_solver = df.groupby("solver")["threads"].nunique()
    series = sorted(df[["solver", "threads"]].drop_duplicates().itertuples(index=False))
    labels = {
        (s, t): s if threads_per_solver[s] == 1 else f"{s} x{t}" for s, t in series
    }
    return series, labels


def plot_graph(pdf, g, series, labels, colors):
    ks = sorted(g["k"].unique())
    x = np.arange(len(ks))
    width = 0.8 / len(series)

    fig, (ax_cut, ax_time) = plt.subplots(2, 1, figsize=(11, 8), sharex=True)
    for i, key in enumerate(series):
        rows = g[(g["solver"] == key[0]) & (g["threads"] == key[1])].set_index("k")
        cut = [rows["edge_cut"].get(k, np.nan) for k in ks]
        # Время 0 мс (меньше разрешения таймера) на логарифмической шкале не
        # рисуется: показываем как 1 мс
        time = [max(rows["time_ms"].get(k, np.nan), 1) for k in ks]
        offset = (i - (len(series) - 1) / 2) * width
        ax_cut.bar(x + offset, cut, width, label=labels[key], color=colors[key])
        ax_time.bar(x + offset, time, width, label=labels[key], color=colors[key])

    first = g.iloc[0]
    fig.suptitle(f"{first['graph']}   (n = {first['n']:,}, m = {first['m']:,})")
    ax_cut.set_ylabel("разрез")
    ax_time.set_ylabel("время, мс")
    ax_time.set_yscale("log")
    ax_time.set_xticks(x, [str(k) for k in ks])
    ax_time.set_xlabel("k")
    for ax in (ax_cut, ax_time):
        ax.grid(axis="y", alpha=0.3)
    ax_cut.legend(ncol=min(len(series), 4), fontsize=8)
    fig.tight_layout()
    pdf.savefig(fig)
    plt.close(fig)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("run_dirs", nargs="+",
                        help="папки прогонов; графики сохраняются в первую")
    parser.add_argument("--eps", type=float, help="дисбаланс, если в данных их несколько")
    parser.add_argument("--threads", type=int, nargs="+",
                        help="только запуски с этим числом потоков, например 24")
    args = parser.parse_args()
    run_dir = Path(args.run_dirs[0])

    df = load(args.run_dirs, args.eps)
    if args.threads:
        # Comparison at a fixed number of threads
        df = df[df["threads"].isin(args.threads)]
        if df.empty:
            raise SystemExit(f"Нет запусков с числом потоков {args.threads}")
    series, labels = series_labels(df)
    palette = plt.get_cmap("tab10")
    colors = {key: palette(i % 10) for i, key in enumerate(series)}

    suffix = "_t" + "_".join(map(str, args.threads)) if args.threads else ""
    out = run_dir / f"bars{suffix}.pdf"
    with PdfPages(out) as pdf:
        # Графы — по возрастанию числа рёбер, как в таблице Уолшоу
        for _, g in sorted(df.groupby("graph"), key=lambda item: item[1]["m"].iloc[0]):
            plot_graph(pdf, g, series, labels, colors)
    print(f"Сохранено: {out} ({df['graph'].nunique()} стр.)")


if __name__ == "__main__":
    main()