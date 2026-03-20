#include "async_duckdb_ut.h"
#include "../DuckDB/async_duckdb.hpp"
#include <iostream>
#include <vector>

// -------------------------------------------------------------------------
// Tests: Basic Connections & Queries
// -------------------------------------------------------------------------
static bool TestBasics(async::Connection& conn) {
    std::cout << " Testing: Basic Querying..." << std::endl;

    auto res = conn.Query("CREATE TABLE test (id INTEGER, val VARCHAR)");
    if (res->HasError()) return false;

    res = conn.Query("INSERT INTO test VALUES (1, 'A'), (2, 'B')");
    if (res->HasError()) return false;

    res = conn.Query("SELECT COUNT(*) FROM test");
    auto chunk = res->Fetch();
    // Verify count is 2
    if (chunk->GetValue(0, 0).GetValue<int64_t>() != 2) return false;

    return true;
}

// -------------------------------------------------------------------------
// Tests: Prepared Statements
// -------------------------------------------------------------------------
static bool TestPrepared(async::Connection& conn) {
    std::cout << " Testing: Prepared Statements..." << std::endl;

    auto stmt = conn.Prepare("INSERT INTO test VALUES (?, ?)");
    if (!stmt->Success()) return false;
    if (stmt->nParam() != 2) return false;

    stmt->Bind(1, 100);
    stmt->Bind(2, std::string("PreparedData"));

    auto res = stmt->Execute();
    if (res->HasError()) return false;

    res = conn.Query("SELECT val FROM test WHERE id = 100");
    auto chunk = res->Fetch();
    // Verify string value
    if (chunk->GetValue(0, 0).ToString() != "PreparedData") return false;

    return true;
}

// -------------------------------------------------------------------------
// Tests: Bulk Appender
// -------------------------------------------------------------------------
static bool TestAppender(async::Connection& conn) {
    std::cout << " Testing: Appender (Bulk Load)..." << std::endl;

    auto appender = conn.CreateAppender("test");
    for (int i = 200; i < 210; ++i) {
        appender->BeginRow();
        appender->Append(i);
        appender->Append("Bulk");
        appender->EndRow();
    }
    appender->Flush();

    auto res = conn.Query("SELECT COUNT(*) FROM test WHERE val = 'Bulk'");
    auto chunk = res->Fetch();
    auto count = chunk->GetValue(0, 0).GetValue<int64_t>();
    std::cout << " Appender Count: " << count << std::endl;
    // Verify we added 10 rows
    if (count != 10) return false;

    return true;
}

// -------------------------------------------------------------------------
// Main Test Runner
// -------------------------------------------------------------------------
int RunUnitTests() {
    std::cout << "--- Starting Async-DuckDB Unit Tests ---" << std::endl;

    try {
        async::Database db(nullptr); // In-memory
        async::Connection conn(db);

        if (!TestBasics(conn)) return 1;
        if (!TestPrepared(conn)) return 1;
        if (!TestAppender(conn)) return 1;

        std::cout << "--- All Tests Passed! ---" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << " [CRITICAL] Exception during tests: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}