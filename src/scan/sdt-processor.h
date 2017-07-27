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

#include <memory>

#include "si/sdt-section.h"
#include "si/sdt-section.h"
#include "si/table-tracker.h"

namespace logi
{

class MultiScanner;

/**
 * NITProcessor:
 * Builds up information from NIT sections.
 */
class SDTProcessor
{
private:
    TableTracker tracker_;
    MultiScanner *mscanner_;
    std::uint16_t current_orig_nw_id_, current_ts_id_, current_service_id_;
public:
    TableTracker::Result
    process(std::shared_ptr<SDTSection> sec, MultiScanner *ms);

    void reset_tracker()
    {
        tracker_.reset();
    }

    bool complete() const
    {
        return tracker_.complete();
    }
protected:
    virtual void process_service_data(const SDTSectionServiceData &ts);

    virtual void process_descriptor(const Descriptor &desc);
};

}
