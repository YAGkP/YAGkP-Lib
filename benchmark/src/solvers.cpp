#include "solvers.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "kaHIP_interface.h"
#include "kaminpar.h"
#include "metis.h"
#include "mtmetis.h"
#include "scotch.h"

namespace bench {

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
		static_assert(
		    sizeof(mtmetis_vtx_type) == 8 && sizeof(mtmetis_adj_type) == 8 &&
		        sizeof(mtmetis_wgt_type) == 8,
		    "mt-metis must be built with 64-bit types (scripts/deps/mtmetis.sh)"
		);
		static_assert(
		    sizeof(kaminpar::shm::NodeID) == 8 && sizeof(kaminpar::shm::EdgeID) == 8 &&
		        sizeof(kaminpar::shm::NodeWeight) == 8,
		    "KaMinPar must be built with 64-bit IDs and weights (scripts/deps/kaminpar.sh)"
		);
	} // namespace

	std::vector<int_t> RunMETIS(
	    const int_t nparts,
	    const double imbalance,
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
		    std::max<idx_t>(1, static_cast<idx_t>(std::lround(imbalance * 1000.0)));

		auto partition_graph = mode == "kway"        ? METIS_PartGraphKway
		                       : mode == "recursive" ? METIS_PartGraphRecursive
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
	    const double imbalance,
	    const std::vector<int_t>& xadj,
	    const std::vector<int_t>& adjncy,
	    const std::vector<int_t>& vwgt,
	    const std::vector<int_t>& adjwgt,
	    const std::string& mode
	) {
		int n                  = static_cast<int>(vwgt.size());
		int parts              = static_cast<int>(nparts);
		kahip_idx edge_cut     = 0;
		double kahip_imbalance = imbalance;

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
		    &kahip_imbalance,
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
	    const double imbalance,
	    const std::vector<int_t>& xadj,
	    const std::vector<int_t>& adjncy,
	    const std::vector<int_t>& vwgt,
	    const std::vector<int_t>& adjwgt,
	    const int threads
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

		// The context sets the number of threads; the graph bound to it is
		// partitioned with these threads
		SCOTCH_Context context;
		SCOTCH_contextInit(&context);
		SCOTCH_contextRandomClone(&context);
		if (SCOTCH_contextThreadSpawn(&context, threads, nullptr) != 0) {
			throw std::runtime_error("SCOTCH: cannot start threads");
		}
		SCOTCH_Graph context_graph;
		SCOTCH_graphInit(&context_graph);
		SCOTCH_contextBindGraph(&context, &graph, &context_graph);

		SCOTCH_Strat strat;
		SCOTCH_stratInit(&strat);
		SCOTCH_stratGraphMapBuild(
		    &strat,
		    SCOTCH_STRATDEFAULT,
		    static_cast<SCOTCH_Num>(nparts),
		    imbalance
		);
		SCOTCH_graphPart(&context_graph, static_cast<SCOTCH_Num>(nparts), &strat, partition.data());

		SCOTCH_stratExit(&strat);
		SCOTCH_graphExit(&context_graph);
		SCOTCH_graphExit(&graph);
		SCOTCH_contextExit(&context);

		return std::vector<int_t>(partition.begin(), partition.end());
	}

	std::vector<int_t> RunMtMETIS(
	    const int_t nparts,
	    const double imbalance,
	    const std::vector<int_t>& xadj,
	    const std::vector<int_t>& adjncy,
	    const std::vector<int_t>& vwgt,
	    const std::vector<int_t>& adjwgt,
	    const std::string& mode,
	    const int threads
	) {
		const mtmetis_vtx_type n     = static_cast<mtmetis_vtx_type>(vwgt.size());
		const mtmetis_vtx_type ncon  = 1;
		const mtmetis_pid_type parts = static_cast<mtmetis_pid_type>(nparts);
		mtmetis_wgt_type edge_cut    = 0;

		std::vector<mtmetis_adj_type> _xadj   = As<mtmetis_adj_type>(xadj);
		std::vector<mtmetis_vtx_type> _adjncy = As<mtmetis_vtx_type>(adjncy);
		std::vector<mtmetis_wgt_type> _vwgt   = As<mtmetis_wgt_type>(vwgt);
		std::vector<mtmetis_wgt_type> _adjwgt = As<mtmetis_wgt_type>(adjwgt);
		std::vector<mtmetis_pid_type> partition(n);

		// Options left MTMETIS_VAL_OFF keep their default values. mt-metis
		// ignores ubvec and takes the allowed imbalance from UBFACTOR
		std::vector<double> options(MTMETIS_NOPTIONS, MTMETIS_VAL_OFF);
		options[MTMETIS_OPTION_NTHREADS]  = threads;
		options[MTMETIS_OPTION_UBFACTOR]  = 1.0 + imbalance;
		options[MTMETIS_OPTION_VERBOSITY] = MTMETIS_VERBOSITY_NONE;

		auto partition_graph = mode == "kway"        ? MTMETIS_PartGraphKway
		                       : mode == "recursive" ? MTMETIS_PartGraphRecursive
		                                             : throw std::logic_error("Incorrect mode");
		const int status     = partition_graph(
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
            options.data(),
            &edge_cut,
            partition.data()
        );
		if (status != MTMETIS_SUCCESS) {
			throw std::runtime_error("mt-metis: error " + std::to_string(status));
		}

		return std::vector<int_t>(partition.begin(), partition.end());
	}

	std::vector<int_t> RunKaMinPar(
	    const int_t nparts,
	    const double imbalance,
	    const std::vector<int_t>& xadj,
	    const std::vector<int_t>& adjncy,
	    const std::vector<int_t>& vwgt,
	    const std::vector<int_t>& adjwgt,
	    const std::string& preset,
	    const int threads
	) {
		using namespace kaminpar::shm;

		std::vector<EdgeID> _xadj       = As<EdgeID>(xadj);
		std::vector<NodeID> _adjncy     = As<NodeID>(adjncy);
		std::vector<NodeWeight> _vwgt   = As<NodeWeight>(vwgt);
		std::vector<EdgeWeight> _adjwgt = As<EdgeWeight>(adjwgt);
		std::vector<BlockID> partition(vwgt.size());

		kaminpar::KaMinPar solver(threads, create_context_by_preset_name(preset));
		solver.set_output_level(kaminpar::OutputLevel::QUIET);
		// The graph arrays are ours (copies), so KaMinPar may use them as is
		solver.borrow_and_mutate_graph(_xadj, _adjncy, _vwgt, _adjwgt);
		solver.compute_partition(static_cast<BlockID>(nparts), imbalance, partition);

		return std::vector<int_t>(partition.begin(), partition.end());
	}

} // namespace bench
