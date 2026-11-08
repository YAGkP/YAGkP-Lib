#include "benchmark.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <tuple>
#include <vector>

#include "libs_wrappers.hpp"
#include "yagkp/config.hpp"
#include "yagkp/graph.hpp"
#include "yagkp/metrics.hpp"
#include "yagkp/parallel.hpp"
#include "yagkp/partitioner.hpp"
#include "yagkp/types.hpp"

namespace yagkp::benchmark {

	void RunSingle(
	    const std::string& graph_path,
	    int k,
	    const std::string& algo,
	    const std::string& output_dir,
	    const bool csv
	) {
		std::string graph_name = std::filesystem::path(graph_path).filename().string();
		Graph g(graph_path, "mtx", true);

		auto run = [&](const std::string& name) -> std::tuple<long long, int_t, double, int_t> {
			std::vector<int_t> partition;

			auto start = std::chrono::steady_clock::now();

			if (name == "yagkp") {
				partition = Partitioner::GetGraphKPartition(g, k);
			}
			else if (name == "kahip_fast" || name == "kahip_eco" || name == "kahip_strong") {
				partition =
				    RunKaHIP(k, g.xadj, g.adjncy, g.vertex_weights, g.edge_weights, name.substr(6));
			}
			else if (name == "metis_kway" || name == "metis_recursive") {
				partition =
				    RunMETIS(k, g.xadj, g.adjncy, g.vertex_weights, g.edge_weights, name.substr(6));
			}
			else if (name == "scotch") {
				partition = RunSCOTCH(k, g.xadj, g.adjncy, g.vertex_weights, g.edge_weights);
			}
			else {
				throw std::runtime_error("Unknown algo: " + name);
			}

			auto end = std::chrono::steady_clock::now();
			long long time_ms =
			    std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

			return {
			    time_ms,
			    PartitionMetrics::GetEdgeCut(g, partition),
			    PartitionMetrics::GetImbalance(g, k, partition),
			    PartitionMetrics::GetMaxPartWeight(g, k, partition)
			};
		};

		auto [time_ms, edge_cut, imbalance, max_part] = run(algo);
		double opt = static_cast<double>(g.getSumOfVertexWeights()) / static_cast<double>(k);

		std::string stem = graph_name + "_k" + std::to_string(k);

		std::ostringstream pct;
		pct << std::fixed << std::setprecision(2) << imbalance * 100.0 << "%";

		// === CSV: одна строка, поля — kCsvHeader ===
		if (csv) {
			// Многопоточен только YAGkP; число потоков — то, что видит OpenMP
			const int_t threads = algo == "yagkp" ? parallel::MaxThreads() : 1;
			std::cout << std::filesystem::path(graph_path).stem().string() << ',' << g.n << ','
			          << g.m / 2 << ',' << k << ',' << ProgramConfig::imbalance << ',' << algo
			          << ',' << threads << ',' << time_ms << ',' << edge_cut << ','
			          << std::setprecision(6) << imbalance << ',' << max_part << '\n';
		}
		else {
			std::cout << std::left << std::fixed << std::setprecision(2) << std::setw(16) << algo
			          << std::setw(12) << time_ms << std::setw(14) << edge_cut << std::setw(12)
			          << pct.str() << std::setw(14) << max_part << std::setw(14) << opt << "\n";
		}

		if (output_dir.empty()) {
			return;
		}

		// === TXT ===
		std::filesystem::create_directories(output_dir);
		std::string txt_path = output_dir + "/" + stem + ".txt";
		bool txt_exists      = std::filesystem::exists(txt_path);

		std::ofstream txt(txt_path, std::ios::app);
		if (!txt_exists) {
			txt << "# Graph: " << graph_name << "  n=" << g.n << "  m=" << g.m << "  k=" << k << " "
			    << "imb=" << ProgramConfig::imbalance << "\n";
			txt << "# " << std::string(86, '-') << "\n";
			txt << "# " << std::left << std::setw(16) << "algo" << std::setw(12) << "time(ms)"
			    << std::setw(14) << "edge cut" << std::setw(12) << "imbalance" << std::setw(14)
			    << "max part" << std::setw(14) << "opt part"
			    << "\n";
			txt << "# " << std::string(86, '-') << "\n";
		}

		txt << std::left << std::fixed << std::setprecision(2) << std::setw(16) << algo
		    << std::setw(12) << time_ms << std::setw(14) << edge_cut << std::setw(12) << pct.str()
		    << std::setw(14) << max_part << std::setw(14) << opt << "\n";
	}
} // namespace yagkp::benchmark