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
#include "single-channel-scanner.h"

namespace logi
{

void SingleChannelScanner::start(MultiScanner *multi_scanner)
{
    multi_scanner_ = multi_scanner;
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

    have_current_ts_id_ = false;

    std::uint16_t pid;
    std::uint8_t table_id;

    pid = Section::NIT_PID;
    table_id = Section::NIT_TABLE;
    if (get_filter_params(pid, table_id))
    {
        nit_filter_.reset(new SectionFilter<NITSection, SingleChannelScanner>(
                multi_scanner->get_receiver(), *this,
                &SingleChannelScanner::nit_filter_cb,
                pid, table_id, 0, 5000, 0xff, 0));
    }
    else
    {
        nit_filter_.reset();
    }
    pid = Section::SDT_PID;
    table_id = Section::SDT_TABLE;
    if (get_filter_params(pid, table_id))
    {
        this_sdt_filter_.reset(
                new SectionFilter<SDTSection, SingleChannelScanner>(
                multi_scanner->get_receiver(), *this,
                &SingleChannelScanner::this_sdt_filter_cb,
                pid, table_id, 0, 5000, 0xff, 0));
    }
    else
    {
        this_sdt_filter_.reset();
    }
    pid = Section::SDT_PID;
    table_id = Section::OTHER_SDT_TABLE;
    if (get_filter_params(pid, table_id))
    {
        other_sdt_filter_.reset(
                new SectionFilter<SDTSection, SingleChannelScanner>(
                multi_scanner->get_receiver(), *this,
                &SingleChannelScanner::other_sdt_filter_cb,
                pid, table_id, 0, 5000, 0xff, 0));
    }
    else
    {
        other_sdt_filter_.reset();
    }
}

void SingleChannelScanner::cancel()
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

NetworkData *
SingleChannelScanner::get_network_data(std::uint16_t network_id)
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

void SingleChannelScanner::nit_filter_cb(int reason,
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
            nit_status_ = TableTracker::REPEAT_COMPLETE;
        else if (nit_status_ == TableTracker::BLANK)
            nit_status_ = TableTracker::OK;
    }

    if (!section || nit_status_ == TableTracker::REPEAT_COMPLETE
        || nit_status_ == TableTracker::ERROR)
    {
        nit_filter_->stop();
        g_debug("Testing completeness for NIT");
        if (filter_trackers_complete())
            finished(any_complete());
    }
}

void SingleChannelScanner::this_sdt_filter_cb(int reason,
        std::shared_ptr<SDTSection> section)
{
    // Freeview HD channels' tuning frequencies are not given in NIT, so we can
    // only get them by scanning through all channels and matching current
    // frequency with ts_id found in SDT.
    if (section && !have_current_ts_id_)
    {
        auto &ts = multi_scanner_->get_transport_stream_data(
                section->original_network_id(), section->transport_stream_id());
        if (!ts.get_tuning())
            ts.set_tuning(multi_scanner_->get_receiver()->current_tuning());
        ts.set_scan_status(TransportStreamData::SCANNED);
        have_current_ts_id_ = true;
        g_print("Current ts_id %d\n", section->transport_stream_id());
    }
    sdt_filter_cb(reason, section, this_sdt_proc_, this_sdt_filter_,
            this_sdt_status_, "this");
}

void SingleChannelScanner::other_sdt_filter_cb(int reason,
        std::shared_ptr<SDTSection> section)
{
    sdt_filter_cb(reason, section, other_sdt_proc_, other_sdt_filter_,
            other_sdt_status_, "other");
}

TableTracker::Result SingleChannelScanner::sdt_filter_cb(int reason,
        std::shared_ptr<SDTSection> section,
        std::unique_ptr<SDTProcessor> &sdt_proc,
        SDTFilterPtr &sdt_filter,
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

    if (!section || sdt_status == TableTracker::REPEAT_COMPLETE)
    {
        sdt_filter->stop();
        g_debug("Testing completeness for SDT (%s ts)", label);
        if (filter_trackers_complete())
            finished(any_complete());
    }
    return result;
}

std::unique_ptr<NITProcessor> SingleChannelScanner::new_nit_processor()
{
    return std::unique_ptr<NITProcessor>(new NITProcessor());
}

std::unique_ptr<SDTProcessor> SingleChannelScanner::new_sdt_processor()
{
    return std::unique_ptr<SDTProcessor>(new SDTProcessor());
}

bool SingleChannelScanner::any_complete() const
{
    return NetworkData::any_complete(networks_);
}

bool SingleChannelScanner::filter_trackers_complete() const
{
    if (nit_filter_ && nit_status_ != TableTracker::COMPLETE &&
            nit_status_ != TableTracker::REPEAT_COMPLETE &&
            nit_status_ != TableTracker::ERROR)
    {
        g_debug("Not complete because nit_status = %d", nit_status_);
        return false;
    }
    if (this_sdt_filter_ && this_sdt_status_ != TableTracker::COMPLETE &&
            this_sdt_status_ != TableTracker::REPEAT_COMPLETE &&
            this_sdt_status_ == TableTracker::ERROR)
    {
        g_debug("Not complete because this_sdt_status = %d", this_sdt_status_);
        return false;
    }
    if (other_sdt_filter_ && other_sdt_status_ != TableTracker::COMPLETE &&
            other_sdt_status_ != TableTracker::REPEAT_COMPLETE &&
            other_sdt_status_ == TableTracker::ERROR)
    {
        g_debug("Not complete because other_sdt_status = %d",
                other_sdt_status_);
        return false;
    }
    return true;
}

void SingleChannelScanner::finished(bool success)
{
    multi_scanner_->channel_finished(success);
}

SingleChannelScanner::CheckHarvestPolicy
SingleChannelScanner::check_harvest_policy() const
{
    return SCAN_ALL_DISCOVERED_TS;
}

bool SingleChannelScanner::get_filter_params(std::uint16_t &, std::uint8_t &)
{
    return true;
}

void SingleChannelScanner::commit_extras_to_database(class Database &,
        const char *)
{
}

}
