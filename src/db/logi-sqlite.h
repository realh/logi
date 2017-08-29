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

#include <exception>
#include <initializer_list>
#include <utility>

#include "logi-db.h"

struct sqlite3;
struct sqlite3_stmt;

namespace logi
{

class Sqlite3Error : public std::exception
{
public:
    Sqlite3Error(sqlite3 *db, int code, const Glib::ustring &desc);
    Sqlite3Error(sqlite3 *db, int code, Glib::ustring &&desc);
    Sqlite3Error(int code, const Glib::ustring &desc) :
        code_(code), desc_(desc) {}
    Sqlite3Error(int code, Glib::ustring &&desc) : code_(code), desc_(desc) {}
    Sqlite3Error(const Sqlite3Error &other) :
        code_(other.code_), desc_(other.desc_) {}
    Sqlite3Error(Sqlite3Error &&other) :
        code_(other.code_), desc_(std::move(other.desc_)) {}

    virtual const char *what() const noexcept override
    {
        return desc_.c_str();
    }

    int code() const { return code_; }
private:
    int code_;
    Glib::ustring desc_;
};

/**
 * Abstraction of Logi's database. Each network should have a unique
 * provider name.
 */
class Sqlite3Database : public Database
{
private:
    class Sqlite3StatementBase
    {
    public:
        Sqlite3StatementBase(sqlite3 *db, const char *sql);

        virtual ~Sqlite3StatementBase();
    protected:
        void reset();

        /// Returns true if result is SQLITE_DONE
        bool step();

        void bind(int pos, std::uint32_t val);

        void bind(int pos, const Glib::ustring &val);

        void fetch(int pos, std::uint32_t &val);

        void fetch(int pos, Glib::ustring &val);

        template<class T> T fetch(int pos)
        {
            T val;
            fetch(pos, val);
            return val;
        }

        template<class TupleType, std::size_t... Is>
        void bind_tuple(const TupleType &tup, std::index_sequence<Is...>)
        {
            // Prior to c++17 the ... has to be within an argument list
            // or similar, so we pretend to make a list of 0s.
            (void)
            std::initializer_list<int>{(bind(Is + 1, std::get<Is>(tup)), 0)...};
        }

        template<typename... Args>
        void bind_tuple(const Tuple<Args...> &tup)
        {
            bind_tuple(tup, std::index_sequence_for<Args...>());
        }

        template<class TupleType, std::size_t... Is>
        TupleType fetch_row(std::index_sequence<Is...>)
        {
            return std::make_tuple
                (fetch<typename std::tuple_element<Is, TupleType>::type>
                    (Is)...);
        }

        template<typename... Args> Tuple<Args...> fetch_row()
        {
            return fetch_row<Tuple<Args...>>
                (std::index_sequence_for<Args...>());
        }

        template<typename... RArgs>
        Vector<RArgs...> &fetch_rows(Vector<RArgs...> &v)
        {
            while (!step())
                v.push_back(fetch_row<RArgs...>());
            return v;
        }
    private:
        sqlite3_stmt *stmt_ = nullptr;
    };

    template<typename... Args>
    class Sqlite3Statement : public Statement<Args...>, Sqlite3StatementBase
    {
    public:
        using Parent = Statement<Args...>;
        using Tup = typename Parent::ArgsTuple;

        Sqlite3Statement(sqlite3 *db, const char *sql) :
            Sqlite3StatementBase(db, sql)
        {}

        Sqlite3Statement(sqlite3 *db, const Glib::ustring &sql) :
            Sqlite3StatementBase(db, sql.c_str())
        {}

        virtual void execute(const typename Parent::ArgsVector &args) override
        {
            for (const auto &tup: args)
            {
                reset();
                bind_tuple(tup);
                step();
            }
        }
    };

    template<class Result, typename... Args>
    class Sqlite3Query : public Query<Result, Args...>, Sqlite3StatementBase
    {
    public:
        using Parent = Query<Result, Args...>;
        using Tup = typename Parent::ArgsTuple;

        Sqlite3Query(sqlite3 *db, const char *sql) :
            Sqlite3StatementBase(db, sql)
        {}

        Sqlite3Query(sqlite3 *db, const Glib::ustring &sql) :
            Sqlite3StatementBase(db, sql.c_str())
        {}

