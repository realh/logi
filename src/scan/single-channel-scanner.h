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

#include "nit-processor.h"
#include "scan-data.h"
#include "sdt-processor.h"
#include "section-filter.h"

namespace logi
{

/**
 * SingleChannelScanner:
 * Scans NIT and SDT.
 */
class SingleChannelScanner
{
protected:
    using NITFilterPtr =
        std::unique_ptr<SectionFilter<NITSection, SingleChannelScanner>>;
    using SDTFilterPtr =
        std::unique_ptr<SectionFilter<SDTSection, SingleChannelScanner>>;

    NITFilterPtr nit_filter_;
    SDTFilterPtr this_sdt_filter_, other_sdt_filter_;
    TableTracker::Result nit_status_, this_sdt_status_, other_sdt_status_;

    MultiScanner *multi_scanner_;

    NetworkData::MapT networks_;

    std::unique_ptr<SDTProcessor> this_sdt_proc_, other_sdt_proc_;

    bool have_current_ts_id_;
    bool successful_ = false;
public:
    virtual ~SingleChannelScanner() = default;

    virtual void start(MultiScanner *multi_scanner);

    /**
     * @cancel:
     * Cancel the current scan, either to interrupt it, or when current channel
     * is finished with.
     */
    virtual void cancel();

    virtual bool is_complete() const;

    enum CheckHarvestPolicy {
        /* Each TS in Freesat carries data for all other streams but not itself,
         * so we need to scan 2 of them to get a complete dataset.
         */
        SCAN_AT_LEAST_2 = 1,

        /* In Freeview we need to scan all transports the SI refers to in order
         * to get service names.
         */
        SCAN_ALL_DISCOVERED_TS = 2,

        /* Freeview's DVB-T2 multiplexes don't get described in DVB-T NIT so
         * continuing scan until all referenced services are discovered is
         * another way we can check for completeness.
         */
        FIND_ALL_SERVICES = 4
    };

    virtual CheckHarvestPolicy check_harvest_policy() const;

    /**
     * Can be overridden to add additional data to the database. This is
     * used to add Freesat region names. The default does nothing so you don't
     * need to chain up.
     */
    virtual void commit_extras_to_database(class Database &db,
            const char *source);
protected:
    /**
     * Can be overidden to use custom pids for the filter or disable it
     * altogether.
     * @param[in, out] pid  
     * @param[in, out] table_id
     * @return false if this filter is not uspported.
     */
    virtual bool get_filter_params(std::uint16_t &pid, std::uint8_t &table_id);

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

    virtual bool filter_trackers_complete() const;

    virtual void finished(bool success = true);
};

}
