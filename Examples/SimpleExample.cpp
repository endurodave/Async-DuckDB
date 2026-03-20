#include "async_duckdb.hpp"
#include "ExampleUtils.h"
#include <iostream>

#include "Examples.h"

void RunSimpleExample() {
    try {
        std::cout << "\n--- Running Simple Example ---\n";
        // 1. Open Database (Creates 'simple.db' on disk)
        async::Database db("");
        async::Connection conn(db);

        // 2. Create Table
        std::cout << "Creating table 'users'..." << std::endl;
        conn.Query("CREATE TABLE IF NOT EXISTS users (id INTEGER, name VARCHAR, age INTEGER);");

        // 3. Insert Data
        std::cout << "Inserting data..." << std::endl;
        conn.Query("INSERT INTO users VALUES (1, 'Alice', 30), (2, 'Bob', 25), (3, 'Charlie', 35);");

        // 4. Simple Query
        std::cout << "Querying all users:" << std::endl;
        auto result = conn.Query("SELECT * FROM users ORDER BY age ASC;");
        PrintResult(result.get());

        // 5. Update Data
        std::cout << "\nUpdating Bob's age..." << std::endl;
        conn.Query("UPDATE users SET age = 26 WHERE name = 'Bob';");

        // 6. Final Query
        std::cout << "Final user list:" << std::endl;
        result = conn.Query("SELECT name, age FROM users;");
        PrintResult(result.get());
    }
    catch (const std::exception& e) {
        std::cerr << "SimpleExample Error: " << e.what() << std::endl;
    }
}
