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
private:
    using nit_filter_ptr =
        std::unique_ptr<SectionFilter<NITSection, StandardChannelScanner> >;
    nit_filter_ptr nit_filter_;
    using sdt_filter_ptr =
        std::unique_ptr<SectionFilter<SDTSection, StandardChannelScanner> >;
    sdt_filter_ptr this_sdt_filter_, other_sdt_filter_;
    TableTracker::Result nit_status_, this_sdt_status_, other_sdt_status_;

    struct NetworkData
    {
        std::unique_ptr<NITProcessor> nit_proc;
        bool nit_complete;
        NetworkData(std::unique_ptr<NITProcessor> &&np) :
            nit_proc(std::move(np)), nit_complete(false)
        {}
    };
    // In practice we're unlikely to see more than a couple of networks, so
    // a map is a horrible waste of CPU cycles etc, but it is scalable and 
    // makes my code much simpler. Or it would if std::pair wasn't incompatible
    // with itself, making large parts of std::map unusable.
    using NwMap = std::map<std::uint16_t, std::unique_ptr<NetworkData> >;
    using NwPair = std::pair<std::uint16_t, std::unique_ptr<NetworkData> >;
    using ConstNwPair = const std::pair<const std::uint16_t,
          std::unique_ptr<NetworkData> >;
    NwMap networks_;

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

    /**
     * Returns: Whether a complete data set has been acquired.
     */
    bool is_complete() const override;
protected:
    /**
     * Can be overridden to specialise the NITProcessor eg for Freeview LCNs.
     */
    virtual std::unique_ptr<NITProcessor> new_nit_processor();

    virtual std::unique_ptr<SDTProcessor> new_sdt_processor();
private:
    NetworkData *get_network_data(std::uint16_t network_id);

    void nit_filter_cb(int reason, std::shared_ptr<NITSection> section);

    void this_sdt_filter_cb(int reason, std::shared_ptr<SDTSection> section);

    void other_sdt_filter_cb(int reason, std::shared_ptr<SDTSection> section);

    TableTracker::Result
    sdt_filter_cb(int reason, std::shared_ptr<SDTSection> section,
            std::unique_ptr<SDTProcessor> &sdt_proc,
            sdt_filter_ptr &sdt_filter,
            TableTracker::Result &sdt_status,
            const char *label);

    bool any_complete() const;

    bool filter_trackers_complete() const;
};

}
