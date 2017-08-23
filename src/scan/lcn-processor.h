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

#include "db/logi-db.h"

namespace logi
{

class LCNProcessor
{
protected:
    using id_t = std::uint32_t;
public:
    LCNProcessor(Database &db) : db_(db)
    {}

    virtual ~LCNProcessor() = default;

    /**
     * Reads data from network_lcns table and others to fill client_lcns table.
     * This version should only be run on the database thread.
     * @param network_name  Bouquet name when Freesat
     * @param region_name   Not used for Freeview
     */
    void process(const std::string &source,
            const std::string &network_name, const std::string &region_name);

    /**
     * This version is for running from another thread.
     * @param callback  Called (on main thread) when done.
     */
    void process(const std::string &source,
            const std::string &network_name, const std::string &region_name,
            sigc::slot<void> callback);
protected:
    virtual void process_lcn(id_t lcn) = 0;

    void process();

    void store_names(const std::string &source,
            const std::string &network_name, const std::string &region_name);

    Database &db_;
    Glib::ustring source_;
    Glib::ustring network_name_;
    Glib::ustring region_name_;
    id_t network_id_;
    id_t region_code_;
    Database::QueryPtr<Database::Vector<id_t, id_t, id_t>, id_t> lcn_ids_q_;
    std::vector<std::tuple<id_t, id_t, id_t>> client_lcns_v_;
};

}
