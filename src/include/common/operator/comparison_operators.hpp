//===----------------------------------------------------------------------===//
//                         DuckDB
//
// common/operator/comparison_operators.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include <cstring>

namespace operators {

//===--------------------------------------------------------------------===//
// Comparison Operations
//===--------------------------------------------------------------------===//
struct Equals {
	template <class T> static inline bool Operation(T left, T right) {
		return left == right;
	}
};
template <> inline bool Equals::Operation(const char *left, const char *right) {
	return strcmp(left, right) == 0;
}
struct NotEquals {
	template <class T> static inline bool Operation(T left, T right) {
		return left != right;
	}
};
template <> inline bool NotEquals::Operation(const char *left, const char *right) {
	return strcmp(left, right) != 0;
}
struct GreaterThan {
	template <class T> static inline bool Operation(T left, T right) {
		return left > right;
	}
};
template <> inline bool GreaterThan::Operation(const char *left, const char *right) {
	return strcmp(left, right) > 0;
}
struct GreaterThanEquals {
	template <class T> static inline bool Operation(T left, T right) {
		return left >= right;
	}
};
template <> inline bool GreaterThanEquals::Operation(const char *left, const char *right) {
	return strcmp(left, right) >= 0;
}
struct LessThan {
	template <class T> static inline bool Operation(T left, T right) {
		return left < right;
	}
};
template <> inline bool LessThan::Operation(const char *left, const char *right) {
	return strcmp(left, right) < 0;
}
struct LessThanEquals {
	template <class T> static inline bool Operation(T left, T right) {
		return left <= right;
	}
};
template <> inline bool LessThanEquals::Operation(const char *left, const char *right) {
	return strcmp(left, right) <= 0;
}
} // namespace operators
