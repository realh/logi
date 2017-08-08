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

#include <map>
#include <utility>
#include <vector>

namespace logi
{

class Section;

/**
 * TableTracker:
 * Keeps track of which sections of a table have been received.
 */
class TableTracker
{
private:
    struct TableInfo
    {
        TableInfo() : version_number(-1), all_complete(false) {}
        std::int8_t version_number;
        bool all_complete;
        std::vector<bool> completeness;
    };
    std::map<std::uint16_t, TableInfo> tables_;
    bool complete_;
public:
    enum Result
    {
        BLANK,
        OK,                 /// This is a new section.
        COMPLETE,           /// This section completes the table.
        REPEAT,             /// Already have this section.
        REPEAT_COMPLETE,    /// Table was already complete.
        OLD_VERSION,        /// This section's version number is obsolete.
        NEXT,               /// Section isn't applicable yet.
        ERROR,
    };

    TableTracker() = default;

    TableTracker(const TableTracker &) = delete;
    TableTracker(TableTracker &&) = delete;
    TableTracker &operator=(const TableTracker &) = delete;
    TableTracker &operator=(TableTracker &&) = delete;

    void reset();

    Result track(const Section &sec);

    bool complete() const { return complete_; }
private:
    Result track_for_id(const Section &sec);
};

}
