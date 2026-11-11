#include "yagkp/uncoarsening.hpp"

#include <array>
#include <stdexcept>

#include "yagkp/buckets.hpp"
#include "yagkp/config.hpp"
#include "yagkp/metrics.hpp"

namespace yagkp::Uncoarser {

	namespace {
		/**
		 * @brief Projects the partition of the coarsened graph onto the the previous coarse level.
		 *
		 * @param coarse_level coarse level data with mapping from finer to coarser vertices
		 * @param coarse_partition partition of the coarse graph
		 * @return partition of the previous coarsening level graph
		 */
		Bisection DirectMapping(
		    const CoarseLevel& coarse_level,
		    const Bisection& coarse_partition
		) {
			const int_t n = coarse_level.uncoarse_to_coarse.size();
			Bisection prev_partition(n);

			for (int_t i = 0; i < n; i++) {
				prev_partition[i] = coarse_partition[coarse_level.uncoarse_to_coarse[i]];
			}

			return prev_partition;
		}

		inline int_t GetGain(int_t V, const UndirectedGraph& graph, const Bisection& partition) {
			int_t gain = 0;
			for (auto [next_V, w]: graph[V]) {
				if (partition[V] != partition[next_V]) {
					gain += w;
				}
				else {
					gain -= w;
				}
			}
			return gain;
		}

		inline int_t Side(Part part) {
			return part == Part::First ? 0 : 1;
		}

		using GainQueues = array<BucketPriorityQueue, 2>;

		GainQueues GetGainQueues(
		    const UndirectedGraph& graph,
		    const Bisection& partition,
		    const int_t max_possible_gain
		) {
			GainQueues heaps{
			    BucketPriorityQueue(graph.n, -max_possible_gain, max_possible_gain),
			    BucketPriorityQueue(graph.n, -max_possible_gain, max_possible_gain)
			};
			for (int_t curr_V = 0; curr_V < graph.n; curr_V++) {
				heaps[Side(partition[curr_V])].insert(GetGain(curr_V, graph, partition), curr_V);
			}
			return heaps;
		}

		array<int_t, 2> GetPartWeights(const UndirectedGraph& graph, const Bisection& partition) {
			array<int_t, 2> weights = {0, 0};
			for (int_t curr_V = 0; curr_V < graph.n; curr_V++) {
				weights[Side(partition[curr_V])] += graph.vertex_weights[curr_V];
			}
			return weights;
		}

		inline void UpdateNearGains(
		    int_t V,
		    const UndirectedGraph& graph,
		    const Bisection& partition,
		    const vector<char>& blocked,
		    GainQueues& heaps
		) {
			for (auto [near_V, w]: graph[V]) {
				if (blocked[near_V]) {
					continue;
				}
				const int_t delta = partition[near_V] == partition[V] ? -2 * w : 2 * w;
				heaps[Side(partition[near_V])].add(delta, near_V);
			}
		}

