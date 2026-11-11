#pragma once

#include <cstddef>
#include <vector>

#include "yagkp/types.hpp"

namespace yagkp {
	template <typename T> class Queue {
		vector<T> data;
		size_t head = 0;

	  public:
		void push(const T& value) {
			data.push_back(value);
		}
		void pop() noexcept {
			++head;
		}
		T front() const {
			return data[head];
		}
		bool empty() const noexcept {
			return head >= data.size();
		}
		void reserve(int_t n) {
			data.reserve(static_cast<size_t>(n));
		}
		void clear() noexcept {
			data.clear();
			head = 0;
		}
	};
} // namespace yagkp