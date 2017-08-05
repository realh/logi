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
 * FreesatChannelScanner:
 * Scans BAT, NIT and SDT.
 */
class FreesatChannelScanner : public ChannelScanner
{
private:
    constexpr static std::uint16_t FS_NIT_PID = 3840;
    constexpr static std::uint16_t FS_BAT_PID = 3841;
    constexpr static std::uint16_t FS_SDT_PID = 3841;

    using NITFilterPtr = NITFilterPtr<FreesatChannelScanner>;
    using SDTFilterPtr = SDTFilterPtr<FreesatChannelScanner>;

    NITFilterPtr nit_filter_;
    SDTFilterPtr sdt_filter_;
    TableTracker::Result nit_status_, sdt_status_;

    NetworkData::MapT networks_;

    std::unique_ptr<SDTProcessor> sdt_proc_;
public:
    FreesatChannelScanner() : ChannelScanner()
    {}

    void start(MultiScanner *multi_scanner) override;

    /**
     * @cancel:
     * Cancel the current scan, either to interrupt it, or when current channel
     * is finished with.
     */
    void cancel() override;
private:
    /**
     * Can be overridden to specialise the NITProcessor eg for Freeview LCNs.
     */
    virtual std::unique_ptr<NITProcessor> new_nit_processor();

    virtual std::unique_ptr<SDTProcessor> new_sdt_processor();

    NetworkData *get_network_data(std::uint16_t network_id);

    void nit_filter_cb(int reason, std::shared_ptr<NITSection> section);

    void sdt_filter_cb(int reason, std::shared_ptr<SDTSection> section);

    bool any_complete() const;

    bool filter_trackers_complete() const;
};

}
