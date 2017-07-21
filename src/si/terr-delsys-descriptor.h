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

#include "delsys-descriptor.h"

namespace logi
{

class TerrestrialDeliverySystemDescriptor : public DeliverySystemDescriptor
{
public:
    TerrestrialDeliverySystemDescriptor(const Descriptor &source) :
        DeliverySystemDescriptor(source)
    {}

    /// Result is in Hz
    std::uint32_t centre_frequency() const { return word32(2) * 10; }

    std::uint32_t bandwidth() const
    {
        return DeliverySystemDescriptor::bandwidth((word8(6) & 0xe0) >> 5);
    }

    /// True for high priority
    bool priority() const { return (word8(6) & 0x10) == 0; }

    /// True if time slicing is enabled
    bool time_slicing() const { return (word8(6) & 8) == 0; }

    /// True if MPE-FEC is used
    bool mpe_fec() const { return (word8(6) & 4) == 0; }

    fe_modulation_t constellation() const;

    fe_hierarchy_t hierarchy() const;

    fe_code_rate_t code_rate_hp() const
    { return code_rate((word8(7) & 7) + 1); }

    fe_code_rate_t code_rate_lp() const
    { return code_rate(((word8(8) & 0xe0) >> 5) + 1); }

    fe_guard_interval_t guard_interval() const
    {
        return DeliverySystemDescriptor::guard_interval((word8(8) & 0x18) >> 3);
    }

    fe_transmit_mode_t transmission_mode() const
    {
        return DeliverySystemDescriptor::transmission_mode((word8(8) & 6) >> 1);
    }

    bool other_frequency() const { return (word8(8) & 1) != 0; }

    /// Result is heap-allocated
    virtual class TuningProperties *get_tuning_properties() const override;
};

}
