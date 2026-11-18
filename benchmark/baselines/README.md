# Baselines

Saved results of METIS, KaHIP and SCOTCH, so that they need not be rerun for every new version of YAGkP.

One directory per graph set and machine: `baselines/<set>/<machine>/`, a copy of a run directory from `results/` (`results.csv`, `plan.sh`, `info.txt`). For example, a plan with `SOLVERS=(kahip_fast kahip_eco kahip_strong metis_kway metis_recursive scotch)`:

```bash
bash run_benchmark.sh others_walshaw
cp -r results/<date>_<time>_others_walshaw baselines/walshaw/<machine>
```

- **Cuts** depend only on the graphs, the library versions and the flags (`info.txt` has them), so they can be compared with runs on any machine.
- **Times** are comparable only with runs on the same machine (`host` and `cpu` in `info.txt`).
- Different imbalances can be kept in one `results.csv`; the analysis scripts then need `--eps`.
