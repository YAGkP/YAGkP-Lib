#include <iostream>
#include <string>

#include "benchmark.hpp"
#include "yagkp/config.hpp"

// ./yagkp_bench --graph ../data/add20.mtx --k 4 --algo kahip_strong      # строка таблицы
// ./yagkp_bench --graph ../data/add20.mtx --k 4 --algo yagkp --csv   # одна строка CSV
// ./yagkp_bench --csv-header                                          # заголовок CSV

int main(int argc, char* argv[]) {

	std::string graph_path;
	std::string algo;
	int k    = 0;
	bool csv = false;

	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];
		if (arg == "--graph" && i + 1 < argc) {
			graph_path = argv[++i];
		}
		else if (arg == "--k" && i + 1 < argc) {
			k = std::stoi(argv[++i]);
		}
		else if (arg == "--algo" && i + 1 < argc) {
			algo = argv[++i];
		}
		else if (arg == "--imbalance" && i + 1 < argc) {
			yagkp::ProgramConfig::imbalance = std::stod(argv[++i]);
		}
		else if (arg == "--csv") {
			csv = true;
		}
		else if (arg == "--csv-header") {
			std::cout << yagkp::benchmark::kCsvHeader << std::endl;
			return 0;
		}
	}

	if (graph_path.empty() || k == 0 || algo.empty()) {
		std::cerr << "Usage: yagkp_bench --graph <path> --k <k> --algo <algo>"
		             " [--csv] [--imbalance <eps>]"
		          << std::endl;
		std::cerr << "       yagkp_bench --csv-header" << std::endl;
		std::cerr << "Algos: yagkp, kahip_strong, kahip_eco, kahip_fast, metis_kway, "
		             "metis_recursive, scotch"
		          << std::endl;
		return 1;
	}

	yagkp::benchmark::RunSingle(graph_path, k, algo, csv);

	return 0;
}