#pragma once

#include <string>
#include <vector>

#include "yagkp/types.hpp"

namespace bench {

	using yagkp::int_t;

	// Обёртки над сторонними решателями. Граф — в формате CSR, как в Graph:
	// xadj, adjncy, веса вершин vwgt и веса рёбер adjwgt (той же длины, что
	// adjncy). Пустой adjwgt — граф без весов рёбер (все веса равны 1).
	// imbalance — допустимый дисбаланс, например 0.03.
	// Библиотеки собраны в 64-битном режиме (scripts/deps/*.sh)

	std::vector<int_t> RunMETIS(
	    int_t nparts,
	    double imbalance,
	    const std::vector<int_t>& xadj,
	    const std::vector<int_t>& adjncy,
	    const std::vector<int_t>& vwgt,
	    const std::vector<int_t>& adjwgt,
	    const std::string& mode
	);

	std::vector<int_t> RunKaHIP(
	    int_t nparts,
	    double imbalance,
	    const std::vector<int_t>& xadj,
	    const std::vector<int_t>& adjncy,
	    const std::vector<int_t>& vwgt,
	    const std::vector<int_t>& adjwgt,
	    const std::string& mode
	);

	// Multithreaded solvers: threads is the number of threads to use.
	// SCOTCH with 1 thread is the sequential SCOTCH

	std::vector<int_t> RunSCOTCH(
	    int_t nparts,
	    double imbalance,
	    const std::vector<int_t>& xadj,
	    const std::vector<int_t>& adjncy,
	    const std::vector<int_t>& vwgt,
	    const std::vector<int_t>& adjwgt,
	    int threads
	);

	// mode: "kway" or "recursive"
	std::vector<int_t> RunMtMETIS(
	    int_t nparts,
	    double imbalance,
	    const std::vector<int_t>& xadj,
	    const std::vector<int_t>& adjncy,
	    const std::vector<int_t>& vwgt,
	    const std::vector<int_t>& adjwgt,
	    const std::string& mode,
	    int threads
	);

	// preset: a KaMinPar preset name, for example "default", "fast" or "eco"
	std::vector<int_t> RunKaMinPar(
	    int_t nparts,
	    double imbalance,
	    const std::vector<int_t>& xadj,
	    const std::vector<int_t>& adjncy,
	    const std::vector<int_t>& vwgt,
	    const std::vector<int_t>& adjwgt,
	    const std::string& preset,
	    int threads
	);

} // namespace bench
