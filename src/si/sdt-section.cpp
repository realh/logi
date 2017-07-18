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

#include <glib.h>

#include "sdt-section.h"

namespace logi
{


std::vector<SDTSectionServiceData> SDTSection::get_services() const
{
    std::vector<SDTSectionServiceData> svcs;
    unsigned o = 11;
    while (o < unsigned(section_length())
            + 3 /* table_id and length */ - 4 /* CRC */)
    {
        svcs.emplace_back(get_data(), get_offset() + o);
        o += 5 + word12(o + 3);
    }
    return svcs;
}

}
