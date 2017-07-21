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

class T2DeliverySystemDescriptor : public ExtensionDescriptor
{
public:
    T2DeliverySystemDescriptor(const Descriptor &source) :
        ExtensionDescriptor(source)
    {}

    std::uint8_t plp_id() const { return word8(3); }

    std::uint16_t t2_system_id() const { return word16(4); }

    /// The following are only valid if length() > 4
    
    std::uint8_t siso_miso() const { return (word8(6) & 0xc0) >> 6; }

    std::uint32_t bandwidth() const
    {
        return DeliverySystemDescriptor::bandwidth((word8(6) & 0x3c) >> 2);
    }

    fe_guard_interval_t guard_interval() const
    {
        return DeliverySystemDescriptor::guard_interval((word8(7) & 0xe0) >> 5);
    }

    fe_transmit_mode_t transmission_mode() const
    {
        return DeliverySystemDescriptor::transmission_mode
            ((word8(7) & 0x1c) >> 2);
    }

    bool other_frequency_flag() const { return (word8(7) & 2) != 0; }

    bool tfs_flag() const { return (word8(7) & 1) != 0; }

};

}
