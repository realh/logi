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

#include <cstring>

#include <glib.h>

#include "multi-scanner.h"
#include "freesat-channel-scanner.h"

namespace logi
{

void FreesatChannelScanner::start(MultiScanner *multi_scanner)
{
    multi_scanner_ = multi_scanner;
    frontend_ = multi_scanner->get_frontend();
    sdt_status_ = nit_status_ = TableTracker::BLANK;

    // Creation of SDTProcessors is deferred to allow use of a virtual method
    if (sdt_proc_)
        sdt_proc_->reset_tracker();
    else
        sdt_proc_ = new_sdt_processor();
    if (sdt_proc_)
        sdt_proc_->reset_tracker();
    else
        sdt_proc_ = new_sdt_processor();

    for (auto &nwd: networks_)
    {
        nwd.second->nit_proc->reset_tracker();
    }

    nit_filter_.reset(new SectionFilter<NITSection, FreesatChannelScanner>(
            multi_scanner->get_receiver(), *this,
            &FreesatChannelScanner::nit_filter_cb,
            FS_NIT_PID, Section::NIT_TABLE, 0, 5000, 0xff, 0));
    sdt_filter_.reset
        (new SectionFilter<SDTSection, FreesatChannelScanner>(
            multi_scanner->get_receiver(), *this,
            &FreesatChannelScanner::sdt_filter_cb,
            FS_SDT_PID, Section::SDT_TABLE, 0, 5000, 0xff, 0));
}

void FreesatChannelScanner::cancel()
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

NetworkData *
FreesatChannelScanner::get_network_data(std::uint16_t network_id)
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

void FreesatChannelScanner::nit_filter_cb(int reason,
        std::shared_ptr<NITSection> section)
{
    NetworkData *nd = section ?
        get_network_data(section->network_id()) : nullptr;

    if (reason)
    {
        g_critical("NIT filter error: %s\n", std::strerror(reason));
        nit_status_ = TableTracker::ERROR;
    }
    else if (nd)
    {
        nd->nit_complete = nd->nit_proc->process(section, multi_scanner_);
        if (nd->nit_complete)
            nit_status_ = TableTracker::COMPLETE;
        else if (nit_status_ == TableTracker::BLANK)
            nit_status_ = TableTracker::OK;
    }

    if (!section || nit_status_ == TableTracker::COMPLETE
        || nit_status_ == TableTracker::ERROR)
    {
        nit_filter_->stop();
        g_debug("Testing completeness for NIT");
        if (filter_trackers_complete())
            finished(any_complete());
    }
}

void FreesatChannelScanner::sdt_filter_cb(int reason,
        std::shared_ptr<SDTSection> section)
{
    auto result = TableTracker::ERROR;
    if (reason)
    {
        g_critical("SDT filter error: %s", std::strerror(reason));
        sdt_status_ = TableTracker::ERROR;
    }
    else if (section)
    {
        result = sdt_proc_->process(section, multi_scanner_);
        switch (result)
        {
            case TableTracker::REPEAT_COMPLETE:
                g_debug("SDT REPEAT_COMPLETE");
                sdt_status_ = result;
                break;
            case TableTracker::COMPLETE:
                g_debug("SDT COMPLETE");
                sdt_status_ = result;
                break;
            default:
                g_debug("SDT %d", result);
                if (sdt_status_ == TableTracker::BLANK)
                    sdt_status_ = result;
                break;
        }
    }

    if (!section || sdt_status_ == TableTracker::COMPLETE
            || sdt_status_ == TableTracker::REPEAT_COMPLETE)
    {
        sdt_filter_->stop();
        g_debug("Testing completeness for SDT");
        if (filter_trackers_complete())
            finished(any_complete());
    }
}

std::unique_ptr<NITProcessor> FreesatChannelScanner::new_nit_processor()
{
    return std::unique_ptr<NITProcessor>(new NITProcessor());
}

std::unique_ptr<SDTProcessor> FreesatChannelScanner::new_sdt_processor()
{
    return std::unique_ptr<SDTProcessor>(new SDTProcessor());
}

bool FreesatChannelScanner::any_complete() const
{
    return NetworkData::any_complete(networks_);
}

bool FreesatChannelScanner::filter_trackers_complete() const
{
    if (nit_status_ != TableTracker::COMPLETE &&
            nit_status_ != TableTracker::ERROR)
    {
        return false;
    }
    if ((sdt_status_ == TableTracker::COMPLETE ||
            sdt_status_ == TableTracker::REPEAT_COMPLETE ||
            sdt_status_ == TableTracker::ERROR))
    {
        return true;
    }
    return false;
}

}
