#include "yagkp/partitioner.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include "yagkp/bipartitioner.hpp"
#include "yagkp/coarsening.hpp"
#include "yagkp/config.hpp"
#include "yagkp/metrics.hpp"
#include "yagkp/uncoarsening.hpp"

namespace yagkp::Partitioner {
	namespace {
		int_t GetHalfLimit(const fp_t W, const int_t M, const int_t part_k, const int_t k) {
			const fp_t levels       = std::ceil(std::log2(fp_t(k)));
			const fp_t level_factor = std::pow(fp_t(M) * fp_t(k) / W, 1.0 / levels);

			const int_t limit =
			    std::min(M * part_k, int_t(level_factor * W * fp_t(part_k) / fp_t(k)));
			return std::max(limit, int_t(std::ceil(W * fp_t(part_k) / fp_t(k))));
		}

		// graph: input graph to partition                      | graph
		// k: number of parts for this recursive call (k >= 1)  | 2
		// offset: starting index for parts in this call        | 2
		// returns: vector of part indices (offset..offset+k-1) | {2, 2, 3, 3}
		std::vector<int_t> RecursivePartition(
		    const Graph& graph,
		    const int_t k,
		    const int_t M,
		    int_t offset
		) {
			std::vector<int_t> partition(graph.n, offset);
			if (k == 1) {
				return partition;
			}

			int_t k1 = k / 2;
			int_t k2 = k - k1;
			std::vector<int_t> first_part_vertices, second_part_vertices;

			{

				std::vector<CoarseLevel> coarse_levels = Coarser::GetCoarseLevels(graph, k);
				const Graph& coarsened_graph =
				    coarse_levels.empty() ? graph : coarse_levels.back().coarsened_graph;

				const fp_t W   = fp_t(graph.getSumOfVertexWeights());
				const int_t C1 = GetHalfLimit(W, M, k1, k);
				const int_t C2 = GetHalfLimit(W, M, k2, k);

				std::vector<Part> initial_partition =
				    Bipartitioner::GetGraphBipartition(coarsened_graph, C1, C2);
				std::vector<Part> restored_partition =
				    Uncoarser::RestorePartition(graph, coarse_levels, initial_partition, C1, C2);

				for (int_t i = 0; i < graph.n; ++i) {
					if (restored_partition[i] == Part::First) {
						first_part_vertices.push_back(i);
					}
					else {
						second_part_vertices.push_back(i);
					}
				}
			}

			Graph first_graph  = graph.selectSubgraph(first_part_vertices);
			Graph second_graph = graph.selectSubgraph(second_part_vertices);

			std::vector<int_t> partition1;
			std::vector<int_t> partition2;

#pragma omp task default(none) shared(first_graph, partition1)                                     \
    firstprivate(k1, M, offset) if (first_graph.n >= ProgramConfig::parallel_task_min_vertices)
			partition1 = RecursivePartition(first_graph, k1, M, offset);

			partition2 = RecursivePartition(second_graph, k2, M, offset + k1);

#pragma omp taskwait

			for (int_t i = 0; i < (int_t)first_part_vertices.size(); i++) {
				partition[first_part_vertices[i]] = partition1[i];
			}
			for (int_t i = 0; i < (int_t)second_part_vertices.size(); i++) {
				partition[second_part_vertices[i]] = partition2[i];
			}

			return partition;
		}
	} // namespace

	std::vector<int_t> GetGraphKPartition(const Graph& graph, const int_t k) {

		const int_t W = graph.getSumOfVertexWeights();

		const int_t M = std::max(
		    int_t(fp_t(W) / fp_t(k) * (1.0 + ProgramConfig::imbalance) + ProgramConfig::EPS),
		    (W + k - 1) / k
		);

		std::vector<std::vector<int_t>> partitions(ProgramConfig::runs);

#pragma omp parallel
#pragma omp single
		for (int_t i = 0; i < ProgramConfig::runs; ++i) {
#pragma omp task default(none) shared(graph, partitions) firstprivate(i, k, M)
			partitions[i] = RecursivePartition(graph, k, M, 0);
		}

		int_t best_edgecut = std::numeric_limits<int_t>::max();
		std::vector<int_t> best_partition;

		for (std::vector<int_t>& partition: partitions) {
			int_t edgecut = PartitionMetrics::GetEdgeCut(graph, partition);

			if (edgecut < best_edgecut) {
				best_edgecut   = edgecut;
				best_partition = std::move(partition);
			}
		}

		return best_partition;
	}
} // namespace yagkp::Partitioner