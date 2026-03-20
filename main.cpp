// Asynchronous DuckDB API's implemented using C++ delegates
// @see https://github.com/endurodave/Async-DuckDB
// @see https://github.com/endurodave/DelegateMQ
// David Lafreniere, 2026.

#include "DelegateMQ.h"
#include "DuckDB/async_duckdb.hpp"
#include "UnitTest/async_duckdb_ut.h"
#include "Examples/Examples.h"
#include <stdio.h>
#include <iostream>
#include <filesystem>

void DeleteDatabaseFiles() {
    try {
        std::filesystem::remove("simple.db");
        std::filesystem::remove("futures.db");
        std::filesystem::remove("prepared.db");
        std::filesystem::remove("appender.db");
        std::filesystem::remove("transactions.db");
    } catch (...) {}
}

int main(void)
{
    DeleteDatabaseFiles();
    bool all_examples_passed = true;
    try {
        async::init_worker();
        
        std::cout << "=== Starting Async-DuckDB Examples ===\n";
        all_examples_passed &= RunSimpleExample();
        all_examples_passed &= RunFutureExample();
        all_examples_passed &= RunPreparedStatementExample();
        all_examples_passed &= RunAppenderExample();
        all_examples_passed &= RunTransactionExample();
        all_examples_passed &= RunStressTest();
        
        if (all_examples_passed) {
            std::cout << "\n=== All Examples Completed Successfully ===\n";
        } else {
            std::cerr << "\n!!! Some Examples Failed !!!\n";
        }

        int ut_ret = RunUnitTests();
        
        async::shutdown_worker();
        
        return (all_examples_passed && ut_ret == 0) ? 0 : 1;
    }
    catch (const std::exception& e) {
        std::cerr << "Unhandled Exception: " << e.what() << std::endl;
        return 1;
    }
}
