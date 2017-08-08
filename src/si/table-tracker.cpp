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

#include <algorithm>

#include <glib.h>

#include "section.h"
#include "table-tracker.h"

namespace logi
{

void TableTracker::reset()
{
    tables_.clear();
    complete_ = false;
}

TableTracker::Result TableTracker::track(const Section &sec)
{
    auto result = track_for_id(sec);
    if (result == REPEAT_COMPLETE)
    {
        if (!std::all_of(tables_.begin(), tables_.end(),
            [](const std::pair<std::uint16_t, TableInfo> &p)
            {
                return p.second.all_complete;
            }))
        {
            return REPEAT;
        }
    }
    return result;
}

TableTracker::Result TableTracker::track_for_id(const Section &sec)
{
    if (!sec.current_next_indicator())
        return NEXT;
    int vn = sec.version_number();
    auto &tab = tables_[sec.section_id()];

    if (tab.version_number == -1)
    {
        // This is the first section with the given id
        tab.version_number = vn;
    }
    else if (vn > tab.version_number || (vn < 12 && tab.version_number >= 24))
    {
        // New version
        reset();
        tab.version_number = vn;
    }
    else if (vn != tab.version_number)
    {
        return OLD_VERSION;
    }

    auto l = sec.last_section_number() + 1u;
    if (l > tab.completeness.size())
        tab.all_complete = false;
    if (l != tab.completeness.size())
        tab.completeness.resize(l);

    // If already complete before adding this section this must be a repeat
    if (tab.all_complete)
        return REPEAT_COMPLETE;

    Result result;

    if (tab.completeness[sec.section_number()])
    {
        result = REPEAT;
    }
    else
    {
        result = OK;
        tab.completeness[sec.section_number()] = true;
    }

    complete_ = std::all_of(tab.completeness.begin(), tab.completeness.end(),
            [](bool t)->bool { return t; });

    return complete_ ? COMPLETE : result;
}

}
