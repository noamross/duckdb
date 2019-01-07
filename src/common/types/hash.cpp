#include "common/types/hash.hpp"

#include "common/exception.hpp"

using namespace std;

namespace duckdb {

template <> uint64_t Hash(uint64_t val) {
	return murmurhash64((uint32_t *)&val);
}

template <> uint64_t Hash(int64_t val) {
	return murmurhash64((uint32_t *)&val);
}

template <> uint64_t Hash(double val) {
	return murmurhash64((uint32_t *)&val);
}

template <> uint64_t Hash(const char *str) {
	uint64_t hash = 5381;
	uint64_t c;

	while ((c = *str++)) {
		hash = ((hash << 5) + hash) + c;
	}

	return hash;
}

template <> uint64_t Hash(char *val) {
	return Hash<const char *>(val);
}

//! djb2 hash function by Dan Bernstein copied from https://stackoverflow.com/questions/7666509/hash-function-for-string
uint32_t HashStr(const char *str) {
	uint32_t hash = 5381;
	int c;
	while ((c = *str++)) {
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	}
	return hash;
}

} // namespace duckdb
