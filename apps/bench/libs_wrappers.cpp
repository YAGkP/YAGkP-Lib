#include "libs_wrappers.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "kaHIP_interface.h"
#include "metis.h"
#include "scotch.h"
#include "yagkp/config.hpp"

namespace yagkp::benchmark {

	namespace {
		// Копия массива в типе, который ждёт библиотека
		template <typename T> std::vector<T> As(const std::vector<int_t>& v) {
			return std::vector<T>(v.begin(), v.end());
		}

		// Указатель на данные или nullptr для пустого массива (веса рёбер не заданы)
		template <typename T> T* DataOrNull(std::vector<T>& v) {
			return v.empty() ? nullptr : v.data();
		}

		static_assert(sizeof(idx_t) == 8, "METIS должен быть собран с IDXTYPEWIDTH 64");
		static_assert(sizeof(kahip_idx) == 8, "KaHIP должен быть собран с 64BITMODE");
		static_assert(sizeof(SCOTCH_Num) == 8, "SCOTCH должен быть собран с INTSIZE=64");
	} // namespace

	std::vector<int_t> RunMETIS(
	    const int_t nparts,
	    const std::vector<int_t>& xadj,
	    const std::vector<int_t>& adjncy,
	    const std::vector<int_t>& vwgt,
	    const std::vector<int_t>& adjwgt,
	    const std::string& mode
	) {
		idx_t n        = static_cast<idx_t>(vwgt.size());
		idx_t ncon     = 1;
		idx_t parts    = static_cast<idx_t>(nparts);
		idx_t edge_cut = 0;

		std::vector<idx_t> _xadj   = As<idx_t>(xadj);
		std::vector<idx_t> _adjncy = As<idx_t>(adjncy);
		std::vector<idx_t> _vwgt   = As<idx_t>(vwgt);
		std::vector<idx_t> _adjwgt = As<idx_t>(adjwgt);
		std::vector<idx_t> partition(n);

		idx_t options[METIS_NOPTIONS];
		METIS_SetDefaultOptions(options);
		options[METIS_OPTION_UFACTOR] =
		    std::max<idx_t>(1, static_cast<idx_t>(std::lround(ProgramConfig::imbalance * 1000.0)));

		auto partition_graph =
		    mode == "kway" ? METIS_PartGraphKway
		    : mode == "recursive"
		        ? METIS_PartGraphRecursive
		        : throw std::logic_error("Incorrect mode");
		partition_graph(
		    &n,
		    &ncon,
		    _xadj.data(),
		    _adjncy.data(),
		    _vwgt.data(),
		    nullptr,
		    DataOrNull(_adjwgt),
		    &parts,
		    nullptr,
		    nullptr,
		    options,
		    &edge_cut,
		    partition.data()
		);

		return std::vector<int_t>(partition.begin(), partition.end());
	}

	std::vector<int_t> RunKaHIP(
	    const int_t nparts,
	    const std::vector<int_t>& xadj,
	    const std::vector<int_t>& adjncy,
	    const std::vector<int_t>& vwgt,
	    const std::vector<int_t>& adjwgt,
	    const std::string& mode
	) {
		int n            = static_cast<int>(vwgt.size());
		int parts        = static_cast<int>(nparts);
		kahip_idx edge_cut = 0;
		double imbalance = ProgramConfig::imbalance;

		std::vector<kahip_idx> _xadj   = As<kahip_idx>(xadj);
		std::vector<kahip_idx> _adjncy = As<kahip_idx>(adjncy);
		std::vector<int> _vwgt         = As<int>(vwgt);
		std::vector<kahip_idx> _adjwgt = As<kahip_idx>(adjwgt);
		std::vector<int> partition(n);

		int kahip_mode;
		if (mode == "fast") {
			kahip_mode = FAST;
		}
		else if (mode == "eco") {
			kahip_mode = ECO;
		}
		else if (mode == "strong") {
			kahip_mode = STRONG;
		}
		else {
			throw std::logic_error("Incorrect mode");
		}

		kaffpa(
		    &n,
		    _vwgt.data(),
		    _xadj.data(),
		    DataOrNull(_adjwgt),
		    _adjncy.data(),
		    &parts,
		    &imbalance,
		    true,
		    0,
		    kahip_mode,
		    &edge_cut,
		    partition.data()
		);

		return std::vector<int_t>(partition.begin(), partition.end());
	}

	std::vector<int_t> RunSCOTCH(
	    const int_t nparts,
	    const std::vector<int_t>& xadj,
	    const std::vector<int_t>& adjncy,
	    const std::vector<int_t>& vwgt,
	    const std::vector<int_t>& adjwgt
	) {
		const SCOTCH_Num n = static_cast<SCOTCH_Num>(vwgt.size());

		std::vector<SCOTCH_Num> _xadj   = As<SCOTCH_Num>(xadj);
		std::vector<SCOTCH_Num> _adjncy = As<SCOTCH_Num>(adjncy);
		std::vector<SCOTCH_Num> _vwgt   = As<SCOTCH_Num>(vwgt);
		std::vector<SCOTCH_Num> _adjwgt = As<SCOTCH_Num>(adjwgt);
		std::vector<SCOTCH_Num> partition(n);

		SCOTCH_Graph graph;
		SCOTCH_graphInit(&graph);
		SCOTCH_graphBuild(
		    &graph,
		    0,
		    n,
		    _xadj.data(),
		    _xadj.data() + 1,
		    _vwgt.data(),
		    nullptr,
		    static_cast<SCOTCH_Num>(_adjncy.size()),
		    _adjncy.data(),
		    DataOrNull(_adjwgt)
		);

		SCOTCH_Strat strat;
		SCOTCH_stratInit(&strat);
		SCOTCH_stratGraphMapBuild(
		    &strat,
		    SCOTCH_STRATDEFAULT,
		    static_cast<SCOTCH_Num>(nparts),
		    ProgramConfig::imbalance
		);
		SCOTCH_graphPart(&graph, static_cast<SCOTCH_Num>(nparts), &strat, partition.data());

		SCOTCH_stratExit(&strat);
		SCOTCH_graphExit(&graph);

		return std::vector<int_t>(partition.begin(), partition.end());
	}

} // namespace yagkp::benchmark
