// Один запуск одного решателя на одном графе с замером времени и метрик.
//
//   yagkp_bench --graph data/walshaw/add20.mtx --k 4 --algo metis_kway   # строка таблицы
//   yagkp_bench --graph data/walshaw/add20.mtx --k 4 --algo yagkp --csv  # строка CSV
//   yagkp_bench --csv-header                                              # заголовок CSV
//   yagkp_bench --algos                                                   # доступные решатели
//   yagkp_bench --threaded-algos                                          # многопоточные из них
//
// Threads of the multithreaded solvers: --threads <n>, or OMP_NUM_THREADS
// if it is not given, or 1. YAGkP takes the number from OMP_NUM_THREADS only
// (and runs in parallel only in the par build).
//
// Граф читается и метрики считаются средствами YAGkP, одинаково для всех
// решателей. Время — только работа решателя: чтение графа и подсчёт метрик в
// него не входят

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "solvers.hpp"
#include "yagkp/config.hpp"
#include "yagkp/graph.hpp"
#include "yagkp/metrics.hpp"
#include "yagkp/parallel.hpp"
#include "yagkp/partitioner.hpp"

namespace {
	using yagkp::int_t;

	constexpr const char* kCsvHeader =
	    "graph,n,m,k,eps,solver,threads,time_ms,edge_cut,imbalance,max_part";

	const std::vector<std::string> kAlgos = {
	    "yagkp",
	    "kahip_fast",
	    "kahip_eco",
	    "kahip_strong",
	    "metis_kway",
	    "metis_recursive",
	    "scotch",
	    "mtmetis_kway",
	    "mtmetis_recursive",
	    "kaminpar_fast",
	    "kaminpar_default",
	    "kaminpar_eco",
	};

	// Solvers that can run in several threads
	const std::vector<std::string> kThreadedAlgos = {
	    "yagkp",
	    "scotch",
	    "mtmetis_kway",
	    "mtmetis_recursive",
	    "kaminpar_fast",
	    "kaminpar_default",
	    "kaminpar_eco",
	};

	bool IsThreaded(const std::string& algo) {
		return std::find(kThreadedAlgos.begin(), kThreadedAlgos.end(), algo) !=
		       kThreadedAlgos.end();
	}

	// The default number of threads: OMP_NUM_THREADS, or 1
	int DefaultThreads() {
		const char* value = std::getenv("OMP_NUM_THREADS");
		return value != nullptr && std::atoi(value) > 0 ? std::atoi(value) : 1;
	}

	int Usage() {
		std::cerr << "Usage: yagkp_bench --graph <file.mtx> --k <k> --algo <algo>"
		             " [--imbalance <eps>] [--threads <n>] [--csv]\n"
		             "       yagkp_bench --csv-header\n"
		             "       yagkp_bench --algos\n"
		             "       yagkp_bench --threaded-algos\n"
		             "Algos:";
		for (const std::string& algo: kAlgos) {
			std::cerr << ' ' << algo;
		}
		std::cerr << '\n';
		return 1;
	}

	std::vector<int_t> Run(
	    const yagkp::UndirectedGraph& g,
	    const std::string& algo,
	    int_t k,
	    double eps,
	    int threads
	) {
		if (algo == "yagkp") {
			return yagkp::Partitioner::GetGraphKPartition(g, k);
		}
		if (algo == "kahip_fast" || algo == "kahip_eco" || algo == "kahip_strong") {
			return bench::RunKaHIP(
			    k,
			    eps,
			    g.xadj,
			    g.adjncy,
			    g.vertex_weights,
			    g.edge_weights,
			    algo.substr(6)
			);
		}
		if (algo == "metis_kway" || algo == "metis_recursive") {
			return bench::RunMETIS(
			    k,
			    eps,
			    g.xadj,
			    g.adjncy,
			    g.vertex_weights,
			    g.edge_weights,
			    algo.substr(6)
			);
		}
		if (algo == "scotch") {
			return bench::RunSCOTCH(
			    k,
			    eps,
			    g.xadj,
			    g.adjncy,
			    g.vertex_weights,
			    g.edge_weights,
			    threads
			);
		}
		if (algo == "mtmetis_kway" || algo == "mtmetis_recursive") {
			return bench::RunMtMETIS(
			    k,
			    eps,
			    g.xadj,
			    g.adjncy,
			    g.vertex_weights,
			    g.edge_weights,
			    algo.substr(8),
			    threads
			);
		}
		if (algo.starts_with("kaminpar_")) {
			return bench::RunKaMinPar(
			    k,
			    eps,
			    g.xadj,
			    g.adjncy,
			    g.vertex_weights,
			    g.edge_weights,
			    algo.substr(9),
			    threads
			);
		}
		throw std::invalid_argument("unknown algo: " + algo);
	}
} // namespace

