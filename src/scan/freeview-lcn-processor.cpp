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

#include "freeview-lcn-processor.h"

namespace logi
{

void FreeviewLCNProcessor::process_lcn(id_t lcn)
{
    //g_print("LCN %d\n", lcn);
    auto lcn_ids = db_.run_query(lcn_ids_q_, {lcn});
    if (!lcn_ids.size())
    {
        g_warning("No data for lcn %d", lcn);
        return;
    }
    // For Freeview we need to match lcn's network_id to services'
    // orignal_network_id which we can do via transport_streams table
    if (!nw_id_q_)
    {
        nw_id_q_ = db_.get_original_network_id_for_network_and_service_id_query
            (source_.c_str());
    }

    auto onwr = db_.run_query(nw_id_q_,
            {std::get<0>(lcn_ids[0]), std::get<1>(lcn_ids[0])});
    if (!onwr.size())
    {
        g_warning("Can't find original_network_id for lcn %d (ids %d/%d)",
                lcn, std::get<0>(lcn_ids[0]), std::get<1>(lcn_ids[0]));
        return;
    }
    client_lcns_v_.emplace_back(lcn,
            std::get<0>(onwr[0]), std::get<1>(lcn_ids[0]));
}

}
