#pragma once

#include <string>
#include <vector>

#include "yagkp/types.hpp"

namespace yagkp::benchmark {

	// Обёртки над сторонними решателями. Граф — в формате CSR, как в Graph:
	// xadj, adjncy, веса вершин vwgt и веса рёбер adjwgt (той же длины, что
	// adjncy). Пустой adjwgt — граф без весов рёбер (все веса равны 1).
	// Библиотеки собраны в 64-битном режиме (scripts/deps/*.sh)

	std::vector<int_t> RunMETIS(
	    int_t nparts,
	    const std::vector<int_t>& xadj,
	    const std::vector<int_t>& adjncy,
	    const std::vector<int_t>& vwgt,
	    const std::vector<int_t>& adjwgt,
	    const std::string& mode
	);

	std::vector<int_t> RunKaHIP(
	    int_t nparts,
	    const std::vector<int_t>& xadj,
	    const std::vector<int_t>& adjncy,
	    const std::vector<int_t>& vwgt,
	    const std::vector<int_t>& adjwgt,
	    const std::string& mode
	);

	std::vector<int_t> RunSCOTCH(
	    int_t nparts,
	    const std::vector<int_t>& xadj,
	    const std::vector<int_t>& adjncy,
	    const std::vector<int_t>& vwgt,
	    const std::vector<int_t>& adjwgt
	);

} // namespace yagkp::benchmark
