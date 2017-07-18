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

#include <cstdint>
#include <vector>

#include "section-data.h"

namespace logi
{

class Section : public SectionData
{
protected:
    std::vector<std::uint8_t> sec_;
public:
    Section(unsigned size = 1024) : SectionData(sec_, 0), sec_(size)
    {}

    /**
     * read_from_fd:
     * Returns: Number of bytes read, or -1 for error.
     */
    int read_from_fd(int fd);

    std::uint8_t table_id()             const   { return word8(0); }

    std::uint16_t section_length()      const   { return word12(1); }

    std::uint16_t section_id()          const   { return word16(3); }

    std::uint8_t version_number()       const   { return (word8(5)>>1)&0x1f; }

    bool current_next_indicator()       const   { return word8(5) & 1; }

    std::uint8_t section_number()       const   { return word8(6); }

    std::uint8_t last_section_number()  const   { return word8(7); }

    void dump_to_stdout() const;
};

}
