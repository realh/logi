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

#include <cstdint>
#include <vector>

namespace logi
{

class Descriptor;

/**
 * Allows access to section data, either on behalf of a full section or a
 * descriptor etc.
 */
class SectionData
{
protected:
    const std::vector<std::uint8_t> &data_;
    unsigned offset_;
public:
    SectionData(const std::vector<std::uint8_t> &data, unsigned offset) :
        data_(data), offset_(offset)
    {}

    // Default copy ctors etc are OK
    
    unsigned get_offset() const
    {
        return offset_;
    }

    const std::vector<std::uint8_t> &get_data() const
    {
        return data_;
    }

    // Words in sections are big endian and not necessarily aligned, so we
    // have to use these methods to access them.
    
    std::uint32_t word32(unsigned o) const
    {
        return (std::uint32_t(word8(o)) << 24) | word24(o + 1);
    }

    std::uint32_t word24(unsigned o) const
    {
        return (std::uint32_t(word8(o)) << 16) | word16(o + 1);
    }

    std::uint16_t word16(unsigned o) const
    {
        return (std::uint16_t(word8(o)) << 8) | word8(o + 1);
    }

    std::uint16_t word12(unsigned o) const
    {
        return word16(o) & 0xfff;
    }

    std::uint8_t word8(unsigned o) const
    {
        return data_[o + offset_];
    }

    std::uint32_t bcd32(unsigned o) const
    {
        return std::uint32_t(bcd8(o)) * 1000000 + bcd24(o + 1);
    }

    std::uint32_t bcd24(unsigned o) const
    {
        return std::uint32_t(bcd8(o)) * 10000 + bcd16(o + 1);
    }

    std::uint16_t bcd16(unsigned o) const
    {
        return std::uint16_t(bcd8(o)) * 100 + bcd8(o + 1);
    }

    std::uint8_t bcd8(unsigned o) const
    {
        return ((word8(o) & 0xf0) >> 4) * 10 + (word8(o) & 0xf);
    }

    std::uint32_t bcd_time(unsigned o) const
    {
        return  std::uint32_t(bcd8(o)) * 3600 +
                std::uint32_t(bcd8(o + 1)) * 60 +
                bcd8(o + 2);
    }

    /**
     * get_descriptors:
     * @o:  Offset of a length field (bottom 12-bits of 2 bytes) immediately
     *      followed by the descriptors' data.
     * Returns: A vector of descriptors found at the given offset.
     */
    std::vector<Descriptor> get_descriptors(unsigned o) const;
};

}
