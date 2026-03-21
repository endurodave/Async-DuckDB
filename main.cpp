// main.cpp
// Async-DuckDB Entry Point
// 
// This application demonstrates the usage of Async-DuckDB, an asynchronous, 
// thread-safe proxy layer for the DuckDB analytical database engine.
// 
// Design Philosophy:
// 1. Proxy-Worker Pattern: User threads interact with 'async' proxy objects 
//    while all raw DuckDB operations are marshalled to a dedicated worker thread.
// 2. Thread Affinity: Ensures the non-thread-safe DuckDB Connection objects 
//    are only accessed from a single, consistent thread.
// 3. Non-Blocking execution: Leverages C++ Delegates (DelegateMQ) and 
//    std::future to allow heavy analytical queries without blocking the UI or 
//    main application logic.
// 
// This main executes a comprehensive suite of examples, stress tests, and 
// unit tests to verify the robustness and correctness of the wrapper.
// 
// @author David Lafreniere
// @date 2026

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