        virtual Result query(const Tup &row) override
        {
            reset();
            bind_tuple(row);
            Result v;
            return fetch_rows(v);
        }
    };

    /// Query with no arguments
    template<class Result>
    class Sqlite3Query<Result, void> :
    public Query<Result, void>, Sqlite3StatementBase
    {
    public:
        using Parent = Query<Result, void>;

        Sqlite3Query(sqlite3 *db, const char *sql) :
            Sqlite3StatementBase(db, sql)
        {}

        Sqlite3Query(sqlite3 *db, const Glib::ustring &sql) :
            Sqlite3StatementBase(db, sql.c_str())
        {}

        virtual Result query() override
        {
            reset();
            Result v;
            return fetch_rows(v);
        }
    };
public:
    ~Sqlite3Database();

    virtual void open() override;

    /**
     * statement args: network_id, name
     */
    virtual StatementPtr<id_t, Glib::ustring>
        get_insert_network_info_statement(const char *source) override;

    /**
     * statement args: orig_nw_id, ts_id, nw_id,
     * tuning prop key, tuning prop value
     */
    virtual StatementPtr<id_t, id_t, id_t, id_t, id_t>
        get_insert_tuning_statement(const char *source) override;

    /**
     * statement args: orig_nw_id, nw_id, ts_id, service_id
     */
    virtual StatementPtr<id_t, id_t, id_t, id_t>
        get_insert_transport_services_statement(const char *source) override;

    /**
     * statement args: orig_nw_id, service_id, ts_id
     */
    virtual StatementPtr<id_t, id_t, id_t, id_t, id_t>
        get_insert_service_id_statement(const char *source) override;

    /**
     * statement args: orig_nw_id, service_id, name
     */
    virtual StatementPtr<id_t, id_t, Glib::ustring>
        get_insert_service_name_statement(const char *source) override;

    /**
     * statement args: provider_name
     */
    virtual StatementPtr<Glib::ustring>
    get_insert_provider_name_statement(const char *source) override;

    /**
     * statement args: orig_nw_id, service_id, provider_name
     */
    virtual StatementPtr<id_t, id_t, id_t>
    get_insert_service_provider_id_statement(const char *source) override;

    /**
     * statement args: network_id, service_id, lcn
     */
    virtual StatementPtr<id_t, id_t, id_t, id_t, id_t>
    get_insert_network_lcn_statement(const char *source) override;

    /**
     * statement args: bouquet_id, region_code, region_name
     */
    virtual StatementPtr<id_t, id_t, Glib::ustring>
    get_insert_region_statement(const char *source) override;

    /**
     * statement args: lcn, original_network_id, service_id,
     * region_code, freesat_id
     * (last two 0 for Freeview)
     */
    virtual StatementPtr<id_t, id_t, id_t, id_t, id_t>
    get_insert_client_lcn_statement(const char *source) override;


    virtual StatementPtr<Glib::ustring>
    get_insert_source_statement() override;

    virtual QueryPtr<Vector<id_t>, Glib::ustring>
    get_provider_id_query(const char *source) override;

    /**
     * result fields: lcn
     */
    virtual QueryPtr<Vector<id_t>, void>
    get_network_lcns_query(const char *source) override;

    /**
     * result fields: network_id, service_id, region_code, freesat_id
     * statement args: lcn
     */
    virtual QueryPtr<Vector<id_t, id_t, id_t, id_t>, id_t>
    get_ids_for_network_lcn_query(const char *source) override;

    /**
     * result fields: network_id
     * statement args: network name
     */
    virtual QueryPtr<Vector<id_t>, Glib::ustring>
    get_network_id_for_name_query(const char *source) override;

    /**
     * result fields: network_id, name
     */
    virtual QueryPtr<Vector<id_t, Glib::ustring>, void>
    get_all_network_ids_query(const char *source) override;

    /**
     * result fields: region_code
     * statement args: region name, bouquet_id
     */
    virtual QueryPtr<Vector<id_t>, Glib::ustring, id_t>
    get_region_code_for_name_and_bouquet_query(const char *source) override;

    /**
     * result fields: original_network_id
     * statement args: network_id, service_id
     */
    virtual QueryPtr<Vector<id_t>, id_t, id_t>
    get_original_network_id_for_network_and_service_id_query(const char *source)
    override;

