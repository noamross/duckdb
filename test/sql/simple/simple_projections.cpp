
#include "catch.hpp"
#include "test_helpers.hpp"

using namespace duckdb;
using namespace std;

TEST_CASE("Test simple projection statements", "[simpleprojection]") {
	unique_ptr<DuckDBResult> result;
	DuckDB db(nullptr);
	DuckDBConnection con(db);

	// create table
	result = con.Query("CREATE TABLE a (i integer, j integer);");

	// insertion: 1 affected row
	result = con.Query("INSERT INTO a VALUES (42, 84);");
	CHECK_COLUMN(result, 0, {1});

	result = con.Query("SELECT * FROM a;");
	CHECK_COLUMN(result, 0, {42});
	CHECK_COLUMN(result, 1, {84});

	// multiple insertions
	result = con.Query("CREATE TABLE test (a INTEGER, b INTEGER);");
	result = con.Query("INSERT INTO test VALUES (11, 22)");
	result = con.Query("INSERT INTO test VALUES (12, 21)");
	result = con.Query("INSERT INTO test VALUES (13, 22)");

	// multiple projections
	result = con.Query("SELECT a, b FROM test;");
	CHECK_COLUMN(result, 0, {11, 12, 13});
	CHECK_COLUMN(result, 1, {22, 21, 22});

	// basic expressions and filters
	result = con.Query("SELECT a + 2, b FROM test WHERE a = 11;");
	CHECK_COLUMN(result, 0, {13});
	CHECK_COLUMN(result, 1, {22});

	result = con.Query("SELECT a + 2, b FROM test WHERE a = 12;");
	CHECK_COLUMN(result, 0, {14});
	CHECK_COLUMN(result, 1, {21});

	// casts
	result = con.Query("SELECT cast(a AS VARCHAR) FROM test;");
	CHECK_COLUMN(result, 0, {"11", "12", "13"});

	result = con.Query("SELECT cast(cast(a AS VARCHAR) as INTEGER) FROM test;");
	CHECK_COLUMN(result, 0, {11, 12, 13});
}