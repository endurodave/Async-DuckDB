// Asynchronous DuckDB wrapper using C++ Delegates
// @see https://github.com/endurodave/DelegateMQ
// David Lafreniere, Jan 2026

#include "async_duckdb.hpp"
#include "DelegateMQ.h"
#include <future>
#include <tuple>
#include <stdexcept>
#include <thread>
#include <functional> 
#include <iostream>
#include <type_traits>
#include <optional>

using namespace dmq;

namespace async
{
    // A private worker thread instance
    static Thread DuckThread("DuckDB Thread");
    static IThread* CentralThread = &DuckThread;

    const std::string PreparedStatement::m_empty_error = "";

    // --------------------------------------------------------------------------------
    // Core Logic: Dispatch to Worker Thread
    // --------------------------------------------------------------------------------
    void DispatchTask(std::function<void()> task)
    {
        if (!CentralThread) return;
        
        auto delegate = dmq::MakeDelegate(task, *CentralThread);
        delegate.AsyncInvoke();
    }

    // Helper trait to check if a type is a pointer-like object to QueryResult
    template<typename T, typename = void>
    struct is_query_result_ptr : std::false_type {};

    template<typename T>
    struct is_query_result_ptr<T, std::void_t<decltype(std::declval<T>().operator->()->HasError())>> : std::true_type {};

    // Ensures a QueryResult is fully materialized on the worker thread before being
    // handed to the caller. StreamQueryResult is tied to the connection and must not
    // be accessed from another thread; this converts it to MaterializedQueryResult.
    static std::unique_ptr<duckdb::QueryResult> EnsureMaterialized(std::unique_ptr<duckdb::QueryResult> result) {
        if (!result || result->type != duckdb::QueryResultType::STREAM_RESULT) {
            return result;
        }
        return static_cast<duckdb::StreamQueryResult&>(*result).Materialize();
    }

    // --------------------------------------------------------------------------------
    // Helper: RunAsync (Returns std::future immediately)
    // --------------------------------------------------------------------------------
    template <typename Func, typename... Args>
    auto RunAsync(Func func, Args&&... args)
    {
        using RetType = std::invoke_result_t<std::decay_t<Func>, std::decay_t<Args>...>;

        auto promise = std::make_shared<std::promise<RetType>>();
        auto future = promise->get_future();

        auto task = [promise, func, args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
            try {
                if constexpr (std::is_void_v<RetType>) {
                    std::apply(func, std::move(args));
                    promise->set_value();
                }
                else {
                    promise->set_value(std::apply(func, std::move(args)));
                }
            }
            catch (...) {
                promise->set_exception(std::current_exception());
            }
            };

        DispatchTask(task);
        return future;
    }

    // --------------------------------------------------------------------------------
    // Helper: RunSync (Blocks until done, returns value)
    // --------------------------------------------------------------------------------
    template <typename Func, typename... Args>
    auto RunSync(dmq::Duration timeout, Func func, Args&&... args)
    {
        using RetType = std::invoke_result_t<std::decay_t<Func>, std::decay_t<Args>...>;

        // If we are already on the worker thread, just run it
        if (CentralThread->IsCurrentThread()) {
            return std::invoke(func, std::forward<Args>(args)...);
        }

        struct SyncState {
            dmq::Semaphore sema;
            std::exception_ptr ex;
            typename std::conditional_t<std::is_void_v<RetType>, bool, std::optional<RetType>> result;
        };
        auto state = std::make_shared<SyncState>();

        auto task = [state, func, args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
            try {
                if constexpr (std::is_void_v<RetType>) {
                    std::apply(func, std::move(args));
                }
                else {
                    state->result = std::apply(func, std::move(args));
                }
            }
            catch (...) {
                state->ex = std::current_exception();
            }
            state->sema.Signal();
            };

        DispatchTask(task);

        if (!state->sema.Wait(timeout)) {
            throw std::runtime_error("DuckDB Operation Timed Out");
        }

        if (state->ex) {
            std::rethrow_exception(state->ex);
        }
        
        if constexpr (!std::is_void_v<RetType>) {
            auto res = std::move(*(state->result));
            
            // Check if RetType is some kind of smart pointer to QueryResult
            if constexpr (is_query_result_ptr<RetType>::value) {
                if (res && res->HasError()) {
                    res->ThrowError();
                }
            }
            return res;
        }
    }

    // --------------------------------------------------------------------------------
    // Initialization & Thread Management
    // --------------------------------------------------------------------------------
    void init_worker(IThread* thread) { 
        if (thread) {
            CentralThread = thread;
        } else {
            DuckThread.CreateThread(); 
            CentralThread = &DuckThread;
        }
    }
    
    void shutdown_worker() { 
        if (CentralThread == &DuckThread) {
            DuckThread.ExitThread(); 
        }
        CentralThread = nullptr;
    }
    
    IThread* get_worker_thread() { return CentralThread; }

    // --------------------------------------------------------------------------------
    // Database Proxy
    // --------------------------------------------------------------------------------
    Database::Database(const char* path, dmq::Duration timeout) {
        auto task = [this, p = (path ? std::string(path) : std::string())]() {
            const char* dbPath = p.empty() ? nullptr : p.c_str();
            m_db = std::make_shared<duckdb::DuckDB>(dbPath);
            };
        RunSync(timeout, task);
    }

    Database::~Database() {
        if (!m_db) return;
        auto task = [db = std::move(m_db)]() {};
        DispatchTask(task);
    }

