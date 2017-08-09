#pragma once

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
#include <memory>
#include <set>

#include "nit-processor.h"
#include "tuning.h"

namespace logi
{

class NetworkData
{
public:
    using MapT = std::map<std::uint16_t, std::unique_ptr<NetworkData>>;
    using PairT = std::pair<std::uint16_t, const std::unique_ptr<NetworkData>&>;

    std::unique_ptr<NITProcessor> nit_proc;
    bool nit_complete;

    NetworkData(std::unique_ptr<NITProcessor> &&np) :
        nit_proc(std::move(np)), nit_complete(false)
    {}

    static bool any_complete(const MapT &nw_map)
    {
        return std::any_of(nw_map.begin(), nw_map.end(),
                [](const NetworkData::PairT &n)->bool
        {
            return n.second->nit_complete;
        });
    }
};

using BouquetData = NetworkData;

class NetworkNameData
{
public:
    NetworkNameData() : network_id_(0) {}

    NetworkNameData(std::uint16_t nw_id, const Glib::ustring &name)
        : network_id_(nw_id), network_name_(name)
    {}

    NetworkNameData(const NetworkNameData &) = default;
    NetworkNameData(NetworkNameData &&) = default;
    NetworkNameData &operator=(const NetworkNameData &) = default;
    NetworkNameData &operator=(NetworkNameData &&) = default;

    std::uint16_t get_network_id() const { return network_id_; }

    std::uint16_t get_bouquet_id() const { return network_id_; }

    const Glib::ustring &get_network_name() const { return network_name_; }

    const Glib::ustring &get_bouquet_name() const { return network_name_; }
private:
    std::uint16_t network_id_;
    Glib::ustring network_name_;
};

using BouquetNameData = NetworkNameData;

class TransportStreamData
{
public:
    enum ScanStatus
    {
        PENDING,
        SCANNED,
        FAILED
    };

    TransportStreamData(std::uint16_t transport_stream_id = 0,
            std::uint16_t network_id = 0,
            std::uint16_t orig_network_id = 0,
            std::shared_ptr<TuningProperties> tuning = nullptr) :
        transport_stream_id_(transport_stream_id),
        network_id_(network_id),
        original_network_id_(orig_network_id),
        tuning_(tuning), scan_status_(PENDING)
    {}

    void set_transport_stream_id(std::uint16_t transport_stream_id)
    {
        transport_stream_id_ = transport_stream_id;
    }

    std::uint16_t get_transport_stream_id() const
    {
        return transport_stream_id_;
    }

    void set_network_id(std::uint16_t network_id)
    {
        network_id_ = network_id;
    }

    std::uint16_t get_network_id() const
    {
        return network_id_;
    }

    void set_original_network_id(std::uint16_t original_network_id)
    {
        original_network_id_ = original_network_id;
    }

    std::uint16_t get_original_network_id() const
    {
        return original_network_id_;
    }

    void set_tuning(TuningProperties *tuning)
    {
        if (!tuning_)
            tuning_ = std::shared_ptr<TuningProperties>(tuning);
    }

    void set_tuning(std::shared_ptr<TuningProperties> tuning)
    {
        if (!tuning_)
            tuning_ = tuning;
    }

    std::shared_ptr<TuningProperties> get_tuning() const
    {
        return tuning_;
    }

    void add_service_id(std::uint16_t service_id)
    {
        service_ids_.insert(service_id);
    }

    const std::set<std::uint16_t> get_service_ids() const
    {
        return service_ids_;
    }

    void set_scan_status(ScanStatus status)
    {
        scan_status_ = status;
    }

    ScanStatus get_scan_status() const
    {
        return scan_status_;
    }
private:
    std::uint16_t transport_stream_id_;
    std::uint16_t network_id_, original_network_id_;
    std::shared_ptr<TuningProperties> tuning_;
    std::set<std::uint16_t> service_ids_;
    ScanStatus scan_status_;
};

class ServiceData
{
public:
    ServiceData(std::uint16_t service_id = 0,
            std::uint16_t orig_nw_id = 0,
            std::uint16_t ts_id = 0,
            std::uint8_t service_type = 0,
            bool free_ca_mode = false,
            bool scanned = false) :
        service_id_(service_id),
        original_network_id_(orig_nw_id),
        ts_id_(ts_id),
        service_type_(service_type),
        free_ca_mode_(free_ca_mode),
        scanned_(scanned)
    {}

    void set_service_id(std::uint16_t service_id)
    {
        service_id_ = service_id;
    }

    std::uint16_t get_service_id() const
    {
        return service_id_;
    }

    void set_original_network_id(std::uint16_t original_network_id)
    {
        original_network_id_ = original_network_id;
    }

    std::uint16_t get_original_network_id() const
    {
        return original_network_id_;
    }

    void set_ts_id(std::uint16_t ts_id)
    {
        ts_id_ = ts_id;
    }

    std::uint16_t get_ts_id() const
    {
        return ts_id_;
    }

    void set_service_type(std::uint8_t service_type)
    {
        service_type_ = service_type;
    }

    std::uint8_t get_service_type() const
    {
        return service_type_;
    }

    template<class S> void set_name(S &&name)
    {
        name_ = name;
    }

    const Glib::ustring &get_name() const
    {
        return name_;
    }

    template<class S> void set_provider_name(S &&name)
    {
        provider_name_ = name;
    }

    const Glib::ustring &get_provider_name() const
    {
        return provider_name_;
    }

    void set_free_ca_mode(bool encrypted)
    {
        free_ca_mode_ = encrypted;
    }

    bool get_free_ca_mode() const
    {
        return free_ca_mode_;
    }

    void set_scanned()
    {
        scanned_ = true;
    }

    bool get_scanned() const
    {
        return scanned_;
    }
private:
    std::uint16_t service_id_;
    std::uint16_t original_network_id_;
    std::uint16_t ts_id_;
    std::uint8_t service_type_;
    Glib::ustring name_, provider_name_;
    bool free_ca_mode_;
    bool scanned_;
};

}
