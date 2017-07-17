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

#include <set>

#include "tuning.h"

namespace logi
{

class TransportStreamData
{
public:
    TransportStreamData(std::uint16_t transport_stream_id = 0,
            std::uint16_t network_id = 0,
            TuningProperties *tuning = nullptr) :
        transport_stream_id_(transport_stream_id),
        network_id_(network_id), tuning_(tuning)
    {}

    ~TransportStreamData()
    {
        delete tuning_;
    }

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

    /// Takes ownership of tuning, deletes repeats
    void set_tuning(TuningProperties *tuning)
    {
        if (tuning_)
            delete tuning;
        else
            tuning_ = tuning;
    }

    const TuningProperties *get_tuning() const
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
private:
    std::uint16_t transport_stream_id_;
    std::uint16_t network_id_;
    TuningProperties *tuning_;
    std::set<std::uint16_t> service_ids_;
};

}