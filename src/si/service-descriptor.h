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

namespace logi
{

class ServiceDescriptor : public Descriptor
{
public:
    ServiceDescriptor(const Descriptor &source) : Descriptor(source)
    {}

    std::uint8_t service_type() const { return word8(2); }

    std::uint8_t service_provider_name_length() const { return word8(3); }

    std::uint8_t service_name_length() const
    { return word8(service_provider_name_length() + 4); }

    Glib::ustring service_provider_name() const
    {
        return decode_string(get_data(), get_offset() + 4,
                service_provider_name_length());
    }

    Glib::ustring service_name() const
    {
        return decode_string(get_data(),
                get_offset() + 5 + service_provider_name_length(),
                service_name_length());
    }
};

}
