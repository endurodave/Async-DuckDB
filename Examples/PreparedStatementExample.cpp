#include "async_duckdb.hpp"
#include "ExampleUtils.h"
#include <iostream>

#include "Examples.h"

bool RunPreparedStatementExample() {
    try {
        std::cout << "\n--- Running Prepared Statement Example ---\n";
        async::Database db("");
        async::Connection conn(db);

        conn.Query("CREATE TABLE IF NOT EXISTS products (id INTEGER, name VARCHAR, price DOUBLE);");

        // 1. Prepare a statement
        std::cout << "Preparing INSERT statement..." << std::endl;
        auto stmt = conn.Prepare("INSERT INTO products VALUES (?, ?, ?);");

        if (!stmt->Success()) {
            throw std::runtime_error("Failed to prepare statement: " + stmt->GetError());
        }

        // 2. Bind values and execute
        std::cout << "Inserting products using prepared statement..." << std::endl;
        
        stmt->Bind(1, 101);
        stmt->Bind(2, "Laptop");
        stmt->Bind(3, 1200.50);
        stmt->Execute();

        stmt->Bind(1, 102);
        stmt->Bind(2, "Mouse");
        stmt->Bind(3, 25.99);
        stmt->Execute();

        // 3. Prepare a SELECT statement
        auto selectStmt = conn.Prepare("SELECT * FROM products WHERE price > ?;");
        selectStmt->Bind(1, 50.0);
        
        std::cout << "Products with price > 50.0:" << std::endl;
        auto result = selectStmt->Execute();
        PrintResult(result.get());
        
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "PreparedStatementExample Error: " << e.what() << std::endl;
        return false;
    }
}
