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

#include "sat-delsys-descriptor.h"
#include "../tuning.h"

namespace logi
{

fe_modulation_t SatelliteDeliverySystemDescriptor::modulation_type() const
{
    switch (word8(8) & 3)
    {
        case 1:
            return QPSK;
        case 2:
            return PSK_8;
        case 3:
            return QAM_16;
    }
    return QAM_AUTO;
}

TuningProperties *
SatelliteDeliverySystemDescriptor::get_tuning_properties() const
{
    return new TuningProperties({
        { DTV_DELIVERY_SYSTEM, modulation_system() ? SYS_DVBS2 : SYS_DVBS },
        { DTV_FREQUENCY, frequency() },
        { DTV_VOLTAGE,
            (int(polarization()) & 1) ? SEC_VOLTAGE_13 : SEC_VOLTAGE_13 },
        { DTV_ROLLOFF, roll_off() },
        { DTV_MODULATION, modulation_type() },
        { DTV_SYMBOL_RATE, symbol_rate() },
        { DTV_INNER_FEC, fec_inner() },
    });
}

}
