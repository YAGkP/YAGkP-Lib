"""Общее для скриптов обработки: чтение results.csv из папок прогонов."""

import sys
from pathlib import Path

import numpy as np
import pandas as pd


def load(run_dirs, eps=None):
    """results.csv одного или нескольких прогонов, например свежего прогона
    YAGkP и сохранённых результатов из baselines/. Повторы одного запуска
    сводятся в одну строку: время — медиана, разрез и дисбаланс — среднее
    (параллельная версия недетерминирована, разрез от повтора к повтору
    меняется). eps — оставить только запуски с этим дисбалансом; если он не
    задан, а в данных их несколько, это ошибка: графики их бы смешали."""
    frames = []
    for run_dir in run_dirs:
        path = Path(run_dir) / "results.csv"
        if not path.exists():
            sys.exit(f"Не найден {path}")
        frames.append(pd.read_csv(path))
    df = pd.concat(frames, ignore_index=True)
    if eps is not None:
        df = df[np.isclose(df["eps"], eps)]
        if df.empty:
            sys.exit(f"Нет запусков с eps = {eps}")
    elif df["eps"].nunique() > 1:
        sys.exit(f"В данных несколько значений eps: {sorted(df['eps'].unique())}; выберите одно: --eps")
    keys = ["graph", "n", "m", "k", "eps", "solver", "threads"]
    return (
        df.groupby(keys, as_index=False)
        .agg(time_ms=("time_ms", "median"),
             edge_cut=("edge_cut", "mean"),
             imbalance=("imbalance", "mean"),
             repeats=("repeat", "count"))
    )


def geomean(values):
    values = np.asarray(values, dtype=float)
    return float(np.exp(np.log(values).mean()))