    /**
     * result fields: original_network_id
     * statement args: service_id
     */
    virtual QueryPtr<Vector<id_t>, id_t>
    get_original_network_id_for_service_id_query(const char *source) override;
protected:
    virtual void ensure_network_info_table(const char *source) override;

    virtual void ensure_tuning_table(const char *source) override;

    virtual void ensure_transport_services_table(const char *source) override;

    virtual void ensure_service_id_table(const char *source) override;

    virtual void ensure_service_name_table(const char *source) override;

    virtual void ensure_provider_name_table(const char *source) override;

    virtual void ensure_service_provider_id_table(const char *source)
        override;

    virtual void ensure_network_lcn_table(const char *source) override;

    virtual void ensure_region_table(const char *source) override;

    virtual void ensure_client_lcn_table(const char *source) override;

    virtual void ensure_source_table() override;
private:
    constexpr static auto NETWORK_INFO_TABLE = "network_info";
    constexpr static auto TUNING_TABLE = "tuning";
    constexpr static auto TRANSPORT_SERVICES_TABLE = "transport_services";
    constexpr static auto SERVICE_ID_TABLE = "service_ids";
    constexpr static auto SERVICE_NAME_TABLE = "service_names";
    constexpr static auto PROVIDER_NAME_TABLE = "provider_names";
    constexpr static auto SERVICE_PROVIDER_ID_TABLE = "service_provider_id";
    constexpr static auto NETWORK_LCN_TABLE = "network_lcns";
    constexpr static auto CLIENT_LCN_TABLE = "client_lcns";
    constexpr static auto REGION_TABLE = "regions";
    constexpr static auto SOURCE_TABLE = "sources";

    template<typename... Args>
    StatementPtr<Args...> build_insert_statement(const char *source,
            const char *table, const std::initializer_list<const char *> &keys,
            bool replace = true)
    {
        return std::static_pointer_cast<Statement<Args...>>
            (std::make_shared<Sqlite3Statement<Args...>>
                (sqlite3_, build_insert_sql(source, table, keys, replace)));
    }

    template<typename... Args>
    StatementPtr<Args...> compile_sql_statement(const std::string &sql)
    {
        return std::static_pointer_cast<Statement<Args...>>
            (std::make_shared<Sqlite3Statement<Args...>>(sqlite3_, sql));
    }

    template<class Result, typename... Args>
    QueryPtr<Result, Args...> build_query(const char *source,
            const char *table, const std::initializer_list<const char *> &keys,
            const char *where = nullptr,
            const char *order_by = nullptr)
    {
        return std::static_pointer_cast<Query<Result, Args...>>
            (std::make_shared<Sqlite3Query<Result, Args...>>
                (sqlite3_,
                 build_query_sql(source, table, keys, where, order_by)));
    }

    template<class Result, typename... Args>
    QueryPtr<Result, Args...> compile_sql_query(const std::string &sql)
    {
        return std::static_pointer_cast<Query<Result, Args...>>
            (std::make_shared<Sqlite3Query<Result, Args...>>(sqlite3_, sql));
    }

    void execute(const Glib::ustring &sql);

    static std::string build_table_name(const char *source, const char *name)
    {
        return source ? (std::string(source) + '_' + name) : std::string(name);
    }

    /// If replace == false IGNORE is used instead
    static Glib::ustring build_insert_sql(const char *source,
            const char *table, const std::initializer_list<const char *> &keys,
            bool replace = true);

    static Glib::ustring build_query_sql(const char *source,
            const char *table, const std::initializer_list<const char *> &keys,
            const char *where, const char *order_by);

    /// Each pair is <key, type + constraint (nullable)>
    static Glib::ustring build_create_table_sql(const std::string &name,
            const std::initializer_list<std::pair<const char *, const char *>>
                &columns, const char *constraints = nullptr);

    static Glib::ustring build_create_index_sql(const std::string &table_name,
            const std::string &index_name, const char *details,
            bool unique = false);

    sqlite3 *sqlite3_ = nullptr;

    constexpr static auto INT_PRIM_KEY =
        "INTEGER PRIMARY KEY ON CONFLICT REPLACE";

    constexpr static auto CONFLICT_REPLACE = "ON CONFLICT REPLACE";
};

}
