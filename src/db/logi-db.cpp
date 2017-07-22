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

#include <exception>
#include <future>

#include <glib.h>

#include "logi-db.h"

namespace logi
{

Database::~Database()
{
    stop();
    while (statement_queue_.size())
    {
        delete statement_queue_.front();
        statement_queue_.pop();
    }
    while (result_queue_.size())
    {
        delete result_queue_.front();
        result_queue_.pop();
    }
}

void Database::stop()
{
    if (stop_)
    {
        return;
    }
    stop_ = true;
    if (thread_)
    {
        std::unique_lock<std::mutex> lk(mut_);
        cv_.notify_one();
        lk.unlock();
        if (thread_->joinable())
            thread_->join();
        delete thread_;
        thread_ = nullptr;
    }
}

void Database::start()
{
    std::promise<bool> prom;
    thread_ = new std::thread([this, &prom]()
    {
        try
        {
            open();
            ensure_sources_table();
        }
        catch (...)
        {
            prom.set_exception(std::current_exception());
        }
        prom.set_value(true);
        thread_main();
    });
    prom.get_future().get();
}

void Database::thread_main()
{
    while (!stop_)
    {
        std::unique_lock<std::mutex> lk(mut_);
        cv_.wait(lk);
        while (statement_queue_.size())
        {
            auto stmt = statement_queue_.front();
            statement_queue_.pop();
            lk.unlock();
            try
            {
                stmt->execute();
            }
            catch (std::exception &x)
            {
                Glib::signal_idle().connect([x]()->bool
                {
                    g_critical("Database error: %s", x.what());
                    return false;
                });
            }
            lk.lock();
            if (stmt->has_result())
            {
                result_queue_.push(stmt);
                Glib::signal_idle().connect
                    (sigc::mem_fun(*this, &Database::result_idle_callback));
            }
        }
    }
}

void Database::queue_statement(CurriedStatementBase *statement)
{
    std::lock_guard<std::mutex> lk(mut_);
    statement_queue_.push(statement);
    cv_.notify_one();
}

bool Database::result_idle_callback()
{
    std::unique_lock<std::mutex> lk(mut_);
    while (result_queue_.size())
    {
        auto stmt = result_queue_.front();
        result_queue_.pop();
        lk.unlock();
        stmt->forward_result();
        delete stmt;
        lk.lock();
    }
    return false;
}

void Database::ensure_tables(const char *source)
{
    std::string s(source);
    queue_function([this, s]()
    {
        ensure_tables_callback(s.c_str());
    });
}

void Database::ensure_tables_callback(const char *source)
{
    ensure_network_info_table(source);
    ensure_transport_stream_info_table(source);
    ensure_tuning_table(source);
    ensure_service_id_table(source);
    ensure_service_name_table(source);
    ensure_service_provider_name_table(source);
}

}
