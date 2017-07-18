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

TableTracker::Result TableTracker::track(const Section &sec)
{

    int vn = sec.version_number();

    if (version_number_ == -1)
    {
        g_debug("Starting with version number %d", vn);
        version_number_ = vn;
    }
    else if (vn > version_number_ || (vn < 64 && version_number_ >= 400))
    {
        g_debug("New table version %d", vn);
        reset();
        version_number_ = vn;
    }
    else if (vn != version_number_)
    {
        return OLD_VERSION;
    }

    auto l = sec.last_section_number() + 1u;
    if (l > table_.size())
        complete_ = false;
    if (l != table_.size())
        table_.resize(l);

    // If already complete before adding this section this must be a repeat
    if (complete_)
        return REPEAT_COMPLETE;

    Result result;

    if (table_[sec.section_number()])
    {
        result = REPEAT;
    }
    else
    {
        result = OK;
        table_[sec.section_number()] = true;
    }

    complete_ = std::all_of(table_.begin(), table_.end(),
            [](bool t)->bool { return t; });

    return complete_ ? COMPLETE : result;
}

}
