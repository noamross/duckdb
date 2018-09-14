#include "catch.hpp"
#include "test_helpers.hpp"

#include <random>
#include <thread>

using namespace duckdb;
using namespace std;

#define THREAD_COUNT 100
#define INSERT_ELEMENTS 10

TEST_CASE("Sequential append", "[transactions]") {
	unique_ptr<DuckDBResult> result;
	DuckDB db(nullptr);
	DuckDBConnection con(db);
	std::vector<std::unique_ptr<DuckDBConnection>> connections;

	// initialize the database
	con.Query("CREATE TABLE integers(i INTEGER);");

	for (size_t i = 0; i < THREAD_COUNT; i++) {
		connections.push_back(make_unique<DuckDBConnection>(db));
		connections[i]->Query("BEGIN TRANSACTION;");
	}

	for (size_t i = 0; i < THREAD_COUNT; i++) {
		result = connections[i]->Query("SELECT COUNT(*) FROM integers");
		Value count = result->collection.chunks[0]->data[0].GetValue(0);
		REQUIRE(Value::Equals(count, Value::Numeric(TypeId::BIGINT, 0)));
		for (size_t j = 0; j < INSERT_ELEMENTS; j++) {
			connections[i]->Query("INSERT INTO integers VALUES (3)");
			result = connections[i]->Query("SELECT COUNT(*) FROM integers");
			Value new_count = result->collection.chunks[0]->data[0].GetValue(0);
			REQUIRE(Value::Equals(new_count,
			                      Value::Numeric(TypeId::BIGINT, j + 1)));
			count = new_count;
		}
	}

	for (size_t i = 0; i < THREAD_COUNT; i++) {
		connections[i]->Query("COMMIT;");
	}
	result = con.Query("SELECT COUNT(*) FROM integers");
	Value count = result->collection.chunks[0]->data[0].GetValue(0);
	REQUIRE(Value::Equals(
	    count, Value::Numeric(TypeId::BIGINT, THREAD_COUNT * INSERT_ELEMENTS)));
}

static volatile std::atomic<int> finished_threads;

static void insert_random_elements(DuckDB *db) {
	REQUIRE(db);
	DuckDBConnection con(*db);
	// initial count
	con.Query("BEGIN TRANSACTION;");
	auto result = con.Query("SELECT COUNT(*) FROM integers");
	Value count = result->collection.chunks[0]->data[0].GetValue(0);
	auto start_count = count.GetNumericValue();
	for (size_t i = 0; i < INSERT_ELEMENTS; i++) {
		// count should increase by one for every append we do
		con.Query("INSERT INTO integers VALUES (3)");
		result = con.Query("SELECT COUNT(*) FROM integers");
		Value new_count = result->collection.chunks[0]->data[0].GetValue(0);
		REQUIRE(Value::Equals(
		    new_count, Value::Numeric(TypeId::BIGINT, start_count + i + 1)));
		count = new_count;
	}
	finished_threads++;
	while (finished_threads != THREAD_COUNT)
		;
	con.Query("COMMIT;");
}

TEST_CASE("[SLOW] Concurrent append", "[transactions]") {
	unique_ptr<DuckDBResult> result;
	DuckDB db(nullptr);
	DuckDBConnection con(db);

	// initialize the database
	con.Query("CREATE TABLE integers(i INTEGER);");

	finished_threads = 0;

	thread threads[THREAD_COUNT];
	for (size_t i = 0; i < THREAD_COUNT; i++) {
		threads[i] = thread(insert_random_elements, &db);
	}

	for (size_t i = 0; i < THREAD_COUNT; i++) {
		threads[i].join();
	}
}