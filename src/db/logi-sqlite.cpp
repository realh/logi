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

#include <glibmm.h>

#include <sqlite3.h>

#include "logi-sqlite.h"

namespace logi
{

Sqlite3Error::Sqlite3Error(sqlite3 *db, int code, const Glib::ustring &desc) :
    Sqlite3Error(code, desc + ": " + sqlite3_errmsg(db))
{}

Sqlite3Error::Sqlite3Error(sqlite3 *db, int code, Glib::ustring &&desc) :
    Sqlite3Error(code, desc + ": " + sqlite3_errmsg(db))
{}

Sqlite3Database::Sqlite3StatementBase::Sqlite3StatementBase
    (sqlite3 *db, const char *sql)
{ 
    int result = sqlite3_prepare_v2(db, sql, -1, &stmt_, nullptr);
    if (result != SQLITE_OK)
    {
        throw Sqlite3Error(db, result,
                Glib::ustring("Error compiling SQL {") + sql + "}");
    }
}

Sqlite3Database::Sqlite3StatementBase::~Sqlite3StatementBase()
{
    if (stmt_)
    {
        sqlite3_finalize(stmt_);
        stmt_ = nullptr;
    }
}

void Sqlite3Database::Sqlite3StatementBase::bind(int pos, std::uint32_t val)
{
    int result = sqlite3_bind_int(stmt_, pos, val);
    if (result != SQLITE_OK)
    {
        throw Sqlite3Error(sqlite3_db_handle(stmt_),
                result, "Error binding SQL int");
    }
}

void Sqlite3Database::Sqlite3StatementBase::bind(int pos,
        const Glib::ustring &val)
{
    int result = sqlite3_bind_text
        (stmt_, pos, val.c_str(), -1, SQLITE_TRANSIENT);
    if (result != SQLITE_OK)
    {
        throw Sqlite3Error(sqlite3_db_handle(stmt_),
                result, "Error binding SQL string");
    }
}

void Sqlite3Database::Sqlite3StatementBase::reset()
{
    sqlite3_clear_bindings(stmt_);
    sqlite3_reset(stmt_);
}

bool Sqlite3Database::Sqlite3StatementBase::step()
{
    int result;
    while ((result = sqlite3_step(stmt_)) == SQLITE_BUSY)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    if (result != SQLITE_ROW && result != SQLITE_DONE)
    {
        throw Sqlite3Error(sqlite3_db_handle(stmt_),
                result, "Error binding SQL string");
    }
    return result == SQLITE_DONE;
}

Sqlite3Database::~Sqlite3Database()
{
    if (sqlite3_)
    {
        sqlite3_close(sqlite3_);
        sqlite3_ = nullptr;
    }
}

void Sqlite3Database::open()
{
    std::string filename = Glib::build_filename(Glib::get_user_data_dir(),
            "logi", "database.sqlite3");
    int result = sqlite3_open(filename.c_str(), &sqlite3_);
    if (result != SQLITE_OK)
    {
        if (sqlite3_)
        {
            Glib::ustring e = sqlite3_errmsg(sqlite3_);
            sqlite3_close(sqlite3_);
            throw Sqlite3Error(result, Glib::ustring("Error opening database '"
                        + filename + "': " + e));
        }
        else
        {
            throw Sqlite3Error(result, Glib::ustring("Error opening database '"
                        + filename + "'"));
        }
    }
}

Database::StatementPtr<id_t, Glib::ustring>
Sqlite3Database::get_insert_network_info_statement(const char *source)
{
    return build_insert_statement<id_t, Glib::ustring>(source, "network_info",
            {"network_id", "name"});
}

Database::StatementPtr<id_t, id_t, id_t, id_t>
Sqlite3Database::get_insert_tuning_statement(const char *source)
{
    return build_insert_statement<id_t, id_t, id_t, id_t>(source, "tuning",
            {"original_network_id", "ts_id", "tuning_key", "tuning_val"});
}

Database::StatementPtr<id_t, id_t, id_t, id_t>
Sqlite3Database::get_insert_service_id_statement(const char *source)
{
    return build_insert_statement<id_t, id_t, id_t, id_t>(source, "service_id",
            {"original_network_id", "service_id", "ts_id", "service_type"});
}

Database::StatementPtr<id_t, id_t, Glib::ustring>
Sqlite3Database::get_insert_service_name_statement(const char *source)
{
    return build_insert_statement<id_t, id_t, Glib::ustring>(source,
            "service_name",
            {"original_network_id", "service_id", "name"});
}

Database::StatementPtr<Glib::ustring>
Sqlite3Database::get_insert_provider_name_statement(const char *source)
{
    return build_insert_statement<Glib::ustring>(source,
            "provider_name", {"provider_name"}, false);
}

Database::StatementPtr<id_t, id_t, Glib::ustring>
Sqlite3Database::get_insert_service_provider_name_statement(const char *source)
{
    return compile_sql<id_t, id_t, Glib::ustring>(
        Glib::ustring("INSERT OR REPLACE INTO ") +
        build_table_name(source, "service_provider_name") +
        " (original_network_id, service_id, provider_id)"
        " VALUES (?, ?, SELECT rowid FROM " +
        build_table_name(source, "provider_name") +
        " WHERE provider_name = ?)");
}

Database::StatementPtr<id_t, id_t, id_t>
Sqlite3Database::get_insert_primary_lcn_statement(const char *source)
{
    return build_insert_statement<id_t, id_t, id_t>(source,
            "primary_lcn",
            {"network_id", "service_id", "lcn"});
}


void Sqlite3Database::ensure_network_info_table(const char *source)
{
    auto table_name = build_table_name(source, "network_info");
    execute(build_create_table_sql(table_name, {
            {"network_id", INT_PRIM_KEY},
            {"name", "TEXT"}
    }));
    // index is superfluous when we have an integer primary key
}

void Sqlite3Database::ensure_tuning_table(const char *source)
{
    auto table_name = build_table_name(source, "tuning");
    execute(build_create_table_sql(table_name, {
            {"original_network_id", "INTEGER"},
            {"transport_stream_id", "INTEGER"},
            {"tuning_key", "INTEGER"},
            {"tuning_val", "INTEGER"},
        },
        "PRIMARY KEY (original_network_id, transport_stream_id, tuning_key)"));
    execute(build_create_index_sql(table_name, table_name + '_' + "index",
                "(original_network_id, transport_stream_id)"));
}

void Sqlite3Database::ensure_transport_services_table(const char *source)
{
    auto table_name = build_table_name(source, "transport_services");
    execute(build_create_table_sql(table_name, {
            {"original_network_id", "INTEGER"},
            {"network_id", "INTEGER"},
            {"transport_stream_id", "INTEGER"},
            {"service_id", "INTEGER"},
        },
        "PRIMARY KEY (original_network_id, network_id, transport_stream_id)"));
}

void Sqlite3Database::ensure_service_id_table(const char *source)
{
    auto table_name = build_table_name(source, "service_id");
    execute(build_create_table_sql(table_name, {
            {"original_network_id", "INTEGER"},
            {"transport_stream_id", "INTEGER"},
            {"service_id", "INTEGER"},
            {"service_type", "INTEGER"},
        },
        "PRIMARY KEY (original_network_id, service_id)"));
}

void Sqlite3Database::ensure_service_name_table(const char *source)
{
    auto table_name = build_table_name(source, "service_name");
    execute(build_create_table_sql(table_name, {
            {"original_network_id", "INTEGER"},
            {"service_id", "INTEGER"},
            {"name", "TEXT"},
        },
        "PRIMARY KEY (original_network_id, service_id)"));
}

void Sqlite3Database::ensure_provider_name_table(const char *source)
{
    auto table_name = build_table_name(source, "provider_name");
    execute(build_create_table_sql(table_name, {
            {"provider_name", "TEXT PRIMARY KEY ON CONFLICT IGNORE"},
        }));
}

void Sqlite3Database::ensure_service_provider_name_table(const char *source)
{
    auto table_name = build_table_name(source, "service_provider_name");
    execute(build_create_table_sql(table_name, {
            {"original_network_id", "INTEGER"},
            {"service_id", "INTEGER"},
            {"provider_id", "INTEGER"},
        },
        "PRIMARY KEY (original_network_id, service_id)"));
}

void Sqlite3Database::ensure_primary_lcn_table(const char *source)
{
    auto table_name = build_table_name(source, "primary_lcn");
    execute(build_create_table_sql(table_name, {
            {"network_id", "INTEGER"},
            {"service_id", "INTEGER"},
            {"lcn", "INTEGER"},
        },
        "PRIMARY KEY (network_id, lcn)"));
}

void Sqlite3Database::ensure_sources_table()
{
    execute(build_create_table_sql("sources", {
            {"source_name", "TEXT"},
        }));
}

void Sqlite3Database::execute(const Glib::ustring &sql)
{
    sqlite3_stmt *stmt;
    int result = sqlite3_prepare_v2(sqlite3_, sql.c_str(), -1, &stmt, nullptr);
    if (result != SQLITE_OK)
    {
        throw Sqlite3Error(sqlite3_, result,
                Glib::ustring("Error compiling SQL {") + sql + "}");
    }
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

Glib::ustring Sqlite3Database::build_insert_sql(const char *source,
        const char *table, const std::initializer_list<const char *> &keys,
        bool replace)
{
    auto s = Glib::ustring("INSERT OR ") + (replace ? "REPLACE" : "IGNORE") +
        " INTO " + build_table_name(source, table) + " (";
    Glib::ustring p;

    auto it = keys.begin();
    auto end = keys.end();
    while (it != end)
    {
        s += *it;
        if (++it != end)
        {
            s += ", ";
            p += "?, ";
        }
        else
        {
            p += "?";
        }
    }
    return s + ") VALUES (" + p + ")";
}

Glib::ustring Sqlite3Database::build_create_table_sql(const std::string &name,
        const std::initializer_list<std::pair<const char *, const char *> >
            &columns, const char *constraints)
{
    auto s = Glib::ustring("CREATE TABLE IF NOT EXISTS ") + name + " (";
    auto it = columns.begin();
    auto end = columns.end();
    while (it != end)
    {
        s += it->first;
        if (it->second)
        {
            s += ' ';
            s += it->second;
        }
        if (++it != end)
        {
            s += ", ";
        }
    }
    if (constraints)
    {
        s += ", ";
        s += constraints;
    }
    return s + ')';
}

Glib::ustring Sqlite3Database::build_create_index_sql(
        const std::string &table_name, const std::string &index_name,
        const char *details, bool unique)
{
    auto s = Glib::ustring("CREATE");
    if (unique)
        s += " UNIQUE";
    s += " INDEX IF NOT EXISTS ";
    s += index_name;
    s += " ON ";
    s += table_name;
    s += ' ';
    return s + details;
}

}
