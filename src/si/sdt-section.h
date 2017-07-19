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

#include "descriptor.h"
#include "section.h"

namespace logi
{

constexpr std::uint16_t SDT_PID = 0x11;

class SDTSectionServiceData : public SectionData
{
public:
    enum RunningStatus
    {
        UNDEFINED = 0,
        NOT_RUNNING,
        START_IMMINENT,
        PAUSING,
        RUNNING,
        OFF_AIR
    };

    SDTSectionServiceData(const std::vector<uint8_t> &data, unsigned offset) :
        SectionData(data, offset)
    {}

    std::uint16_t service_id() const { return word16(0); }

    bool EIT_schedule_flag() const { return (word8(2) & 2) != 0; }

    bool EIT_present_following_flag() const { return (word8(2) & 1) != 0; }

    RunningStatus running_status() const
    { return RunningStatus((word8(3) & 0xe0) >> 5); }

    /// true = encrypted
    bool free_CA_mode() const { return (word8(3) & 0x10) != 0; }

    std::uint16_t descriptors_loop_length() const { return word12(3); }

    std::vector<Descriptor> get_descriptors() const
    {
        return SectionData::get_descriptors(3);
    }
};

class SDTSection : public Section
{
public:
    SDTSection() : Section()
    {}

    std::uint16_t transport_stream_id() const
    {
        return section_id();
    }

    std::uint16_t original_network_id() const
    {
        return word16(8);
    }

    std::vector<SDTSectionServiceData> get_services() const;
};

}
