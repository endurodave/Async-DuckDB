#include "async_duckdb.hpp"
#include "ExampleUtils.h"
#include <iostream>

#include "Examples.h"

bool RunAppenderExample() {
    try {
        std::cout << "\n--- Running Appender Example ---\n";
        async::Database db("");
        async::Connection conn(db);

        conn.Query("CREATE TABLE IF NOT EXISTS metrics (cpu_usage DOUBLE, mem_usage DOUBLE, process_name VARCHAR);");

        // 1. Create an Appender for fast bulk loading
        std::cout << "Creating appender for table 'metrics'..." << std::endl;
        auto appender = conn.CreateAppender("metrics");

        // 2. Append rows
        std::cout << "Bulk loading data..." << std::endl;
        for (int i = 0; i < 100; ++i) {
            appender->BeginRow();
            appender->Append(1.5 * i);   // cpu_usage
            appender->Append(0.5 * i);   // mem_usage
            appender->Append("process_" + std::to_string(i)); // process_name
            appender->EndRow();
        }

        // 3. Flush and Close
        appender->Flush();
        appender->Close();

        // 4. Verify results
        std::cout << "Total rows in 'metrics':" << std::endl;
        auto result = conn.Query("SELECT count(*) FROM metrics;");
        PrintResult(result.get());
        
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "AppenderExample Error: " << e.what() << std::endl;
        return false;
    }
}
