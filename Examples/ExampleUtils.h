#ifndef EXAMPLE_UTILS_H
#define EXAMPLE_UTILS_H

#include <iostream>
#include <string>
#include "duckdb.hpp"

inline void PrintResult(duckdb::QueryResult* result)
{
    if (!result || result->HasError()) {
        std::cout << "Query Error: " << (result ? result->GetError() : "Unknown") << std::endl;
        return;
    }

    // Print headers
    for (duckdb::idx_t i = 0; i < result->ColumnCount(); i++) {
        std::cout << result->ColumnName(i) << "\t";
    }
    std::cout << "\n" << std::string(8 * result->ColumnCount(), '-') << "\n";

    // Print rows
    for (auto& row : *result) {
        for (duckdb::idx_t i = 0; i < result->ColumnCount(); i++) {
            std::cout << row.GetValue<duckdb::Value>(i).ToString() << "\t";
        }
        std::cout << "\n";
    }
}

inline size_t GetRowCount(duckdb::QueryResult* result)
{
    if (!result || result->HasError()) return 0;
    size_t count = 0;
    for (auto& row : *result) {
        (void)row;
        count++;
    }
    return count;
}

#endif // EXAMPLE_UTILS_H
