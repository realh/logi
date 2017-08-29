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

#include <algorithm>

#include "freesat-lcn-processor.h"

namespace logi
{

void FreesatLCNProcessor::process_lcn(id_t lcn)
{
    //g_print("LCN %d\n", lcn);
    auto lcn_ids = db_.run_query(lcn_ids_q_, {lcn});
    if (!lcn_ids.size())
    {
        g_warning("No data for lcn %d", lcn);
        return;
    }

    // First try to find a match for both bouquet and region
    auto it = std::find(lcn_ids.begin(), lcn_ids.end(),
    // fields: network_id, service_id, region_code, freesat_id
    //[this](const std::tuple<id_t, id_t, id_t, id_t> &lids)
    [this](const auto &lids)
    {
        return std::get<0>(lids) == network_id_ &&
            std::get<2>(lids) == region_code_;
    });

    // If we can't find a perfect match, 65535 seems to be a universal region
    if (it == lcn_ids.end())
    {
        it = std::find(lcn_ids.begin(), lcn_ids.end(),
        [this](const auto &lids)
        {
            return std::get<0>(lids) == network_id_ &&
                std::get<2>(lids) == 65535;
        });
    }

    // BBC 1 and 2 HD are weird, using different combinations of 101/102/106/108
    // depending on the bouquet. For "England HD" lcn 106 is assigned to
    // BBC One HD NI, Scotland or Wales variants, but not English (which is on
    // 972). Special-case code that can deal with all bouquets would be too
    // complicated, but we can at least make a somewhat random assignment to
    // 106/108, using a region code of 0.
    if (it == lcn_ids.end())
    {
        it = std::find(lcn_ids.begin(), lcn_ids.end(),
        [this](const auto &lids)
        {
            return std::get<0>(lids) == network_id_ &&
                std::get<2>(lids) == 0;
        });
    }

    if (it == lcn_ids.end())
    {
        // FIXME: Rather than downgrading this to a debug I should filter
        // all_lcns query for the target bouquet
        g_warning("No mapping for lcn '%d' in '%s'",
                lcn, network_name_.c_str());
        return;
    }
    // Freesat (currently) only has two distinct original_network_id values,
    // 2 for TV/radio, 59 for data etc, so in the absence of a
    // transport_services mapping we can just look up the onw_id for a
    // service_id in service_ids.
    if (!onw_id_q_)
    {
        onw_id_q_ = db_.get_original_network_id_for_service_id_query
            (source_.c_str());
    }
    auto onw = db_.run_query(onw_id_q_, {std::get<1>(*it)});
    if (!onw.size())
    {
        g_warning("Can't find service_id %d (for lcn %d)",
                std::get<1>(*it), lcn);
        return;
    }

    client_lcns_v_.emplace_back(lcn, std::get<0>(onw[0]),
            std::get<1>(*it), region_code_, std::get<3>(*it));
}

}
