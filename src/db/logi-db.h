#pragma once

/*
    logi - A DVB DVR designed for web-based clients.
    Copyright (C) 2017 Tony Houghton <h@realh.co.uk>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

#include <glibmm/main.h>
#include <glibmm/ustring.h>

namespace logi
{

/**
 * Abstraction of Logi's database. Each network should have a unique
 * provider name.
 */
class Database
{
public:
    template<typename... Args> using Tuple = std::tuple<Args...>;
    template<typename... Args> using Vector = std::vector<Tuple<Args...>>;

    /// Result type is either void or
    /// std::shared_ptr<std::vector<Tuple<...>>>.
    /// Input is a single row of data.
    template<class Result, typename... Args> class Query
    {
    public:
        using ArgsTuple = std::tuple<Args...>;

        virtual ~Query() = default;

        virtual Result query(const ArgsTuple &args) = 0;
    };

    template<class Result, typename... Args>
    using QueryPtr = std::shared_ptr<Query<Result, Args...>>;

    /// For queries with no arguments
    template<class Result> class Query<Result, void>
    {
    public:
        virtual ~Query() = default;

        virtual Result query() = 0;
    };

    /// An insertion/modification statement that takes multiple rows of input
    template<typename... Args>
    class Statement
    {
    public:
        using ArgsTuple = std::tuple<Args...>;
        using ArgsVector = std::vector<ArgsTuple>;

        virtual ~Statement() = default;

        virtual void execute(const ArgsVector &args) = 0;
    };

    template<typename... Args>
    using StatementPtr = std::shared_ptr<Statement<Args...>>;
private:
    class CurriedStatementBase
    {
    public:
        virtual ~CurriedStatementBase() = default;

        virtual void execute() = 0;

        virtual void forward_result() = 0;

        virtual bool has_result() const = 0;
    };

    template<class Result, class... Args> class CurriedQuery :
        public CurriedStatementBase
    {
    public:
        using ArgPtr = std::shared_ptr<std::tuple<Args...>>;
        using Slot = sigc::slot<void, Result>;
        using Q = Query<Result, Args...>;
              
        CurriedQuery(Q &query, ArgPtr args, Slot &callback)
            : query_(query), args_(args), callback_(callback)
        {}

        virtual void execute() override
        {
            result_ = query_.query(*args_);
        }

        virtual void forward_result() override
        {
            callback_(result_);
        }

        virtual bool has_result() const override
        {
            return true;
        }
    private:
        Q &query_;
        ArgPtr args_;
        Result result_;
        Slot callback_;
    };

    template<class... Args> class CurriedStatement : public CurriedStatementBase
    {
    public:
        using ArgPtr = std::shared_ptr<std::vector<std::tuple<Args...>>>;
        using Stmt = StatementPtr<Args...>;
              
        CurriedStatement(Stmt statement, ArgPtr args)
            : statement_(statement), args_(args)
        {}

        virtual void execute() override
        {
            statement_->execute(*args_);
        }

        virtual void forward_result() override
        {
        }

        virtual bool has_result() const override
        {
            return false;
        }
    private:
        Stmt statement_;
        ArgPtr args_;
    };

    /// Allows database thread to signal main thread when all preceding
    /// statements have been executed
    class PseudoQuery : public CurriedStatementBase
    {
    public:
        PseudoQuery(sigc::slot<void> &callback) : callback_(callback)
        {}

        virtual void execute() override
        {
        }

        virtual void forward_result() override
        {
            callback_();
        }

        virtual bool has_result() const override
        {
            return true;
        }
    private:
        sigc::slot<void> callback_;
    };

    /// For running arbitrary functions on the database thread
    template <class Callback>
    class PseudoStatement : public CurriedStatementBase
    {
    public:
        PseudoStatement(Callback callback) : callback_(callback)
        {}

        virtual void execute() override
        {
            callback_();
        }

        virtual void forward_result() override
        {
        }

        virtual bool has_result() const override
        {
            return false;
        }
    private:
        Callback callback_;
    };
public:
    using id_t = std::uint32_t;

    virtual ~Database();

    void start();

    /// Subclasses should call this in their destructors
    void stop();

    virtual void open() = 0;

    /**
     * statement args: network_id, name
     */
    virtual StatementPtr<id_t, Glib::ustring>
        get_insert_network_info_statement(const char *source) = 0;

    /**
     * statement args: orig_nw_id, ts_id, tuning prop key, tuning prop value
     */
    virtual StatementPtr<id_t, id_t, id_t, id_t>
        get_insert_tuning_statement(const char *source) = 0;

    /**
     * statement args: orig_nw_id, nw_id, ts_id, service_id
     */
    virtual StatementPtr<id_t, id_t, id_t, id_t>
        get_insert_transport_services_statement(const char *source) = 0;

    /**
     * statement args: orig_nw_id, service_id, ts_id, service_type, free_ca_mode
     */
    virtual StatementPtr<id_t, id_t, id_t, id_t, id_t>
        get_insert_service_id_statement(const char *source) = 0;

    /**
     * statement args: orig_nw_id, service_id, name
     */
    virtual StatementPtr<id_t, id_t, Glib::ustring>
        get_insert_service_name_statement(const char *source) = 0;

