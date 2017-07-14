/*
    logi - A DVB DVR designed for web-based clients.
    Copyright (C) 2016-2017 Tony Houghton <h@realh.co.uk>

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
#include <cstring>

#include <glib.h>

#include "multi-scanner.h"
#include "standard-channel-scanner.h"

namespace logi
{

void StandardChannelScanner::start(MultiScanner *multi_scanner)
{
    multi_scanner_ = multi_scanner;
    frontend_ = multi_scanner->get_frontend();

    nit_filter_.reset(new SectionFilter<NITSection, StandardChannelScanner>(
            multi_scanner->get_receiver(), *this,
            &StandardChannelScanner::nit_filter_cb,
            NIT_PID, 0x40, 0, 5000, 0xff, 0));
}

void StandardChannelScanner::cancel()
{
    if (nit_filter_)
    {
        nit_filter_->stop();
        nit_filter_.reset();
    }
}

bool StandardChannelScanner::is_complete() const
{
    return false;
}

void StandardChannelScanner::nit_filter_cb(int reason,
        std::shared_ptr<NITSection> section)
{
    NetworkData *nd = nullptr;

    if (section)
    {
        auto network_id = section->network_id();
        auto ndi = networks_.find(network_id);

        if (ndi == networks_.end())
        {
            networks_[network_id] =
                std::unique_ptr<NetworkData>(nd = new NetworkData());
        }
        else
        {
            nd = ndi->second.get();
        }
    }

    if (reason)
    {
        g_log(nullptr, G_LOG_LEVEL_CRITICAL, 
                "NIT filter error: %s\n", std::strerror(reason));
        nit_error_ = true;
    }
    else if (nd)
    {
        nit_error_ = false;
        nd->nit_complete = nd->nit_proc.process(section);
    }

    if (!section || all_complete_or_error())
    {
        nit_filter_->stop();
        finished(any_complete());
    }
}

bool StandardChannelScanner::all_complete_or_error() const
{
    if (nit_error_)
        return true;
    return std::all_of(networks_.begin(), networks_.end(),
            [](ConstNwPair &n)->bool
    {
        return n.second->nit_complete;
    });
}

bool StandardChannelScanner::any_complete() const
{
    return std::any_of(networks_.begin(), networks_.end(),
            [](ConstNwPair &n)->bool
    {
        return n.second->nit_complete;
    });
}

}
