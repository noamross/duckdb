
#include "catch.hpp"
#include "test_helpers.hpp"

using namespace duckdb;
using namespace std;

TEST_CASE("Test ORDER BY keyword", "[order]") {
	unique_ptr<DuckDBResult> result;
	DuckDB db(nullptr);
	DuckDBConnection con(db);

	result = con.Query("CREATE TABLE test (a INTEGER, b INTEGER);");
	result = con.Query("INSERT INTO test VALUES (11, 22)");
	result = con.Query("INSERT INTO test VALUES (12, 21)");
	result = con.Query("INSERT INTO test VALUES (13, 22)");

	// simple ORDER BY
	result = con.Query("SELECT b FROM test ORDER BY a DESC;");
	CHECK_COLUMN(result, 0, {22, 21, 22});

	result = con.Query("SELECT a, b FROM test ORDER BY a;");
	CHECK_COLUMN(result, 0, {11, 12, 13});
	CHECK_COLUMN(result, 1, {22, 21, 22});

	result = con.Query("SELECT a, b FROM test ORDER BY a DESC;");
	CHECK_COLUMN(result, 0, {13, 12, 11});
	CHECK_COLUMN(result, 1, {22, 21, 22});

	// ORDER BY on multiple columns
	result = con.Query("SELECT a, b FROM test ORDER BY b, a;");
	CHECK_COLUMN(result, 0, {12, 11, 13});
	CHECK_COLUMN(result, 1, {21, 22, 22});

	// ORDER BY using select indices
	result = con.Query("SELECT a, b FROM test ORDER BY 2, 1;");
	CHECK_COLUMN(result, 0, {12, 11, 13});
	CHECK_COLUMN(result, 1, {21, 22, 22});

	result = con.Query("SELECT a, b FROM test ORDER BY b DESC, a;");
	CHECK_COLUMN(result, 0, {11, 13, 12});
	CHECK_COLUMN(result, 1, {22, 22, 21});

	result = con.Query("SELECT a, b FROM test ORDER BY b, a DESC;");
	CHECK_COLUMN(result, 0, {12, 13, 11});
	CHECK_COLUMN(result, 1, {21, 22, 22});

	// TOP N queries
	result = con.Query("SELECT a, b FROM test ORDER BY b, a DESC LIMIT 1;");
	CHECK_COLUMN(result, 0, {12});
	CHECK_COLUMN(result, 1, {21});

	// Offset
	result =
	    con.Query("SELECT a, b FROM test ORDER BY b, a DESC LIMIT 1 OFFSET 1;");
	CHECK_COLUMN(result, 0, {13});
	CHECK_COLUMN(result, 1, {22});

	result = con.Query("SELECT a, b FROM test WHERE a < 13 ORDER BY b;");
	CHECK_COLUMN(result, 0, {12, 11});
	CHECK_COLUMN(result, 1, {21, 22});

	result = con.Query("SELECT a, b FROM test WHERE a < 13 ORDER BY 2;");
	CHECK_COLUMN(result, 0, {12, 11});
	CHECK_COLUMN(result, 1, {21, 22});

	result = con.Query("SELECT a, b FROM test WHERE a < 13 ORDER BY b DESC;");
	CHECK_COLUMN(result, 0, {11, 12});
	CHECK_COLUMN(result, 1, {22, 21});

	result = con.Query("SELECT b, a FROM test WHERE a < 13 ORDER BY b DESC;");
	CHECK_COLUMN(result, 0, {22, 21});
	CHECK_COLUMN(result, 1, {11, 12});

	// order by expression
	result = con.Query(
	    "SELECT b % 2 AS f, SUM(a) FROM test GROUP BY f ORDER BY b % 2;");
	CHECK_COLUMN(result, 0, {0, 1});
	CHECK_COLUMN(result, 1, {24, 12});

	// order by expression that is not in SELECT
	result = con.Query("SELECT b % 2 AS f, a FROM test ORDER BY b % 4;");
	CHECK_COLUMN(result, 0, {1, 0, 0});
	CHECK_COLUMN(result, 1, {12, 11, 13});

	// ORDER BY alias
	result =
	    con.Query("SELECT b % 2 AS f, SUM(a) FROM test GROUP BY f ORDER BY f;");
	CHECK_COLUMN(result, 0, {0, 1});
	CHECK_COLUMN(result, 1, {24, 12});

	result =
	    con.Query("SELECT b % 2 AS f, SUM(a) FROM test GROUP BY f ORDER BY 1;");
	CHECK_COLUMN(result, 0, {0, 1});
	CHECK_COLUMN(result, 1, {24, 12});
}