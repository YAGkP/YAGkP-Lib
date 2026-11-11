// =============================================================================
// yagkp_partition: разбиение графа только средствами YAGkP, без сторонних
// библиотек. Используется в режимах debug / release / profile.
//
//   ./yagkp_partition --graph data/add20.mtx --k 4
//   ./yagkp_partition --graph data/add20.mtx --k 4 --imbalance 0.01 --output add20.part.4
// =============================================================================

#include <chrono>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

#include "yagkp/config.hpp"
#include "yagkp/graph.hpp"
#include "yagkp/metrics.hpp"
#include "yagkp/partitioner.hpp"
#include "yagkp/types.hpp"
#include "yagkp/parallel.hpp"

namespace {

	struct Options {
		std::string graph_path;
		std::string output_path; // пусто: разбиение не сохраняется
		yagkp::int_t k        = 0;
		yagkp::fp_t imbalance = yagkp::ProgramConfig::imbalance;
	};

	void PrintUsage(std::ostream& out) {
		out << "Usage: yagkp_partition --graph <file.mtx> --k <parts> [options]\n"
		       "\n"
		       "Options:\n"
		       "  --imbalance <eps>  допустимый дисбаланс (по умолчанию "
		    << yagkp::ProgramConfig::imbalance
		    << ")\n"
		       "  --output <file>    сохранить разбиение: номер части для каждой вершины,\n"
		       "                     по одному на строку (формат METIS)\n"
		       "  -h, --help         показать эту справку\n";
	}

	// Возвращает false, если аргументы неверны (сообщение уже выведено)
	bool ParseArgs(int argc, char* argv[], Options& opts, bool& show_help) {
		for (int i = 1; i < argc; ++i) {
			const std::string arg = argv[i];

			if (arg == "-h" || arg == "--help") {
				show_help = true;
				return true;
			}

			if (i + 1 >= argc) {
				std::cerr << "Ошибка: у аргумента " << arg << " нет значения\n";
				return false;
			}
			const std::string value = argv[++i];

			if (arg == "--graph") {
				opts.graph_path = value;
			}
			else if (arg == "--k") {
				opts.k = std::stoll(value);
			}
			else if (arg == "--imbalance") {
				opts.imbalance = std::stod(value);
			}
			else if (arg == "--output") {
				opts.output_path = value;
			}
			else {
				std::cerr << "Ошибка: неизвестный аргумент " << arg << "\n";
				return false;
			}
		}

		if (opts.graph_path.empty() || opts.k < 1) {
			std::cerr << "Ошибка: обязательны --graph и --k (k >= 1)\n";
			return false;
		}
		if (opts.imbalance < 0.0) {
			std::cerr << "Ошибка: --imbalance должен быть >= 0\n";
			return false;
		}
		if (!std::filesystem::exists(opts.graph_path)) {
			std::cerr << "Ошибка: файл не найден: " << opts.graph_path << "\n";
			return false;
		}
		return true;
	}

	void SavePartition(const std::string& path, const yagkp::Partition& partition) {
		std::ofstream out(path);
		if (!out) {
			throw std::runtime_error("не удалось открыть для записи: " + path);
		}
		for (yagkp::int_t part: partition) {
			out << part << '\n';
		}
	}

} // namespace

int main(int argc, char* argv[]) {
	using namespace yagkp;

	Options opts;
	bool show_help = false;

	try {
		if (!ParseArgs(argc, argv, opts, show_help)) {
			PrintUsage(std::cerr);
			return 1;
		}
	} catch (const std::exception&) { // std::stoll / std::stod
		std::cerr << "Ошибка: неверное числовое значение аргумента\n";
		PrintUsage(std::cerr);
		return 1;
	}

	if (show_help) {
		PrintUsage(std::cout);
		return 0;
	}

	ProgramConfig::imbalance = opts.imbalance;

	try {
		const UndirectedGraph graph(opts.graph_path, "mtx", true);

		const auto start          = std::chrono::steady_clock::now();
		const Partition partition = Partitioner::GetGraphKPartition(graph, opts.k);
		const auto end            = std::chrono::steady_clock::now();

		const auto time_ms =
		    std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

		std::cout << std::fixed << std::setprecision(2) << "graph      " << opts.graph_path << "\n"
		          << "n, m       " << graph.n << ", " << graph.m << "\n"
		          << "k          " << opts.k << "\n"
		          << "eps        " << opts.imbalance * 100.0 << "% (допустимый дисбаланс)\n"
				  << "threads    " << parallel::MaxThreads() << "\n"
		          << "time       " << time_ms << " ms\n"
		          << "edge cut   " << PartitionMetrics::GetEdgeCut(graph, partition) << "\n"
		          << "imbalance  "
		          << PartitionMetrics::GetImbalance(graph, opts.k, partition) * 100.0 << "%\n"
		          << "max part   " << PartitionMetrics::GetMaxPartWeight(graph, opts.k, partition)
		          << "\n";

		if (!opts.output_path.empty()) {
			SavePartition(opts.output_path, partition);
			std::cout << "saved to   " << opts.output_path << "\n";
		}
	} catch (const std::exception& e) {
		std::cerr << "Ошибка: " << e.what() << "\n";
		return 1;
	} catch (const char* e) { // external/mmio бросает строки
		std::cerr << "Ошибка: " << e << "\n";
		return 1;
	}

	return 0;
}