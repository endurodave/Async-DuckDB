#include "async_duckdb.hpp"
#include "ExampleUtils.h"
#include <iostream>
#include <vector>

#include "Examples.h"

bool RunFutureExample() {
    try {
        std::cout << "\n--- Running Future Example ---\n";
        async::Database db("");
        async::Connection conn(db);

        conn.Query("CREATE TABLE IF NOT EXISTS sensor_data (id INTEGER, timestamp TIMESTAMP, value DOUBLE);");

        // 1. Using QueryFuture for non-blocking execution
        std::cout << "Starting multiple asynchronous queries..." << std::endl;
        
        auto f1 = conn.QueryFuture("INSERT INTO sensor_data VALUES (1, current_timestamp, 10.5);");
        auto f2 = conn.QueryFuture("INSERT INTO sensor_data VALUES (2, current_timestamp, 11.2);");
        auto f3 = conn.QueryFuture("INSERT INTO sensor_data VALUES (3, current_timestamp, 9.8);");

        // Do some other work while the queries are running...
        std::cout << "Waiting for inserts to complete..." << std::endl;

        // Wait for results
        f1.get();
        f2.get();
        f3.get();

        // 2. Querying with Future
        std::cout << "Fetching results asynchronously..." << std::endl;
        auto selectFuture = conn.QueryFuture("SELECT * FROM sensor_data;");

        // Wait for the final result
        auto result = selectFuture.get();
        PrintResult(result.get());
        
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "FutureExample Error: " << e.what() << std::endl;
        return false;
    }
}
