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

#include <utility>

#include "terr-delsys-descriptor.h"

#include "tuning.h"

namespace logi
{

fe_modulation_t TerrestrialDeliverySystemDescriptor::constellation() const
{
    switch ((word8(7) & 0xc0) >> 6)
    {
        case 0:
            return QPSK;
        case 1:
            return QAM_16;
        case 2:
            return QAM_64;
    }
    return QAM_AUTO;
}

fe_hierarchy_t TerrestrialDeliverySystemDescriptor::hierarchy() const
{
    switch ((word8(7) & 0x18) >> 3)
    {
        case 0:
            return HIERARCHY_NONE;
        case 1:
            return HIERARCHY_1;
        case 2:
            return HIERARCHY_2;
        case 3:
            return HIERARCHY_4;
    }
    return HIERARCHY_AUTO;
}

TuningProperties *
TerrestrialDeliverySystemDescriptor::get_tuning_properties() const
{
    return new TuningProperties({
        { DTV_DELIVERY_SYSTEM, SYS_DVBT },
        { DTV_FREQUENCY, centre_frequency()},
        { DTV_BANDWIDTH_HZ, bandwidth() },
        { DTV_MODULATION, constellation() },
        { DTV_HIERARCHY, hierarchy() },
        { DTV_CODE_RATE_HP, code_rate_hp() },
        { DTV_CODE_RATE_LP, code_rate_lp() },
        { DTV_GUARD_INTERVAL, guard_interval() },
        { DTV_TRANSMISSION_MODE, transmission_mode() },
    });
    /*
    return new TuningProperties({
        { DTV_DELIVERY_SYSTEM, SYS_DVBT },
        { DTV_FREQUENCY, centre_frequency()},
        { DTV_BANDWIDTH_HZ, bandwidth() },
        { DTV_MODULATION, QAM_AUTO },
        { DTV_HIERARCHY, HIERARCHY_AUTO },
        { DTV_CODE_RATE_HP, FEC_AUTO },
        { DTV_CODE_RATE_LP, FEC_AUTO },
        { DTV_GUARD_INTERVAL, GUARD_INTERVAL_AUTO },
        { DTV_TRANSMISSION_MODE, TRANSMISSION_MODE_AUTO }
    });
    */
}

}
