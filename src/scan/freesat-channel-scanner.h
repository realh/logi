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

#include "single-channel-scanner.h"

namespace logi
{

// Key is (bouquet_id << 16) | region_code
using FreesatRegionMap = std::map<std::uint32_t, std::string>;

class FreesatNITProcessor: public NITProcessor
{
protected:
    virtual void process_descriptor(const Descriptor &desc);
};

class FreesatBATProcessor: public NITProcessor
{
public:
    FreesatBATProcessor(FreesatRegionMap &rmap) : regions_(rmap)
    {}
protected:
    virtual void process_descriptor(const Descriptor &desc);
private:
    constexpr static std::uint8_t FREESAT_LCN_TAG = 0xD3;
    constexpr static std::uint8_t FREESAT_REGION_TAG = 0xD4;
    constexpr static std::uint32_t FREESAT_PRIVATE_DATA_SPECIFIER = 0x46534154;

    FreesatRegionMap &regions_;
};

/**
 * FreesatChannelScanner:
 * Scans BAT, BAT and SDT.
 */
class FreesatChannelScanner : public SingleChannelScanner
{
private:
    constexpr static std::uint16_t FS_NIT_PID = 3840;
    constexpr static std::uint16_t FS_BAT_PID = 3841;
    constexpr static std::uint16_t FS_SDT_PID = 3841;

    using BATFilterPtr = 
        std::unique_ptr<SectionFilter<NITSection, FreesatChannelScanner>>;
    BATFilterPtr bat_filter_;
    TableTracker::Result bat_status_;
    BouquetData::MapT bouquets_;
    FreesatRegionMap regions_;
public:
    virtual void start(MultiScanner *multi_scanner) override;

    virtual void cancel() override;

    //virtual bool is_complete() const override;

    virtual CheckHarvestPolicy check_harvest_policy() const override;

    virtual void commit_extras_to_database(class Database &db,
            const char *source) override;
protected:
    virtual bool get_filter_params(std::uint16_t &pid, std::uint8_t &table_id)
        override;

    virtual std::unique_ptr<NITProcessor> new_nit_processor() override;

    virtual bool filter_trackers_complete() const override;
private:
    BouquetData *get_bouquet_data(std::uint16_t bouquet_id);

    std::unique_ptr<NITProcessor> new_bat_processor();

    void bat_filter_cb(int reason, std::shared_ptr<NITSection> section);
};

}
