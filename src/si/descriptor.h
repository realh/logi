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

#include "section-data.h"

namespace logi
{

class Descriptor : public SectionData
{
public:
    constexpr static std::uint8_t NETWORK_NAME = 0x40;
    constexpr static std::uint8_t SERVICE_LIST = 0x41;
    constexpr static std::uint8_t SATELLITE_DELIVERY_SYSTEM = 0x43;
    constexpr static std::uint8_t SERVICE = 0x48;
    constexpr static std::uint8_t TERRESTRIAL_DELIVERY_SYSTEM = 0x5A;
    constexpr static std::uint8_t EXTENSION = 0x7F;
public:
    Descriptor(const SectionData &sec, unsigned offset) :
        SectionData(sec.get_data(), offset)
    {}

    /**
     * Some subclasses require polymorphism.
     */
    virtual ~Descriptor() = default;

    std::uint8_t tag() const
    {
        return word8(0);
    }

    std::uint8_t length() const
    {
        return word8(1);
    }
};

class ExtensionDescriptor: public Descriptor
{
public:
    ExtensionDescriptor(const SectionData &sec, unsigned offset) :
        Descriptor(sec, offset)
    {}

    std::uint8_t tag_extension() const
    {
        return word8(2);
    }
};

}
