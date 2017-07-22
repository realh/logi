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

int Sqlite3Database::Sqlite3StatementBase::step()
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
    return result;
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

Database::StatementPtr<void, id_t, Glib::ustring>
Sqlite3Database::get_insert_network_info_statement(const char *source)
{
    return build_insert_statement<id_t, Glib::ustring>(source, "network_info",
            {"network_id", "name"});
}

Database::StatementPtr<void, id_t, id_t>
Sqlite3Database::get_insert_transport_stream_info_statement(const char *source)
{
    return build_insert_statement<id_t, id_t>(source, "transport_stream_info",
            {"network_id", "ts_id"});
}

Database::StatementPtr<void, id_t, id_t, id_t, id_t>
Sqlite3Database::get_insert_tuning_statement(const char *source)
{
    return build_insert_statement<id_t, id_t, id_t, id_t>(source, "tuning",
            {"network_id", "ts_id", "tuning_key", "tuning_val"});
}

Database::StatementPtr<void, id_t, id_t, id_t>
Sqlite3Database::get_insert_service_id_statement(const char *source)
{
    return build_insert_statement<id_t, id_t, id_t>(source, "service_id",
            {"network_id", "service_id", "ts_id"});
}

Database::StatementPtr<void, id_t, id_t, Glib::ustring>
Sqlite3Database::get_insert_service_name_statement(const char *source)
{
    return build_insert_statement<id_t, id_t, Glib::ustring>(source,
            "service_name",
            {"network_id", "service_id", "name"});
}

Database::StatementPtr<void, id_t, id_t, Glib::ustring>
Sqlite3Database::get_insert_service_provider_name_statement(const char *source)
{
    return build_insert_statement<id_t, id_t, Glib::ustring>(source,
            "service_name",
            {"network_id", "service_id", "provider_name"});
}

Glib::ustring Sqlite3Database::build_insert_sql(const char *source,
        const char *table, const std::initializer_list<const char *> &keys)
{
    auto s = Glib::ustring("INSERT OR REPLACE INTO ") +
        source + "_" + table + " (";
    Glib::ustring p;

    auto it = keys.begin();
    auto end = keys.end();
    while (it != keys.end())
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

}