    // --------------------------------------------------------------------------------
    // PreparedStatement Proxy
    // --------------------------------------------------------------------------------
    PreparedStatement::~PreparedStatement() {
        if (m_state) {
            auto task = [state = std::move(m_state)]() {};
            DispatchTask(task);
        }
    }

    void PreparedStatement::BindValue(duckdb::idx_t index, duckdb::Value val) {
        if (index == 0) throw std::out_of_range("DuckDB Bind index starts at 1");
        auto task = [state = m_state, index, v = std::move(val)]() {
            if (state->stmt && state->stmt->success) {
                if (state->params.size() < index) {
                    state->params.resize(index);
                }
                state->params[index - 1] = v;
            }
            };
        RunSync(MAX_WAIT, task);
    }

    std::unique_ptr<duckdb::QueryResult> PreparedStatement::Execute(dmq::Duration timeout) {
        auto task = [state = m_state]() -> std::unique_ptr<duckdb::QueryResult> {
            return EnsureMaterialized(state->stmt->Execute(state->params));
        };
        return RunSync(timeout, task);
    }

    std::future<std::unique_ptr<duckdb::QueryResult>> PreparedStatement::ExecuteFuture() {
        auto task = [state = m_state]() -> std::unique_ptr<duckdb::QueryResult> {
            return EnsureMaterialized(state->stmt->Execute(state->params));
            };
        return RunAsync(task);
    }

    duckdb::idx_t PreparedStatement::nParam() {
        return RunSync(MAX_WAIT, [state = m_state]() {
            if (!state->stmt || !state->stmt->success) return (duckdb::idx_t)0;
            return (duckdb::idx_t)state->stmt->named_param_map.size();
            });
    }

    // --------------------------------------------------------------------------------
    // Appender Proxy
    // --------------------------------------------------------------------------------
    Appender::~Appender() {
        if (m_appender) {
            auto task = [app = std::move(m_appender)]() {
                app->Flush();
                app->Close();
                };
            DispatchTask(task);
        }
    }

    void Appender::BeginRow() {
        if (!m_appender) return;
        m_pending_row.clear();
    }

    void Appender::EndRow() {
        if (!m_appender) return;
        RunSync(MAX_WAIT, [app = m_appender, row = std::move(m_pending_row)]() {
            app->BeginRow();
            for (const auto& v : row) {
                app->Append(v);
            }
            app->EndRow();
        });
        m_pending_row.clear();
    }

    void Appender::Append(const char* val) {
        AppendValue(duckdb::Value(val));
    }

    void Appender::AppendValue(duckdb::Value val) {
        m_pending_row.push_back(std::move(val));
    }

    void Appender::Flush() {
        if (!m_appender) return;
        RunSync(MAX_WAIT, [app = m_appender]() { app->Flush(); });
    }

    void Appender::Close() {
        if (!m_appender) return;
        RunSync(MAX_WAIT, [app = m_appender]() { app->Close(); });
    }

    // --------------------------------------------------------------------------------
    // Connection Proxy
    // --------------------------------------------------------------------------------
    Connection::Connection(Database& db, dmq::Duration timeout) {
        auto task = [this, &db]() {
            m_conn = std::make_unique<duckdb::Connection>(*db.unsafe_raw());
            };
        RunSync(timeout, task);
    }

    Connection::~Connection() {
        if (!m_conn) return;
        auto task = [conn = std::move(m_conn)]() {};
        DispatchTask(task);
    }

    std::unique_ptr<duckdb::QueryResult> Connection::Query(const std::string& sql, dmq::Duration timeout) {
        auto task = [this, sql]() -> std::unique_ptr<duckdb::QueryResult> {
            return EnsureMaterialized(m_conn->Query(sql));
        };
        return RunSync(timeout, task);
    }

    std::future<std::unique_ptr<duckdb::QueryResult>> Connection::QueryFuture(const std::string& sql) {
        auto task = [this, sql]() -> std::unique_ptr<duckdb::QueryResult> {
            return EnsureMaterialized(m_conn->Query(sql));
            };
        return RunAsync(task);
    }

    std::unique_ptr<PreparedStatement> Connection::Prepare(const std::string& sql, dmq::Duration timeout) {
        auto task = [this, sql]() -> std::unique_ptr<PreparedStatement> {
            auto raw_stmt = m_conn->Prepare(sql);
            if (!raw_stmt->success) {
                raw_stmt->error.Throw();
            }
            std::shared_ptr<duckdb::PreparedStatement> shared_stmt = std::move(raw_stmt);
            return std::make_unique<PreparedStatement>(shared_stmt);
            };
        return RunSync(timeout, task);
    }

    std::unique_ptr<Appender> Connection::CreateAppender(const std::string& table, dmq::Duration timeout) {
        auto task = [this, table]() -> std::unique_ptr<Appender> {
            auto raw_app = std::make_unique<duckdb::Appender>(*m_conn, table);
            std::shared_ptr<duckdb::Appender> shared_app = std::move(raw_app);
            return std::make_unique<Appender>(shared_app);
            };
        return RunSync(timeout, task);
    }

    void Connection::BeginTransaction() {
        RunSync(MAX_WAIT, [this]() { m_conn->BeginTransaction(); });
    }

    void Connection::Commit() {
        RunSync(MAX_WAIT, [this]() { m_conn->Commit(); });
    }

    void Connection::Rollback() {
        RunSync(MAX_WAIT, [this]() { m_conn->Rollback(); });
    }

} // namespace async
