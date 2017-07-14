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

#include "decode-string.h"
#include "descriptor.h"
#include "section-data.h"

namespace logi
{

class ServiceListDescriptor : public Descriptor
{
public:

    class ServiceInfo : public SectionData
    {
    public:
        ServiceInfo(const Descriptor &source, unsigned offset) :
            SectionData(source.get_data(), source.get_offset() + offset)
        {}

        std::uint16_t service_id() const { return word16(0); }

        std::uint8_t service_type() const { return word8(2); }
    };

    ServiceListDescriptor(const Descriptor &source) : Descriptor(source)
    {}

    std::uint8_t get_services_length() const { return length(); }

    std::vector<ServiceInfo> get_services() const;
};

}
