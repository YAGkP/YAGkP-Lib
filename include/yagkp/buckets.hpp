#pragma once

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

#include "yagkp/types.hpp"

namespace yagkp {
	class BucketPriorityQueue {
		struct Node {
			int_t prev = -1;
			int_t next = -1;
		};

		static constexpr int_t kMaxBuckets       = int_t{1} << 22;
		static constexpr int_t kBucketsPerVertex = 8;

		bool use_heap;

		std::vector<int_t> buckets;
		std::vector<Node> nodes;
		std::vector<int_t> gain_value;
		int_t min_gain;
		int_t max_gain;
		int_t n_nodes = 0;

		std::vector<int_t> heap;
		std::vector<int_t> pos;

		bool HeapLess(int_t i, int_t j) const {
			return gain_value[heap[i]] < gain_value[heap[j]];
		}

		void HeapSwap(int_t i, int_t j) {
			std::swap(heap[i], heap[j]);
			pos[heap[i]] = i;
			pos[heap[j]] = j;
		}

		void SiftUp(int_t i) {
			while (i > 0 && HeapLess((i - 1) / 2, i)) {
				HeapSwap(i, (i - 1) / 2);
				i = (i - 1) / 2;
			}
		}

		void SiftDown(int_t i) {
			const int_t size = static_cast<int_t>(heap.size());
			while (true) {
				int_t largest = i;
				for (int_t child = 2 * i + 1; child <= 2 * i + 2 && child < size; child++) {
					if (HeapLess(largest, child)) {
						largest = child;
					}
				}
				if (largest == i) {
					return;
				}
				HeapSwap(i, largest);
				i = largest;
			}
		}

	  public:
		// n - amount of vertices in the graph, [min_g, max_g] - gains range
		BucketPriorityQueue(int_t n, int_t min_g, int_t max_g)
		    : use_heap(max_g - min_g + 1 > std::max(kMaxBuckets, kBucketsPerVertex * n)),
		      buckets(use_heap ? 0 : max_g - min_g + 1, -1), nodes(use_heap ? 0 : n),
		      gain_value(n, min_g - 1), min_gain(min_g), max_gain(min_g), pos(use_heap ? n : 0, -1) {
			if (use_heap) {
				heap.reserve(n);
			}
		}

		void insert(int_t gain, int_t id) {

			if (gain_value[id] >= min_gain) {
				throw std::logic_error("BucketPriorityQueue: id already present");
			}

			if (use_heap) {
				gain_value[id] = gain;
				pos[id]        = static_cast<int_t>(heap.size());
				heap.push_back(id);
				SiftUp(pos[id]);
				n_nodes++;
				return;
			}

			int_t idx      = gain - min_gain;
			gain_value[id] = gain;

			nodes[id].next = buckets[idx];
			nodes[id].prev = -1;
			if (buckets[idx] != -1) {
				nodes[buckets[idx]].prev = id;
			}
			buckets[idx] = id;

			if (max_gain < gain) {
				max_gain = gain;
			}
			n_nodes++;
		}

		void add(int_t delta, int_t id) {
			if (gain_value[id] < min_gain) {
				return;
			}

			if (use_heap) {
				gain_value[id] += delta;
				if (delta > 0) {
					SiftUp(pos[id]);
				}
				else {
					SiftDown(pos[id]);
				}
				return;
			}

			int_t old_gain = gain_value[id];
			int_t old_idx  = old_gain - min_gain;

			if (nodes[id].prev != -1) {
				nodes[nodes[id].prev].next = nodes[id].next;
			}
			else {
				buckets[old_idx] = nodes[id].next;
			}

			if (nodes[id].next != -1) {
				nodes[nodes[id].next].prev = nodes[id].prev;
			}

			int_t new_gain = old_gain + delta;
			int_t new_idx  = new_gain - min_gain;
			gain_value[id] = new_gain;

			nodes[id].next = buckets[new_idx];
			nodes[id].prev = -1;
			if (buckets[new_idx] != -1) {
				nodes[buckets[new_idx]].prev = id;
			}
			buckets[new_idx] = id;

			if (new_gain > max_gain) {
				max_gain = new_gain;
			}
			else if (old_gain == max_gain && buckets[old_idx] == -1) {
				while (buckets[max_gain - min_gain] == -1) {
					max_gain--;
				}
			}
		}

		std::pair<int_t, int_t> extract() { // max
			if (use_heap) {
				const int_t id   = heap[0];
				const int_t gain = gain_value[id];
				HeapSwap(0, static_cast<int_t>(heap.size()) - 1);
				heap.pop_back();
				pos[id]        = -1;
				gain_value[id] = min_gain - 1;
				n_nodes--;
				if (!heap.empty()) {
					SiftDown(0);
				}
				return {gain, id};
			}

			int_t idx  = max_gain - min_gain;
			int_t id   = buckets[idx];
			int_t gain = gain_value[id];

			buckets[idx] = nodes[id].next;
			if (nodes[id].next != -1) {
				nodes[nodes[id].next].prev = -1;
			}

			gain_value[id] = min_gain - 1;
			n_nodes--;

			if (n_nodes > 0) {
				while (buckets[max_gain - min_gain] == -1) {
					max_gain--;
				}
			}
			else {
				max_gain = min_gain;
			}

			return {gain, id};
		}

		std::pair<int_t, int_t> top() const {
			if (empty())
				throw std::runtime_error("Empty BucketPriorityQueue");
			if (use_heap) {
				return {gain_value[heap[0]], heap[0]};
			}
			int_t idx = max_gain - min_gain;
			int_t id  = buckets[idx];
			return {gain_value[id], id};
		}

		bool empty() const {
			return n_nodes == 0;
		}

		bool contains(int_t id) const {
			return gain_value[id] >= min_gain;
		}
	};
} // namespace yagkp