int main(int argc, char* argv[]) {
	std::string graph_path;
	std::string algo;
	int_t k    = 0;
	double eps = 0.05;
	bool csv   = false;
	int threads = DefaultThreads();

	for (int i = 1; i < argc; i++) {
		const std::string arg = argv[i];
		if (arg == "--graph" && i + 1 < argc) {
			graph_path = argv[++i];
		}
		else if (arg == "--k" && i + 1 < argc) {
			k = std::stoll(argv[++i]);
		}
		else if (arg == "--algo" && i + 1 < argc) {
			algo = argv[++i];
		}
		else if (arg == "--imbalance" && i + 1 < argc) {
			eps = std::stod(argv[++i]);
		}
		else if (arg == "--threads" && i + 1 < argc) {
			threads = std::stoi(argv[++i]);
		}
		else if (arg == "--csv") {
			csv = true;
		}
		else if (arg == "--csv-header") {
			std::cout << kCsvHeader << std::endl;
			return 0;
		}
		else if (arg == "--algos") {
			for (const std::string& name: kAlgos) {
				std::cout << name << '\n';
			}
			return 0;
		}
		else if (arg == "--threaded-algos") {
			for (const std::string& name: kThreadedAlgos) {
				std::cout << name << '\n';
			}
			return 0;
		}
		else {
			return Usage();
		}
	}

	if (graph_path.empty() || k <= 0 || algo.empty() || threads <= 0) {
		return Usage();
	}

	try {
		const yagkp::UndirectedGraph g(graph_path, "mtx", true);
		yagkp::ProgramConfig::imbalance = eps;
		// The threads actually used: for YAGkP the number OpenMP sees,
		// for the single-threaded solvers 1
		const int_t used_threads = algo == "yagkp"    ? yagkp::parallel::MaxThreads()
		                           : IsThreaded(algo) ? threads
		                                              : 1;

		const auto start                   = std::chrono::steady_clock::now();
		const std::vector<int_t> partition = Run(g, algo, k, eps, threads);
		const auto end                     = std::chrono::steady_clock::now();
		const long long time_ms =
		    std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

		const int_t edge_cut = yagkp::PartitionMetrics::GetEdgeCut(g, partition);
		const double imb     = yagkp::PartitionMetrics::GetImbalance(g, k, partition);
		const int_t max_part = yagkp::PartitionMetrics::GetMaxPartWeight(g, k, partition);

		if (csv) {
			std::cout << std::filesystem::path(graph_path).stem().string() << ',' << g.n << ','
			          << g.m << ',' << k << ',' << eps << ',' << algo << ',' << used_threads << ','
			          << time_ms << ',' << edge_cut << ',' << std::setprecision(6) << imb << ','
			          << max_part << '\n';
		}
		else {
			const double opt = static_cast<double>(g.total_vertex_weight) / static_cast<double>(k);
			std::cout << std::left << std::fixed << std::setprecision(2) << std::setw(16) << algo
			          << std::setw(12) << time_ms << std::setw(14) << edge_cut << std::setw(12)
			          << imb * 100.0 << std::setw(14) << max_part << std::setw(14) << opt << "\n";
		}
	} catch (const std::invalid_argument& e) {
		std::cerr << "Error: " << e.what() << "\n";
		return Usage();
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << "\n";
		return 1;
	} catch (const char* message) { // так сообщает об ошибках чтение .mtx в YAGkP
		std::cerr << "Error: " << graph_path << ": " << message << "\n";
		return 1;
	}
	return 0;
}
