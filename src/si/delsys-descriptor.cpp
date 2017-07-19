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

}
