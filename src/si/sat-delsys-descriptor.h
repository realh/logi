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

class SatelliteDeliverySystemDescriptor : public DeliverySystemDescriptor
{
public:
    enum Polarization
    {
        HORIZONTAL,
        VERTICAL,
        LEFT,
        RIGHT
    };

    SatelliteDeliverySystemDescriptor(const Descriptor &source) :
        DeliverySystemDescriptor(source)
    {}

    /// Result is in KHz
    std::uint32_t frequency() const { return bcd32(2) * 10; }

    /// In units of 1/10 degree ie 282 means 28.2deg
    std::uint16_t orbital_position() const
    {
        return bcd16(6);
    }

    /// false = W, true = E
    bool west_east_flag() const
    {
        return (word8(8) & 0x80) != 0;
    }

    Polarization polarization() const
    {
        return Polarization((word8(8) & 0x60) >> 5);
    }

    fe_rolloff_t roll_off() const
    {
        return fe_rolloff_t((word8(8) & 0x18) >> 3);
    }

    /// true = DVB-S2, false = DVB-S (S1)
    bool modulation_system() const
    {
        return (word8(8) & 4) != 0;
    }

    fe_modulation_t modulation_type() const;

    /// Result is in Hz
    std::uint32_t symbol_rate() const
    {
        return (bcd32(9) / 10) * 100;
    }

    fe_code_rate_t fec_inner() const
    {
        return code_rate(word8(12) & 0xf);
    }

    /// Result is heap-allocated
    virtual class TuningProperties *get_tuning_properties() const override;
};

}
