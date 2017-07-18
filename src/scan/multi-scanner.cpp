/*
    logi - A DVB DVR designed for web-based clients.
    Copyright (C) 2016 Tony Houghton <h@realh.co.uk>

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

#include "channel-scanner.h"
#include "multi-scanner.h"

#include "si/service-list-descriptor.h"
#include "si/terr-delsys-descriptor.h"

namespace logi
{

MultiScanner::MultiScanner(std::shared_ptr<Receiver> rcv,
        std::shared_ptr<ChannelScanner> channel_scanner,
        std::shared_ptr<TuningIterator> iter)
    : rcv_{rcv}, channel_scanner_{channel_scanner}, iter_{iter},
    status_{BLANK}, finished_{false}
{}

void MultiScanner::start()
{
    lock_conn_ = rcv_->lock_signal().connect(
            sigc::mem_fun(*this, &MultiScanner::lock_cb));
    nolock_conn_ = rcv_->nolock_signal().connect(
            sigc::mem_fun(*this, &MultiScanner::nolock_cb));
    next();
}

void MultiScanner::cancel()
{
    bool finished = finished_;

    finished_ = true;

    lock_conn_.disconnect();
    nolock_conn_.disconnect();

    channel_scanner_->cancel();

    if (channel_scanner_->is_complete())
        status_ = COMPLETE;

    if (!finished)
        finished_signal_.emit(status_);
}

void MultiScanner::channel_finished(bool success)
{
    if (current_ts_data_)
    {
        current_ts_data_->set_scan_status(success ?
                TransportStreamData::SCANNED : TransportStreamData::PENDING);
    }

    bool complete = channel_scanner_->is_complete();

    if (complete)
    {
        status_ = COMPLETE;
        cancel();
    }
    else if (success)
    {
        status_ = PARTIAL;
    }
    
    if (!complete)
        next();
}

void MultiScanner::next()
{
    /*
    g_debug("NEXT");
    for (const auto &tspair: ts_data_)
    {
        const auto &tsdat = tspair.second;
        auto props = tsdat.get_tuning();
        g_debug("  id %d/%d, desc %s, status %d",
                tspair.first, tsdat.get_transport_stream_id(),
                props ? props->describe().c_str() : "null",
                tsdat.get_scan_status());
    }
    */

    while (true)
    {
        std::shared_ptr<TuningProperties> props = nullptr;
        current_ts_data_ = nullptr;

        // First look for any discovered (in NIT) transports that haven't been
        // scanned yet.
        for (auto &tsdat: ts_data_)
        {
            if (tsdat.second.get_scan_status() == TransportStreamData::PENDING
                    && (props = tsdat.second.get_tuning()) != nullptr)
            {
                current_ts_data_ = &tsdat.second;
                break;
            }
        }

        // If there's nothing to be scanned in NIT go through the iterator.
        // This loops skips those from iterator which have been discovered.
        if (!props)
        {
            do
            {
                props = iter_->next();
            } while (props && std::find_if(ts_data_.begin(), ts_data_.end(),
                [&props](const std::pair<std::uint16_t, TransportStreamData> &t)
                {
                    auto tuning = t.second.get_tuning();
                    return tuning && *tuning == *props;
                }) != ts_data_.end());
        }

        if (!props)
        {
            cancel();
            return;
        }

        g_print("Tuning to %s... ", props->describe().c_str());
        try
        {
            rcv_->tune(props, 5000);
            break;
        }
        catch (Glib::Exception &x)
        {
            g_log(nullptr, G_LOG_LEVEL_CRITICAL, "%s", x.what().c_str());
        }
    }
}

void MultiScanner::lock_cb()
{
    g_print("Locked\n");
    channel_scanner_->start(this);
}

void MultiScanner::nolock_cb()
{
    if (current_ts_data_)
        current_ts_data_->set_scan_status(TransportStreamData::FAILED);
    g_print("No lock\n");
    next();
}

TransportStreamData &
MultiScanner::get_transport_stream_data(std::uint16_t ts_id)
{
    auto &tsdat = ts_data_[ts_id];
    tsdat.set_transport_stream_id(ts_id);
    current_ts_data_ = &tsdat;
    return tsdat;
}


void MultiScanner::process_service_list_descriptor(std::uint16_t ts_id,
        const Descriptor &desc)
{
    ServiceListDescriptor sd(desc);
    /*
    g_debug("    Services:");
    for (const auto &s: sd.get_services())
    {
        g_debug("      id %04x type %02x", s.service_id(), s.service_type());
    }
    */
    auto &tsdat = get_transport_stream_data(ts_id);
    const auto &svcs = sd.get_services();
    g_debug("    %ld services", svcs.size());
    for (const auto &s: svcs)
    {
        tsdat.add_service_id(s.service_id());
    }
}

void MultiScanner::process_delivery_system_descriptor(std::uint16_t ts_id,
        const Descriptor &desc)
{
    // FIXME: Also need to support satellite later
    TerrestrialDeliverySystemDescriptor d(desc);
    auto &tsdat = get_transport_stream_data(ts_id);
    tsdat.set_tuning(d.get_tuning_properties());
    g_debug("    TS %d: %s", ts_id, tsdat.get_tuning()->describe().c_str());
}

}
