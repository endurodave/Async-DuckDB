#include "async_duckdb.hpp"
#include "ExampleUtils.h"
#include "Examples.h"
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>

void RunStressTest() {
    std::cout << "\n--- Running Stress Test ---\n";

    try {
        async::Database db(""); // In-memory
        async::Connection main_conn(db);

        // Setup table
        main_conn.Query("CREATE TABLE stress (id INTEGER, thread_id INTEGER, val DOUBLE, msg VARCHAR);");

        const int NUM_THREADS = 8;
        const int OPS_PER_THREAD = 200;
        std::atomic<int> total_ops{0};
        std::atomic<int> errors{0};

        std::vector<std::thread> threads;
        auto start_time = std::chrono::high_resolution_clock::now();

        std::cout << "Starting " << NUM_THREADS << " threads, each doing " << OPS_PER_THREAD << " operations..." << std::endl;

        for (int i = 0; i < NUM_THREADS; ++i) {
            threads.emplace_back([&db, i, OPS_PER_THREAD, &total_ops, &errors]() {
                try {
                    async::Connection conn(db);
                    auto stmt = conn.Prepare("INSERT INTO stress VALUES (?, ?, ?, ?);");

                    for (int j = 0; j < OPS_PER_THREAD; ++j) {
                        // Mix of sync and async operations
                        if (j % 2 == 0) {
                            stmt->Bind(1, j);
                            stmt->Bind(2, i);
                            stmt->Bind(3, (double)j * 1.5);
                            stmt->Bind(4, "Message from thread " + std::to_string(i));
                            stmt->Execute();
                        } else {
                            auto fut = conn.QueryFuture("SELECT count(*) FROM stress WHERE thread_id = " + std::to_string(i));
                            fut.get();
                        }
                        total_ops++;
                    }
                }
                catch (const std::exception& e) {
                    std::cerr << "Thread " << i << " error: " << e.what() << std::endl;
                    errors++;
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        std::cout << "Stress test completed in " << duration.count() << "ms" << std::endl;
        std::cout << "Total operations: " << total_ops.load() << std::endl;
        std::cout << "Errors encountered: " << errors.load() << std::endl;

        auto final_count_res = main_conn.Query("SELECT count(*) FROM stress;");
        std::cout << "Final row count in 'stress' table: ";
        PrintResult(final_count_res.get());

        // Verify data integrity
        auto thread_counts = main_conn.Query("SELECT thread_id, count(*) FROM stress GROUP BY thread_id ORDER BY thread_id;");
        std::cout << "Rows per thread (should be " << OPS_PER_THREAD / 2 << "):" << std::endl;
        PrintResult(thread_counts.get());
    }
    catch (const std::exception& e) {
        std::cerr << "Stress test failed: " << e.what() << std::endl;
    }
}
