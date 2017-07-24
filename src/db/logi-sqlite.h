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
        void bind(int pos, std::uint32_t val);

        void bind(int pos, const Glib::ustring &val);

        /*
        template<std::size_t Pos, class TupleType>
        void bind(const TupleType &tup)
        {
            bind(Pos + 1, std::get<Pos>(tup));
        }
        */

        template<class TupleType, std::size_t... Is>
        void bind_tuple(const TupleType &tup, std::index_sequence<Is...>)
        {
            (bind(Is + 1, std::get<Is>(tup)), ...);
        }

        template<typename... Args>
        void bind_tuple(const std::tuple<Args...> &tup)
        {
            bind_tuple(tup, std::index_sequence_for<Args...>());
        }

        /*
        template<typename T1>
        void bind_tuple(const std::tuple<T1> &tup)
        {
            bind(1, std::get<0>(tup));
        }

        template<typename T1, typename T2>
        void bind_tuple(const std::tuple<T1, T2> &tup)
        {
            bind(1, std::get<0>(tup));
            bind(2, std::get<1>(tup));
        }

        template<typename T1, typename T2, typename T3>
        void bind_tuple(const std::tuple<T1, T2, T3> &tup)
        {
            bind(1, std::get<0>(tup));
            bind(2, std::get<1>(tup));
            bind(3, std::get<2>(tup));
        }

        template<typename T1, typename T2, typename T3, typename T4>
        void bind_tuple (const std::tuple<T1, T2, T3, T4> &tup)
        {
            bind(1, std::get<0>(tup));
            bind(2, std::get<1>(tup));
            bind(3, std::get<2>(tup));
            bind(4, std::get<3>(tup));
        }
        */

        void reset();

        /// Returns true if result is SQLITE_DONE
        bool step();

        void fetch(int pos, std::uint32_t &val);

        void fetch(int pos, Glib::ustring &val);

        template<typename T1> std::tuple<T1> fetch_row()
        {
            T1 v1;
            fetch(0, v1);
            return {v1};
        }

        template<typename T1, typename T2> std::tuple<T1, T2> fetch_row()
        {
            T1 v1;
            T2 v2;
            fetch(0, v1);
            fetch(1, v2);
            return {v1, v2};
        }

        template<typename T1, typename T2, typename T3>
        std::tuple<T1, T2, T3> fetch_row()
        {
            T1 v1;
            T2 v2;
            T3 v3;
            fetch(0, v1);
            fetch(1, v2);
            fetch(2, v3);
            return {v1, v2, v3};
        }

        template<typename T1, typename T2, typename T3, typename T4>
        std::tuple<T1, T2, T3, T4> fetch_row()
        {
            T1 v1;
            T2 v2;
            T3 v3;
            T4 v4;
            fetch(0, v1);
            fetch(1, v2);
            fetch(2, v3);
            fetch(3, v4);
            return {v1, v2, v3, v4};
        }

        /// Takes an out parameter instead of returning a vector to enable
        /// type deduction
        template<typename... Args>
        void fetch_rows(std::vector<std::tuple<Args...> > &v)
        {
            bool result;
            do {
                result = step();
                v.push_back(fetch_row<Args...>());
            } while (!result);
            return v;
        }
    private:
        sqlite3_stmt *stmt_ = nullptr;
    };

#if 0
    /// Uses template recursion to bind all args
    template<std::size_t N, typename... Args> class Binder
    {
    public:
        Binder(Sqlite3StatementBase &s, std::tuple<Args...> &tup)
        {
            Binder<N - 1, Args...> b(s, tup);
            s.bind(N + 1, std::get<N, Args...>(tup));
        }
    };

    /// Specialization of Binder to end recursion at 0
    template<typename... Args> class Binder<0, Args...>
    {
    public:
        Binder(Sqlite3StatementBase &s, std::tuple<Args...> &tup)
        {
            s.bind(1, std::get<0, Args...>(tup));
        }
    };
#endif

    /// Prepends column C of type T from sqlite result to tuple<Args...>,
    /// making a tuple<T, Args...>
    /*
    template<std::size_t C, typename T, typename... Args> class Debinder
    {
        std::tuple<T, Args...> debind(Sqlite3StatementBase &s,
                const std::tuple<Args...> tup) const
        {
            T val;
            s.unbind(C, val);
            return std::tuple_cat({val}, tup);
        }
    };
    */

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

        virtual void execute(typename Parent::ArgsVector &args) override
        {
            for (auto &tup: args)
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
            Result result;
            reset();
            bind_tuple(row);
            fetch_rows(result);
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
     * statement args: network_id, ts_id, tuning prop key, tuning prop value
     */
    virtual StatementPtr<id_t, id_t, id_t, id_t>
        get_insert_tuning_statement(const char *source) override;

    /**
     * statement args: network_id, service_id, ts_id
     */
    virtual StatementPtr<id_t, id_t, id_t, id_t>
        get_insert_service_id_statement(const char *source) override;

    /**
     * statement args: network_id, service_id, name
     */
    virtual StatementPtr<id_t, id_t, Glib::ustring>
        get_insert_service_name_statement(const char *source) override;

    /**
     * statement args: provider_name
     */
    virtual StatementPtr<Glib::ustring>
    get_insert_provider_name_statement(const char *source) override;

    /**
     * statement args: network_id, service_id, rowid from provider_name
     */
    virtual StatementPtr<id_t, id_t, id_t>
    get_insert_service_provider_name_statement(const char *source) override;

    virtual StatementPtr<id_t, id_t, id_t>
    get_insert_primary_lcn_statement(const char *source) override;
protected:
    virtual void ensure_network_info_table(const char *source) override;

    virtual void ensure_tuning_table(const char *source) override;

    virtual void ensure_service_id_table(const char *source) override;

    virtual void ensure_service_name_table(const char *source) override;

    virtual void ensure_provider_name_table(const char *source) override;

    virtual void ensure_service_provider_name_table(const char *source)
        override;

    virtual void ensure_primary_lcn_table(const char *source) override;

    virtual void ensure_sources_table() override;
private:
    template<typename... Args>
    StatementPtr<Args...> build_insert_statement(const char *source,
            const char *table, const std::initializer_list<const char *> &keys,
            bool replace = true)
    {
        return std::static_pointer_cast<Statement<Args...> >
            (std::make_shared<Sqlite3Statement<Args...> >
                (sqlite3_, build_insert_sql(source, table, keys, replace)));
    }

    template<typename... Args>
    StatementPtr<Args...> compile_sql(const std::string &sql)
    {
        return std::static_pointer_cast<Statement<Args...> >
            (std::make_shared<Sqlite3Statement<Args...> >(sqlite3_, sql));
    }

    void execute(const Glib::ustring &sql);

    static std::string build_table_name(const char *source, const char *name)
    {
        return std::string(source) + '_' + name;
    }

    /// If replace == false IGNORE is used instead
    static Glib::ustring build_insert_sql(const char *source,
            const char *table, const std::initializer_list<const char *> &keys,
            bool replace = true);

    /// Each pair is <key, type + constraint (nullable)>
    static Glib::ustring build_create_table_sql(const std::string &name,
            const std::initializer_list<std::pair<const char *, const char *> >
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
