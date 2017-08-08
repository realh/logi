/*
    logi - A DVB DVR designed for web-based clients.
    Copyright (C) 2016-2017 Tony Houghton <h@realh.co.uk>

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

#include <cstring>

#include <glib.h>

#include "si/section.h"
#include "multi-scanner.h"
#include "freesat-channel-scanner.h"

namespace logi
{

bool FreesatChannelScanner::get_filter_params(std::uint16_t &pid,
        std::uint8_t &table_id)
{
    switch (table_id)
    {
        case Section::NIT_TABLE:
            pid = FS_NIT_PID;
            table_id = Section::OTHER_NIT_TABLE;
            break;
        case Section::SDT_TABLE:
            return false;
        case Section::OTHER_SDT_TABLE:
            pid = FS_SDT_PID;
            break;
    }
    return true;
}

}
