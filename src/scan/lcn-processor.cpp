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

#include "lcn-processor.h"

namespace logi
{

void LCNProcessor::process(const std::string &source,
        const std::string &network_name, const std::string &region_name)
{
    store_names(source, network_name, region_name);
    process();
}

void LCNProcessor::process(const std::string &source,
        const std::string &network_name, const std::string &region_name,
        sigc::slot<void> callback)
{
    store_names(source, network_name, region_name);
    db_.queue_function([this]() { process(); });
    db_.queue_callback(callback);
}

void LCNProcessor::store_names(const std::string &source,
        const std::string &network_name, const std::string &region_name)
{
    source_ = source;
    network_name_ = network_name;
    region_name_ = region_name;
}

void LCNProcessor::process()
{
    auto src = source_.c_str();
    lcn_ids_q_ = db_.get_ids_for_network_lcn_query(src);
    auto qid = db_.run_query(db_.get_network_id_for_name_query(src),
            {network_name_});
    if (qid.size())
        network_id_ = std::get<0>(qid[0]);
    else
        network_id_ = 0;
    qid = db_.run_query(
            db_.get_region_code_for_name_and_bouquet_query(src),
            {region_name_, network_id_});
    if (qid.size())
        region_code_ = std::get<0>(qid[0]);
    else
        region_code_ = 0;
    auto lcns = db_.run_query(db_.get_network_lcns_query(src));
    unsigned last_lcn = 0xffffffff;
    for (const auto &l: lcns)
    {
        unsigned lcn = std::get<0>(l);
        if (lcn != last_lcn)
        {
            last_lcn = lcn;
            process_lcn(lcn);
        }
    }

    auto ins = db_.get_insert_client_lcn_statement(src);
    db_.run_statement(ins, client_lcns_v_);
}

}
