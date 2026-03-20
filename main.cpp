// Asynchronous DuckDB API's implemented using C++ delegates
// @see https://github.com/endurodave/Async-DuckDB
// @see https://github.com/endurodave/DelegateMQ
// David Lafreniere, 2026.

#include "DelegateMQ.h"
#include "DuckDB/async_duckdb.hpp"
#include "UnitTest/async_duckdb_ut.h"
#include <stdio.h>
#include <iostream>

void PrintResult(duckdb::QueryResult* result)
{
    if (!result || result->HasError()) {
        std::cout << "Query Error: " << (result ? result->GetError() : "Unknown") << std::endl;
        return;
    }

    // Print rows
    for (auto& row : *result) {
        std::string rowLine;
        for (duckdb::idx_t i = 0; i < result->ColumnCount(); i++) {
            rowLine += row.GetValue<duckdb::Value>(i).ToString() + "\t";
        }
        std::cout << rowLine << std::endl;
    }
}

void example1()
{
    std::cout << "\n--- Example 1: Simple ---\n";

    try {
        // 1. Open Database (Creates 'async_simple.db' on disk)
        async::Database db("async_simple.db");
        async::Connection conn(db);

        // 2. Create Table
        conn.Query("CREATE TABLE IF NOT EXISTS people (id INTEGER, name VARCHAR);");
        std::cout << "Table created." << std::endl;

        // 3. Insert
        conn.Query("INSERT INTO people VALUES (1, 'John'), (2, 'Jane');");
        std::cout << "Records inserted." << std::endl;

        // 4. Select
        auto result = conn.Query("SELECT * FROM people;");
        PrintResult(result.get());
    }
    catch (const std::exception& e) {
        std::cerr << "Example 1 Error: " << e.what() << std::endl;
    }
}

int main(void)
{
    try {
        async::init_worker();
        
        example1();
        
        int ret = RunUnitTests();
        
        async::shutdown_worker();
        
        return ret;
    }
    catch (const std::exception& e) {
        std::cerr << "Unhandled Exception: " << e.what() << std::endl;
        return 1;
    }
}
