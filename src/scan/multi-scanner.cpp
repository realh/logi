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
    while (true)
    {
        auto props = iter_->next();
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
    g_print("No lock\n");
    next();
}

TransportStreamData &
MultiScanner::get_transport_stream_data(std::uint16_t ts_id)
{
    auto &tsdat = ts_data_[ts_id];
    tsdat.set_transport_stream_id(ts_id);
    return tsdat;
}


void MultiScanner::process_service_list_descriptor(std::uint16_t ts_id,
        const Descriptor &desc)
{
    ServiceListDescriptor sd(desc);
    g_print("    Services:\n");
    for (const auto &s: sd.get_services())
    {
        g_print("      id %04x type %02x\n", s.service_id(), s.service_type());
    }
    auto &tsdat = get_transport_stream_data(ts_id);
    for (const auto &s: sd.get_services())
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
    g_print("    TS %d: %s\n", ts_id, tsdat.get_tuning()->describe().c_str());
}

}
