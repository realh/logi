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
    auto q = db_.get_network_lcns_query(source_.c_str());
    auto lcns = db_.run_query(q);
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
}

}
