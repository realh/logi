#pragma once

/*
    logi - A DVB DVR designed for web-based clients.
    Copyright (C) 2017 Tony Houghton <h@realh.co.uk>

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

#include <linux/dvb/frontend.h>

#include "descriptor.h"

namespace logi
{

class DeliverySystemDescriptor : public Descriptor
{
public:
    DeliverySystemDescriptor(const Descriptor &source) :
        Descriptor(source)
    {}

    static std::uint32_t bandwidth(std::uint8_t b);

    static fe_code_rate_t code_rate(std::uint8_t r);

    static fe_guard_interval_t guard_interval(std::uint8_t g);

    static fe_transmit_mode_t transmission_mode(std::uint8_t t);

    /// Result is heap-allocated
    virtual class TuningProperties *get_tuning_properties() const = 0;
};

}