    /**
     * statement args: provider_name
     */
    virtual StatementPtr<Glib::ustring>
    get_insert_provider_name_statement(const char *source) = 0;

    /**
     * statement args: orig_nw_id, service_id, provider_name
     */
    virtual StatementPtr<id_t, id_t, id_t>
    get_insert_service_provider_id_statement(const char *source) = 0;

    /**
     * statement args: network_id, service_id, region_code, lcn, freesat_id
     */
    virtual StatementPtr<id_t, id_t, id_t, id_t, id_t>
    get_insert_network_lcn_statement(const char *source) = 0;

    /**
     * statement args: bouquet_id, region_code, region_name
     */
    virtual StatementPtr<id_t, id_t, Glib::ustring>
    get_insert_region_statement(const char *source) = 0;

    /**
     * statement args: lcn, original_network_id, service_id
     */
    virtual StatementPtr<id_t, id_t, id_t>
    get_insert_client_lcn_statement(const char *source) = 0;

    virtual StatementPtr<Glib::ustring>
    get_insert_source_statement() = 0;

    virtual QueryPtr<Vector<id_t>, Glib::ustring>
    get_provider_id_query(const char *source) = 0;

    /**
     * result fields: lcn
     */
    virtual QueryPtr<Vector<id_t>, void>
    get_network_lcns_query(const char *source) = 0;

    /**
     * result fields: network_id, service_id, region_code
     * statement args: lcn
     */
    virtual QueryPtr<Vector<id_t, id_t, id_t>, id_t>
    get_ids_for_network_lcn_query(const char *source) = 0;

    /**
     * result fields: network_id
     * statement args: network name
     */
    virtual QueryPtr<Vector<id_t>, Glib::ustring>
    get_network_id_for_name_query(const char *source) = 0;

    /**
     * result fields: region_code
     * statement args: region name, bouquet_id
     */
    virtual QueryPtr<Vector<id_t>, Glib::ustring, id_t>
    get_region_code_for_name_and_bouquet_query(const char *source) = 0;

    /**
     * result fields: original_network_id
     * statement args: network_id, service_id
     */
    virtual QueryPtr<Vector<id_t>, id_t, id_t>
    get_original_network_id_for_network_and_service_id_query
    (const char *source) = 0;

    /**
     * Queues a query to be executed on the database thread. The result callback
     * is called on the Glib main thread.
     */
    template<class Result, typename... Args>
    void queue_query(QueryPtr<Result, Args...> query,
            std::shared_ptr<Tuple<Args...>> args,
            sigc::slot<void, Result> &callback)
    {
        queue_statement(new CurriedQuery<Result, Args...>
                (query, args, callback));
    }

    /**
     * Runs a query immediately. Must be called from database thread.
     */
    template<class Result, typename... Args>
    Result run_query(QueryPtr<Result, Args...> query,
            const Tuple<Args...> &args)
    {
        return query->query(args);
    }

    template<class Result>
    Result run_query(QueryPtr<Result, void> query)
    {
        return query->query();
    }

    /**
     * Like queue_query but for a statement with multiple inputs and no result.
     */
    template<typename... Args>
    void queue_statement(StatementPtr<Args...> statement,
            std::shared_ptr<Vector<Args...>> args)
    {
        queue_statement(new CurriedStatement<Args...>(statement, args));
    }

    /**
     * Runs a statement immediately. Must be called from database thread.
     */
    template<typename... Args>
    void run_statement(StatementPtr<Args...> stmt,
            const Vector<Args...> &args)
    {
        stmt->execute(args);
    }

    /// Request a callback on the main thread when all preceding statements
    /// have been executed.
    void queue_callback(sigc::slot<void> callback)
    {
        queue_statement(new PseudoQuery(callback));
    }

    /// Run an arbitrary function or lambda on the database thread.
    template<class Callback>
    void queue_function(Callback cb)
    {
        queue_statement(new PseudoStatement<Callback>(cb));
    }

    /// Ensures that all tables required by named source exist.
    void ensure_tables(const char *source);
protected:
    /**
     * Each of the ensure_* methods creates the required table if it doesn't
     * already exist in the database.
     */
    virtual void ensure_network_info_table(const char *source) = 0;

    virtual void ensure_tuning_table(const char *source) = 0;

    virtual void ensure_transport_services_table(const char *source) = 0;

    virtual void ensure_service_id_table(const char *source) = 0;

    virtual void ensure_service_name_table(const char *source) = 0;

    virtual void ensure_provider_name_table(const char *source) = 0;

    virtual void ensure_service_provider_id_table(const char *source) = 0;

    virtual void ensure_network_lcn_table(const char *source) = 0;

    virtual void ensure_region_table(const char *source) = 0;

    virtual void ensure_client_lcn_table(const char *source) = 0;

    virtual void ensure_source_table() = 0;
private:
    void thread_main();

    void queue_statement(CurriedStatementBase *statement);

    bool result_idle_callback();

    void ensure_tables_callback(const char *source);

    std::thread *thread_ = nullptr;
    std::mutex mut_;
    std::condition_variable cv_;
    std::queue<CurriedStatementBase *> statement_queue_, result_queue_;
    bool stop_ = false;
};

}