		/**
		 * @brief Improves the partition of the graph with Kernighan-Lin and fixes its balance.
		 *
		 * @param graph graph to refine
		 * @param current_partition partition of this graph
		 * @param C1 restriction on the 1st part
		 * @param C2 restriction on the 2nd part
		 * @return improved partition of the graph
		 */
		Bisection KernighanLinBlocking(
		    const UndirectedGraph& graph,
		    Bisection current_partition,
		    const int_t C1,
		    const int_t C2
		) {
			const int_t n           = graph.n;
			const array<int_t, 2> C = {C1, C2};

			int_t max_possible_gain = 0;
			for (int_t curr_V = 0; curr_V < n; curr_V++) {
				int_t current_possible_gain = 0;
				for (auto [next_V, w]: graph[curr_V]) {
					current_possible_gain += w;
				}
				setmax(max_possible_gain, current_possible_gain);
			}

			array<int_t, 2> weights = GetPartWeights(graph, current_partition);
			if (weights[0] > C[0] || weights[1] > C[1]) {
				vector<char> blocked(n, false);
				GainQueues heaps = GetGainQueues(graph, current_partition, max_possible_gain);

				for (int_t from = 0; from < 2; from++) {
					const int_t to = 1 - from;
					while (weights[from] > C[from] && !heaps[from].empty()) {
						auto [gain, curr_V] = heaps[from].extract();
						blocked[curr_V]     = true;

						const int_t weight = graph.vertex_weights[curr_V];
						if (weights[to] + weight <= C[to]) {
							weights[from] -= weight;
							weights[to] += weight;

							current_partition[curr_V] = GetOtherPart(current_partition[curr_V]);
							UpdateNearGains(curr_V, graph, current_partition, blocked, heaps);
						}
					}
				}
			}

			int_t current_edgecut = PartitionMetrics::GetEdgeCut(graph, current_partition);

			for (int_t run_number = 0; run_number < ProgramConfig::uncoarsening_KernighanLin_runs;
			     run_number++) {

				weights = GetPartWeights(graph, current_partition);

				vector<char> blocked(n, false);
				GainQueues heaps = GetGainQueues(graph, current_partition, max_possible_gain);

				vector<int_t> gains;
				vector<int_t> vertices;

				int_t current_run_best_edgecut = current_edgecut;

				int_t waste_cnt = 0;

				while (waste_cnt < ProgramConfig::uncoarsening_KernighanLin_waste_limit) {

					int_t from;
					if (weights[0] > C[0]) {
						from = 0;
					}
					else if (weights[1] > C[1]) {
						from = 1;
					}
					else if (heaps[0].empty() && heaps[1].empty()) {
						break;
					}
					else if (heaps[0].empty()) {
						from = 1;
					}
					else if (heaps[1].empty()) {
						from = 0;
					}
					else {
						const auto [gain0, V0] = heaps[0].top();
						const auto [gain1, V1] = heaps[1].top();

						const bool fits0 = weights[1] + graph.vertex_weights[V0] <= C[1];
						const bool fits1 = weights[0] + graph.vertex_weights[V1] <= C[0];

						if (!fits0 && !fits1) {
							heaps[0].extract();
							heaps[1].extract();

							blocked[V0] = true;
							blocked[V1] = true;

							continue;
						}
						else if (!fits0) {
							from = 1;
						}
						else if (!fits1) {
							from = 0;
						}
						else if (gain0 != gain1) {
							from = gain0 > gain1 ? 0 : 1;
						}
						else {
							from = C[0] - weights[0] <= C[1] - weights[1] ? 0 : 1;
						}
					}

					if (heaps[from].empty()) {
						break;
					}

					const int_t to      = 1 - from;
					auto [gain, curr_V] = heaps[from].extract();

					const int_t weight = graph.vertex_weights[curr_V];
					if (weights[to] + weight > C[to]) {
						continue;
					}

					blocked[curr_V] = true;

					gains.push_back(gain);
					vertices.push_back(curr_V);

					current_edgecut -= gain;

					if (current_edgecut < current_run_best_edgecut) {
						current_run_best_edgecut = current_edgecut;
						waste_cnt                = 0;
					}
					else {
						waste_cnt++;
					}

					weights[from] -= weight;
					weights[to] += weight;
					current_partition[curr_V] = GetOtherPart(current_partition[curr_V]);

					UpdateNearGains(curr_V, graph, current_partition, blocked, heaps);
				}

				while (current_edgecut > current_run_best_edgecut) {
					int_t gain = gains.back();
					gains.pop_back();
					int_t curr_V = vertices.back();
					vertices.pop_back();

					current_edgecut += gain;
					current_partition[curr_V] = GetOtherPart(current_partition[curr_V]);
				}
			}

			return current_partition;
		}

		/**
		 * @brief
		 *
		 * @param previous_graph Projects the partition of the coarsened graph onto the the previous
		 * coarse level and does some improvements
		 * @param coarse_level coarse level data with mapping from finer to coarser vertices
		 * @param coarse_partition partition of the coarse graph
		 * @param C1 restriction on the 1st part
		 * @param C2 restriction on the 2nd part
		 * @return improved partition of the previous coarsening level graph
		 */
		Bisection KernighanLinBlocking(
		    const UndirectedGraph& previous_graph,
		    const CoarseLevel& coarse_level,
		    const Bisection& coarse_partition,
		    const int_t C1,
		    const int_t C2
		) {
			return KernighanLinBlocking(
			    previous_graph,
			    DirectMapping(coarse_level, coarse_partition),
			    C1,
			    C2
			);
		}
	} // namespace

	Bisection RestorePartition(
	    const UndirectedGraph& graph,
	    const vector<CoarseLevel>& levels,
	    Bisection partition,
	    const int_t C1,
	    const int_t C2
	) {
		using Method = ProgramConfig::UncoarseningMethod;

		switch (ProgramConfig::uncoarsening_method) {
		case Method::DirectMapping:
			for (int_t i = levels.size() - 1; i >= 0; i--) {
				partition = Uncoarser::DirectMapping(levels[i], partition);
			}
			return partition;

		case Method::KernighanLin:
			if (levels.empty()) {
				partition = Uncoarser::KernighanLinBlocking(graph, std::move(partition), C1, C2);
			}
			for (int_t i = levels.size() - 1; i >= 0; i--) {
				partition = Uncoarser::KernighanLinBlocking(
				    i > 0 ? levels[i - 1].coarsened_graph : graph,
				    levels[i],
				    partition,
				    C1,
				    C2
				);
			}
			return partition;
		}
		throw std::logic_error("Unknown uncoarsening method");
	}
} // namespace yagkp::Uncoarser
