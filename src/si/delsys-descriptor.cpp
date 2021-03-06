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

std::uint32_t DeliverySystemDescriptor::bandwidth(std::uint8_t b)
{
    switch (b)
    {
        case 0:
            return 8000000;
        case 1:
            return 7000000;
        case 2:
            return 6000000;
        case 3:
            return 5000000;
        case 4:
            return 10000000;
        case 5:
            return 1712000;
    }
    return 8000000;
}

fe_code_rate_t DeliverySystemDescriptor::code_rate(std::uint8_t r)
{
    switch (r)
    {
        case 1:
            return FEC_1_2;
        case 2:
            return FEC_2_3;
        case 3:
            return FEC_3_4;
        case 4:
            return FEC_5_6;
        case 5:
            return FEC_7_8;
        case 6:
            return FEC_8_9;
        case 7:
            return FEC_3_5;
        case 8:
            return FEC_4_5;
        case 9:
            return FEC_9_10;
    }
    return FEC_AUTO;
}

fe_guard_interval_t DeliverySystemDescriptor::guard_interval(std::uint8_t g)
{
    switch (g)
    {
        case 0:
            return GUARD_INTERVAL_1_32;
        case 1:
            return GUARD_INTERVAL_1_16;
        case 2:
            return GUARD_INTERVAL_1_8;
        case 3:
            return GUARD_INTERVAL_1_4;
        case 4:
            return GUARD_INTERVAL_1_128;
        case 5:
            return GUARD_INTERVAL_19_128;
        case 6:
            return GUARD_INTERVAL_19_256;
    }
    return GUARD_INTERVAL_AUTO;
}

fe_transmit_mode_t
DeliverySystemDescriptor::transmission_mode(std::uint8_t t)
{
    switch (t)
    {
        case 0:
            return TRANSMISSION_MODE_2K;
        case 1:
            return TRANSMISSION_MODE_8K;
        case 2:
            return TRANSMISSION_MODE_4K;
        case 3:
            return TRANSMISSION_MODE_1K;
        case 4:
            return TRANSMISSION_MODE_16K;
        case 5:
            return TRANSMISSION_MODE_32K;
    }
    return TRANSMISSION_MODE_AUTO;
}

}
