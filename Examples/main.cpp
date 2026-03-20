#include "DuckDB/async_duckdb.hpp"
#include "Examples.h"
#include <iostream>

int main() {
    try {
        std::cout << "=== Starting Async-DuckDB Examples ===\n";
        
        // Initialize the asynchronous worker thread once
        async::init_worker();

        // Run all examples
        RunSimpleExample();
        RunFutureExample();
        RunPreparedStatementExample();
        RunAppenderExample();
        RunTransactionExample();

        // Shutdown the worker thread once
        async::shutdown_worker();
        
        std::cout << "\n=== All Examples Completed ===\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Unhandled Exception in Examples: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
