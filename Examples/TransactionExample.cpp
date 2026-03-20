#include "async_duckdb.hpp"
#include "ExampleUtils.h"
#include <iostream>

#include "Examples.h"

void RunTransactionExample() {
    try {
        std::cout << "\n--- Running Transaction Example ---\n";
        async::Database db("");
        async::Connection conn(db);

        conn.Query("CREATE TABLE IF NOT EXISTS accounts (id INTEGER, balance DOUBLE);");
        conn.Query("INSERT INTO accounts VALUES (1, 1000.0), (2, 500.0);");

        std::cout << "Initial balances:" << std::endl;
        PrintResult(conn.Query("SELECT * FROM accounts;").get());

        // 1. Successful transaction
        std::cout << "\nStarting successful transaction (transfer 200 from 1 to 2)..." << std::endl;
        conn.BeginTransaction();
        try {
            conn.Query("UPDATE accounts SET balance = balance - 200 WHERE id = 1;");
            conn.Query("UPDATE accounts SET balance = balance + 200 WHERE id = 2;");
            conn.Commit();
            std::cout << "Transaction committed." << std::endl;
        } catch (...) {
            conn.Rollback();
            std::cout << "Transaction rolled back due to error." << std::endl;
        }

        std::cout << "Balances after successful transfer:" << std::endl;
        PrintResult(conn.Query("SELECT * FROM accounts;").get());

        // 2. Rolled back transaction
        std::cout << "\nStarting failing transaction (transfer 100 but force failure)..." << std::endl;
        conn.BeginTransaction();
        try {
            conn.Query("UPDATE accounts SET balance = balance - 100 WHERE id = 1;");
            
            // Force a failure (e.g., query a non-existent table)
            conn.Query("SELECT * FROM non_existent_table;");
            
            conn.Commit();
        } catch (const std::exception& e) {
            std::cout << "Caught expected error: " << e.what() << std::endl;
            conn.Rollback();
            std::cout << "Transaction rolled back." << std::endl;
        }

        std::cout << "Balances after failed transfer (should be unchanged):" << std::endl;
        PrintResult(conn.Query("SELECT * FROM accounts;").get());
    }
    catch (const std::exception& e) {
        std::cerr << "TransactionExample Error: " << e.what() << std::endl;
    }
}
