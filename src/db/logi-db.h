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

namespace logi
{

/**
 * Abstraction of Logi's database. Each network should have a unique
 * provider name.
 */
class Database
{
public:
    /// Result type is either void or
    /// std::shared_ptr<std::vector<std::tuple<...>>>
    template<class Result, typename... Args>
    class Statement
    {
    public:
        using ArgsVector = std::vector<std::tuple<Args...> >;

        virtual ~Statement() = default;

        virtual Result query(ArgsVector &args) = 0;
    };

    template<class Result, typename... Args>
    using StatementPtr = std::shared_ptr<Statement<Result, Args...> >;
private:
    class CurriedStatementBase
    {
    public:
        virtual ~CurriedStatementBase() = default;

        virtual void execute() = 0;

        virtual void forward_result() = 0;
    };

    template<class Result, class... Args> class CurriedQuery :
        public CurriedStatementBase
    {
    public:
        using ArgPtr = std::shared_ptr<std::vector<std::tuple<Args...> > >;
        using Slot = sigc::slot<void, Result>;
        using Stmt = Statement<Result, Args...>;
              
        CurriedQuery(Stmt &statement, ArgPtr args, Slot &callback)
            : statement_(statement), args_(args), callback_(callback)
        {}

        virtual void execute() override
        {
            result_ = statement_.query(*args_);
        }

        virtual void forward_result() override
        {
            callback_(result_);
        }
    private:
        Stmt &statement_;
        ArgPtr args_;
        Result result_;
        Slot callback_;
    };

    template<class... Args> class CurriedStatement : public CurriedStatementBase
    {
    public:
        using ArgPtr = std::shared_ptr<std::vector<std::tuple<Args...> > >;
        using Stmt = StatementPtr<void, Args...>;
              
        CurriedStatement(Stmt statement, ArgPtr args)
            : statement_(statement), args_(args)
        {}

        virtual void execute() override
        {
            statement_.query(*args_);
        }

        virtual void forward_result() override
        {
        }
    private:
        Stmt statement_;
        ArgPtr args_;
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
    virtual StatementPtr<void, id_t, std::string>
        get_insert_network_info_statement(const char *provider) = 0;

    /**
     * statement args: network_id, ts_id
     */
    virtual StatementPtr<void, id_t, id_t>
        get_insert_transport_stream_info_statement(const char *provider) = 0;

    /**
     * statement args: network_id, ts_id, tuning prop key, tuning prop value
     */
    virtual StatementPtr<void, id_t, id_t, id_t, id_t>
        get_insert_tuning_statement(const char *provider) = 0;

    /**
     * statement args: network_id, service_id, ts_id
     */
    virtual StatementPtr<void, id_t, id_t, id_t>
        get_insert_service_id_statement(const char *provider) = 0;

    /**
     * statement args: network_id, service_id, name
     */
    virtual StatementPtr<void, id_t, id_t, std::string>
        get_insert_service_name_statement(const char *provider) = 0;

    /**
     * statement args: network_id, service_id, provider_name
     */
    virtual StatementPtr<void, id_t, id_t, std::string>
    get_insert_service_provider_name_statement(const char *provider) = 0;

    /**
     * Queues a query to be executed on the database thread. The result callback
     * is called on the Glib main thread.
     */
    template<class Result, typename... Args>
    void queue_query(StatementPtr<Result, Args...> statement,
            std::shared_ptr<std::tuple<Args...> > args,
            sigc::slot<void, Result> &callback)
    {
        queue_statement(new CurriedQuery<Result, Args...>
                (statement, args, callback));
    }

    /**
     * Like queue_query when there is no result.
     */
    template<typename... Args>
    void queue_statement(StatementPtr<void, Args...> statement,
            std::shared_ptr<std::tuple<Args...> > args)
    {
        queue_statement(new CurriedStatement<Args...>(statement, args));
    }
private:
    void thread_main();

    void queue_statement(CurriedStatementBase *statement);

    bool result_idle_callback();

    std::thread *thread_ = nullptr;
    std::mutex mut_;
    std::condition_variable cv_;
    std::queue<CurriedStatementBase *> statement_queue_, result_queue_;
    bool stop_ = false;
};

}
