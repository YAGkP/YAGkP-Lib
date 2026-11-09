#pragma once

#include <string>

namespace yagkp::benchmark {

	// Заголовок строк, которые RunSingle печатает с csv = true
	inline constexpr const char* kCsvHeader =
	    "graph,n,m,k,eps,solver,threads,time_ms,edge_cut,imbalance,max_part";

	// Один запуск алгоритма algo на графе. Результат печатается в stdout:
	// строкой таблицы для человека или, с csv = true, одной строкой CSV
	// (поля — kCsvHeader)
	void RunSingle(const std::string& graph_path, int k, const std::string& algo, bool csv);

} // namespace yagkp::benchmark