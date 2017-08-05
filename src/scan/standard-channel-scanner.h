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
#include <memory>

#include "channel-scanner.h"
#include "nit-processor.h"
#include "scan-data.h"
#include "sdt-processor.h"
#include "section-filter.h"

namespace logi
{

/**
 * StandardChannelScanner:
 * Scans NIT and SDT.
 */
class StandardChannelScanner : public ChannelScanner
{
protected:
    using NITFilterPtr = NITFilterPtr<StandardChannelScanner>;
    using SDTFilterPtr = SDTFilterPtr<StandardChannelScanner>;

    NITFilterPtr nit_filter_;
    SDTFilterPtr this_sdt_filter_, other_sdt_filter_;
    TableTracker::Result nit_status_, this_sdt_status_, other_sdt_status_;

    NetworkData::MapT networks_;

    std::unique_ptr<SDTProcessor> this_sdt_proc_, other_sdt_proc_;

    bool have_current_ts_id_;
public:
    StandardChannelScanner() : ChannelScanner()
    {}

    void start(MultiScanner *multi_scanner) override;

    /**
     * @cancel:
     * Cancel the current scan, either to interrupt it, or when current channel
     * is finished with.
     */
    void cancel() override;
protected:
    /**
     * Can be overridden to specialise the NITProcessor eg for Freeview LCNs.
     */
    virtual std::unique_ptr<NITProcessor> new_nit_processor();

    virtual std::unique_ptr<SDTProcessor> new_sdt_processor();

    NetworkData *get_network_data(std::uint16_t network_id);

    void nit_filter_cb(int reason, std::shared_ptr<NITSection> section);

    void this_sdt_filter_cb(int reason, std::shared_ptr<SDTSection> section);

    void other_sdt_filter_cb(int reason, std::shared_ptr<SDTSection> section);

    TableTracker::Result
    sdt_filter_cb(int reason, std::shared_ptr<SDTSection> section,
            std::unique_ptr<SDTProcessor> &sdt_proc,
            SDTFilterPtr &sdt_filter,
            TableTracker::Result &sdt_status,
            const char *label);

    bool any_complete() const;

    bool filter_trackers_complete() const;
};

}
