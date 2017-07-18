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
    filter_error_ = false;
    sdt_complete_ = false;

    // Creation of SDTProcessor is deferred to allow use of a virtual method
    // and because TableTrackers need resetting for each channel
    sdt_proc_ = new_sdt_processor();

    nit_filter_.reset(new SectionFilter<NITSection, StandardChannelScanner>(
            multi_scanner->get_receiver(), *this,
            &StandardChannelScanner::nit_filter_cb,
            NIT_PID, 0x40, 0, 5000, 0xff, 0));
    sdt_filter_.reset(new SectionFilter<SDTSection, StandardChannelScanner>(
            multi_scanner->get_receiver(), *this,
            &StandardChannelScanner::sdt_filter_cb,
            SDT_PID, 0x42, 0, 5000, 0xfb, 0));
            // mask gets 0x42 and 0x46
}

void StandardChannelScanner::cancel()
{
    if (nit_filter_)
    {
        nit_filter_->stop();
        nit_filter_.reset();
    }
    if (sdt_filter_)
    {
        sdt_filter_->stop();
        sdt_filter_.reset();
    }
}

bool StandardChannelScanner::is_complete() const
{
    return false;
}

StandardChannelScanner::NetworkData *
StandardChannelScanner::get_network_data(std::uint16_t network_id)
{
    auto ndi = networks_.find(network_id);
    NetworkData *nd = nullptr;

    if (ndi == networks_.end())
    {
        networks_[network_id] = std::unique_ptr<NetworkData>
                (nd = new NetworkData(new_nit_processor()));
    }
    else
    {
        nd = ndi->second.get();
    }
    return nd;
}

void StandardChannelScanner::nit_filter_cb(int reason,
        std::shared_ptr<NITSection> section)
{
    NetworkData *nd = section ?
        get_network_data(section->network_id()) : nullptr;

    if (reason)
    {
        g_log(nullptr, G_LOG_LEVEL_CRITICAL, 
                "NIT filter error: %s\n", std::strerror(reason));
        filter_error_ = true;
    }
    else if (nd)
    {
        nd->nit_complete = nd->nit_proc->process(section, multi_scanner_);
    }

    if (!section || all_complete_or_error())
    {
        nit_filter_->stop();
        finished(any_complete());
    }
}

void StandardChannelScanner::sdt_filter_cb(int reason,
        std::shared_ptr<SDTSection> section)
{
    if (reason)
    {
        g_log(nullptr, G_LOG_LEVEL_CRITICAL, 
                "NIT filter error: %s\n", std::strerror(reason));
        filter_error_ = true;
    }
    else if (section)
    {
        sdt_complete_ = sdt_proc_->process(section, multi_scanner_);
    }

    if (!section || all_complete_or_error())
    {
        sdt_filter_->stop();
        finished(any_complete());
    }
}

std::unique_ptr<NITProcessor> StandardChannelScanner::new_nit_processor()
{
    return std::unique_ptr<NITProcessor>(new NITProcessor());
}

std::unique_ptr<SDTProcessor> StandardChannelScanner::new_sdt_processor()
{
    return std::unique_ptr<SDTProcessor>(new SDTProcessor());
}

bool StandardChannelScanner::all_complete_or_error() const
{
    if (filter_error_)
        return true;
    return sdt_complete_ &&
        std::all_of(networks_.begin(), networks_.end(),
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
