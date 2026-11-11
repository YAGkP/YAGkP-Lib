#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace yagkp {

	// Signed integer type
	using int_t = std::int64_t;

	// Floating-point type
	using fp_t = double;

	// Side of a vertex in a bisection
	enum class Part { First, Second };

	// Partition into k parts: the part number {0, 1, ..., k-1} for every vertex
	using Partition = std::vector<int_t>;

	// Partition into two parts: the side of every vertex
	using Bisection = std::vector<Part>;

	using std::vector;
	using std::array;
	using std::string;

	template <typename T> void setmax(T& a, T b) {
		if (b > a) {
			a = b;
		}
	}

	template <typename T> void setmin(T& a, T b) {
		if (b < a) {
			a = b;
		}
	}

	inline Part GetOtherPart(Part part) {
		if (part == Part::First) {
			return Part::Second;
		}
		return Part::First;
	}
} // namespace yagkp