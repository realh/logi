#pragma once

/*
    logi - A DVB DVR designed for web-based clients.
    Copyright (C) 2016 Tony Houghton <h@realh.co.uk>

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

#include "descriptor.h"
#include "section.h"

namespace logi
{

class TSSectionData;

class NITSection : public Section
{
public:
    NITSection() : Section()
    {}

    std::uint16_t network_id() const
    {
        return section_id();
    }

    std::uint16_t bouquet_id() const
    {
        return section_id();
    }

    unsigned network_descriptors_length() const
    {
        return word12(8);
    }

    std::vector<Descriptor> get_network_descriptors() const
    {
        return get_descriptors(8);
    }

    // BAT is practically identical to NIT apart from descriptor content
    std::vector<Descriptor> get_bouquet_descriptors() const
    {
        return get_descriptors(8);
    }

    unsigned transport_stream_loop_length() const
    {
        return word12(network_descriptors_length() + 10);
    }

    std::vector<TSSectionData> get_transport_stream_loop() const;
};

using BATSection = NITSection;

}
