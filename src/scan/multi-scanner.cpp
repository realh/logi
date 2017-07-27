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

#include "si/network-name-descriptor.h"
#include "si/service-descriptor.h"
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
    if (check_harvest())
    {
        cancel();
        return;
    }

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

    // Loop in case tune fails
    while (true)
    {
        std::shared_ptr<TuningProperties> props = nullptr;
        current_ts_data_ = nullptr;
        std::uint32_t eq;

        // First look for any discovered (in NIT) transports that haven't been
        // scanned yet.
        for (auto &tsdat: ts_data_)
        {
            if (tsdat.second.get_scan_status() == TransportStreamData::PENDING
                    && (props = tsdat.second.get_tuning()) != nullptr
                    && !scanned_equivalences_.count
                        (eq = props->get_equivalence_value()))
            {
                current_ts_data_ = &tsdat.second;
                break;
            }
        }

        // If there's nothing to be scanned in NIT go through the iterator.
        if (!props)
        {
            do
            {
                props = iter_->next();
            } while (props && scanned_equivalences_.count
                    (eq = props->get_equivalence_value()));
        }

        if (!props)
        {
            cancel();
            return;
        }

        scanned_equivalences_.insert(eq);
        g_print("Tuning to %s... ", props->describe().c_str());
        try
        {
            rcv_->tune(props, 5000);
            break;
        }
        catch (Glib::Exception &x)
        {
            g_print("Failed\n");
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
MultiScanner::get_transport_stream_data(std::uint16_t orig_nw_id,
        std::uint16_t ts_id)
{
    auto &tsdat = ts_data_[(std::uint32_t(orig_nw_id) << 16) | ts_id];
    tsdat.set_transport_stream_id(ts_id);
    tsdat.set_original_network_id(orig_nw_id);
    current_ts_data_ = &tsdat;
    return tsdat;
}

ServiceData &
MultiScanner::get_service_data(std::uint16_t orig_nw_id,
        std::uint16_t service_id)
{
    auto &sdat = service_data_[(orig_nw_id << 16) | service_id];
    sdat.set_original_network_id(orig_nw_id);
    sdat.set_service_id(service_id);
    return sdat;
}

void MultiScanner::process_service_list_descriptor(std::uint16_t orig_nw_id,
        std::uint16_t ts_id, const Descriptor &desc)
{
    ServiceListDescriptor sd(desc);
    /*
    g_debug("    Services:");
    for (const auto &s: sd.get_services())
    {
        g_debug("      id %04x type %02x", s.service_id(), s.service_type());
    }
    */
    auto &tsdat = get_transport_stream_data(orig_nw_id, ts_id);
    const auto &svcs = sd.get_services();
    g_debug("    %ld services", svcs.size());
    for (const auto &s: svcs)
    {
        tsdat.add_service_id(s.service_id());
    }
}

void MultiScanner::process_delivery_system_descriptor(std::uint16_t nw_id,
        std::uint16_t orig_nw_id, std::uint16_t ts_id, const Descriptor &desc)
{
    // FIXME: Also need to support satellite later
    TerrestrialDeliverySystemDescriptor d(desc);
    auto &tsdat = get_transport_stream_data(orig_nw_id, ts_id);
    tsdat.set_network_id(nw_id);
    auto tuning = d.get_tuning_properties();
    if (!tsdat.get_tuning())
        g_print("New TS %d: %s\n", ts_id, tuning->describe().c_str());
    else
        g_debug("    TS %d: %s", ts_id, tuning->describe().c_str());
    tsdat.set_tuning(tuning);
}

void MultiScanner::process_service_descriptor(std::uint16_t orig_nw_id,
        std::uint16_t service_id, std::uint16_t ts_id, const Descriptor &desc)
{
    ServiceDescriptor sdesc(desc);
    g_debug("  Type %d, provider_name '%s', name '%s'",
            sdesc.service_type(),
            sdesc.service_provider_name().c_str(),
            sdesc.service_name().c_str());
    auto &sdat = get_service_data(orig_nw_id, service_id);
    sdat.set_scanned();
    sdat.set_name(sdesc.service_name());
    sdat.set_provider_name(sdesc.service_provider_name());
    sdat.set_ts_id(ts_id);
    sdat.set_service_type(sdesc.service_type());
}

void MultiScanner::set_lcn(std::uint16_t nw_id, std::uint16_t service_id,
        std::uint16_t lcn)
{
    lcn_data_[(std::uint32_t(nw_id) << 16) | service_id] = lcn;
}

bool MultiScanner::check_harvest()
{
    if (!ts_data_.size() || !service_data_.size())
        return false;
    /*
    g_print("Outstanding transports:\n");
    for (const auto &ts: ts_data_)
    {
        if (ts.second.get_scan_status() == TransportStreamData::PENDING)
            g_print("%ld ", ts.first);
    }
    g_print("\n");
    */
    /*
    g_print("Outstanding services:\n");
    for (const auto &s: service_data_)
    {
        if (!s.second.get_scanned())
            g_print("%d ", s.first);
    }
    g_print("\n");
    */
    return std::all_of(ts_data_.begin(), ts_data_.end(),
        [](const std::pair<std::uint16_t, const TransportStreamData &> &ts)
        {
            return ts.second.get_scan_status() != TransportStreamData::PENDING;
        });
    /*
    && std::all_of(service_data_.begin(), service_data_.end(),
        [](const std::pair<std::uint16_t, const ServiceData &> &s)
        { return s.second.get_scanned(); });
    */
}

void MultiScanner::process_network_name(std::uint16_t network_id,
        const Glib::ustring &name)
{
    nw_data_[network_id] = NetworkNameData(network_id, name);
}

void MultiScanner::commit_to_database(Database &db, const char *source)
{
    auto ins_nw = db.get_insert_network_info_statement(source);
    auto ins_tuning = db.get_insert_tuning_statement(source);
    auto ins_trans_serv = db.get_insert_transport_services_statement(source);
    auto ins_serv_id = db.get_insert_service_id_statement(source);
    auto ins_serv_name = db.get_insert_service_name_statement(source);
    auto ins_prov_nm = db.get_insert_provider_name_statement(source);
    auto ins_serv_prov_nm =
        db.get_insert_service_provider_name_statement(source);
    auto ins_prim_lcn = db.get_insert_primary_lcn_statement(source);

    auto nw_v =
        std::make_shared<std::vector<std::tuple<id_t, Glib::ustring> > >();
    auto tuning_v =
        std::make_shared<std::vector<std::tuple<id_t, id_t, id_t, id_t> > >();
    auto trans_serv_v =
        std::make_shared<std::vector<std::tuple<id_t, id_t, id_t, id_t> > >();
    auto serv_id_v =
        std::make_shared<std::vector<std::tuple<id_t, id_t, id_t, id_t> > >();
    auto serv_name_v =
        std::make_shared<std::vector<std::tuple<id_t, id_t,
                Glib::ustring> > >();
    auto prov_nm_v =
        std::make_shared<std::vector<std::tuple<Glib::ustring> > >();
    auto serv_prov_nm_v =
        std::make_shared<std::vector<std::tuple<id_t, id_t,
                Glib::ustring> > >();
    auto prim_lcn_v =
        std::make_shared<std::vector<std::tuple<id_t, id_t, id_t> > >();

    for (const auto &nw: nw_data_)
    {
        nw_v->emplace_back(nw.first, nw.second.get_network_name());
    }
    db.queue_statement(ins_nw, nw_v);

    for (const auto &tsp: ts_data_)
    {
        const auto &ts = tsp.second;
        auto tuning = ts.get_tuning();
        if (tuning)
        {
            auto props = tuning->get_props();
            for (std::uint32_t n = 0; n < props->num; ++n)
            {
                const auto &prop = props->props[n];
                tuning_v->emplace_back(ts.get_original_network_id(),
                        ts.get_transport_stream_id(),
                        prop.cmd, prop.u.data);
            }
            for (const auto &s: ts.get_service_ids())
            {
                trans_serv_v->emplace_back(ts.get_original_network_id(),
                        ts.get_network_id(), ts.get_transport_stream_id(),
                        s);

            }
        }
    }
    db.queue_statement(ins_nw, nw_v);
    db.queue_statement(ins_tuning, tuning_v);

    for (const auto &sp: service_data_)
    {
        const auto &s = sp.second;
        serv_id_v->emplace_back(s.get_original_network_id(), s.get_service_id(),
                s.get_ts_id(), s.get_service_type());
        const auto &sn = s.get_name();
        if (sn.size())
        {
            serv_name_v->emplace_back(s.get_original_network_id(),
                    s.get_service_id(), sn);
        }
        const auto &spn = s.get_provider_name();
        if (spn.size())
        {
            prov_nm_v->emplace_back(spn);
            serv_prov_nm_v->emplace_back(s.get_original_network_id(),
                    s.get_service_id(), spn);
        }
    }
    // Global provider names have to be inserted before service_provider_names
    db.queue_statement(ins_prov_nm, prov_nm_v);
    db.queue_statement(ins_serv_name, serv_name_v);
    db.queue_statement(ins_serv_prov_nm, serv_prov_nm_v);

    for (const auto &lp: lcn_data_)
    {
        auto ns = lp.first;
        prim_lcn_v->emplace_back((ns >> 16) &0xffff, ns & 0xff, lp.second);
    }
    db.queue_statement(ins_prim_lcn, prim_lcn_v);
}

}
