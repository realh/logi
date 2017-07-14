#pragma once

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

#include "section-data.h"

namespace logi
{

/**
 * Transport stream data from a NIT section.
 */
class TSSectionData : public SectionData
{
public:
    TSSectionData(const SectionData &data, unsigned offset) :
        SectionData(data.get_data(), offset)
    {}

    std::uint16_t transport_stream_id() const
    {
        return word16(0);
    }

    std::uint16_t original_network_id() const
    {
        return word16(2);
    }

    unsigned transport_descriptors_length() const
    {
        return word12(4);
    }

    std::vector<Descriptor> get_transport_descriptors() const
    {
        return get_descriptors(4);
    }
};

}
