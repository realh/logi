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
    this_sdt_status_ = other_sdt_status_ = nit_status_ = TableTracker::BLANK;

    // Creation of SDTProcessors is deferred to allow use of a virtual method
    if (this_sdt_proc_)
        this_sdt_proc_->reset_tracker();
    else
        this_sdt_proc_ = new_sdt_processor();
    if (other_sdt_proc_)
        other_sdt_proc_->reset_tracker();
    else
        other_sdt_proc_ = new_sdt_processor();

    for (auto &nwd: networks_)
    {
        nwd.second->nit_proc->reset_tracker();
    }

    nit_filter_.reset(new SectionFilter<NITSection, StandardChannelScanner>(
            multi_scanner->get_receiver(), *this,
            &StandardChannelScanner::nit_filter_cb,
            NIT_PID, 0x40, 0, 5000, 0xff, 0));
    this_sdt_filter_.reset
        (new SectionFilter<SDTSection, StandardChannelScanner>(
            multi_scanner->get_receiver(), *this,
            &StandardChannelScanner::this_sdt_filter_cb,
            SDT_PID, 0x42, 0, 5000, 0xff, 0));
    other_sdt_filter_.reset
        (new SectionFilter<SDTSection, StandardChannelScanner>(
            multi_scanner->get_receiver(), *this,
            &StandardChannelScanner::other_sdt_filter_cb,
            SDT_PID, 0x46, 0, 5000, 0xff, 0));
}

void StandardChannelScanner::cancel()
{
    if (nit_filter_)
    {
        nit_filter_->stop();
        nit_filter_.reset();
    }
    if (this_sdt_filter_)
    {
        this_sdt_filter_->stop();
        this_sdt_filter_.reset();
    }
    if (other_sdt_filter_)
    {
        other_sdt_filter_->stop();
        other_sdt_filter_.reset();
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

void StandardChannelScanner::this_sdt_filter_cb(int reason,
        std::shared_ptr<SDTSection> section)
{
    sdt_filter_cb(reason, section, this_sdt_proc_, this_sdt_filter_,
            this_sdt_status_, "this");
}

void StandardChannelScanner::other_sdt_filter_cb(int reason,
        std::shared_ptr<SDTSection> section)
{
    sdt_filter_cb(reason, section, other_sdt_proc_, other_sdt_filter_,
            other_sdt_status_, "other");
}

TableTracker::Result StandardChannelScanner::sdt_filter_cb(int reason,
        std::shared_ptr<SDTSection> section,
        std::unique_ptr<SDTProcessor> &sdt_proc,
        sdt_filter_ptr &sdt_filter,
        TableTracker::Result &sdt_status,
        const char *label)
{
    auto result = TableTracker::ERROR;
    if (reason)
    {
        g_critical("SDT (%s ts) filter error: %s\n",
                label, std::strerror(reason));
        sdt_status = TableTracker::ERROR;
    }
    else if (section)
    {
        result = sdt_proc->process(section, multi_scanner_);
        switch (result)
        {
            case TableTracker::REPEAT_COMPLETE:
                g_debug("SDT (%s ts) REPEAT_COMPLETE", label);
                sdt_status = result;
                break;
            case TableTracker::COMPLETE:
                g_debug("SDT (%s ts) COMPLETE", label);
                sdt_status = result;
                break;
            default:
                g_debug("SDT (%s ts) %d\n", label, result);
                if (sdt_status == TableTracker::BLANK)
                    sdt_status = result;
                break;
        }
    }

    if (!section || sdt_status == TableTracker::COMPLETE
            || sdt_status == TableTracker::REPEAT_COMPLETE)
    {
        sdt_filter->stop();
        g_debug("Testing completeness for SDT (%s ts)", label);
        if (filter_trackers_complete())
            finished(any_complete());
    }
    return result;
}

std::unique_ptr<NITProcessor> StandardChannelScanner::new_nit_processor()
{
    return std::unique_ptr<NITProcessor>(new NITProcessor());
}

std::unique_ptr<SDTProcessor> StandardChannelScanner::new_sdt_processor()
{
    return std::unique_ptr<SDTProcessor>(new SDTProcessor());
}

bool StandardChannelScanner::any_complete() const
{
    return std::any_of(networks_.begin(), networks_.end(),
            [](ConstNwPair &n)->bool
    {
        return n.second->nit_complete;
    });
}

bool StandardChannelScanner::filter_trackers_complete() const
{
    if (nit_status_ != TableTracker::COMPLETE &&
            nit_status_ != TableTracker::ERROR)
    {
        return false;
    }
    if ((this_sdt_status_ == TableTracker::COMPLETE ||
            this_sdt_status_ == TableTracker::REPEAT_COMPLETE ||
            this_sdt_status_ == TableTracker::ERROR)
        && (other_sdt_status_ == TableTracker::COMPLETE ||
            other_sdt_status_ == TableTracker::REPEAT_COMPLETE || 
            other_sdt_status_ == TableTracker::ERROR))
    {
        return true;
    }
    return false;
    // If one SDT is complete and repeating while the other's filter hasn't
    // received any data it probably means the latter isn't present
    /*
    g_debug("this_sdt_status_ %d, other_sdt_status_ %d",
            this_sdt_status_, other_sdt_status_);
    return (this_sdt_status_ == TableTracker::REPEAT_COMPLETE &&
            other_sdt_status_ == TableTracker::BLANK)
        || (other_sdt_status_ == TableTracker::REPEAT_COMPLETE &&
            this_sdt_status_ == TableTracker::BLANK);
    */
}

}
