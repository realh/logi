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

#include <memory>

#include <glibmm/ustring.h>

#include "si/nit-section.h"
#include "si/table-tracker.h"

namespace logi
{

class MultiScanner;

/**
 * NITProcessor:
 * Builds up information from NIT sections.
 */
class NITProcessor
{
protected:
    TableTracker tracker_;
    Glib::ustring network_name_;
    MultiScanner *mscanner_;
    std::uint16_t current_ts_id_, current_nw_id_, current_orig_nw_id_;
public:
    /**
     * process:
     * Returns: true if a complete table has been received.
     */
    bool process(std::shared_ptr<NITSection> sec, MultiScanner *ms);

    void reset_tracker()
    {
        tracker_.reset();
    }
protected:
    virtual void process_ts_data(const TSSectionData &ts);

    virtual void process_descriptor(const Descriptor &desc);
};

}
