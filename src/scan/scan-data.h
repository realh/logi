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

#include <memory>
#include <set>

#include "tuning.h"

namespace logi
{

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
            std::shared_ptr<TuningProperties> tuning = nullptr) :
        transport_stream_id_(transport_stream_id),
        network_id_(network_id), tuning_(tuning),
        scan_status_(PENDING)
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

    void set_tuning(TuningProperties *tuning)
    {
        if (!tuning_)
            tuning_ = std::shared_ptr<TuningProperties>(tuning);
    }

    void set_tuning(std::shared_ptr<TuningProperties> &tuning)
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
    std::uint16_t network_id_;
    std::shared_ptr<TuningProperties> tuning_;
    std::set<std::uint16_t> service_ids_;
    ScanStatus scan_status_;
};

class ServiceData
{
};

}
