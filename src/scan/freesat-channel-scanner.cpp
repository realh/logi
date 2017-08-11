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

#include <cstring>

#include <glib.h>

#include "si/decode-string.h"
#include "si/network-name-descriptor.h"
#include "si/section.h"
#include "multi-scanner.h"
#include "freesat-channel-scanner.h"

namespace logi
{

class FreesatRegionData : public SectionData
{
public:
    FreesatRegionData(const std::vector<std::uint8_t> &data, unsigned offset) :
        SectionData(data, offset)
    {}

    FreesatRegionData(const FreesatRegionData &fr) : SectionData(fr)
    {}

    std::uint16_t region_code() const
    {
        return word16(0);
    }

    std::uint32_t language() const { return word24(2); }

    std::uint8_t name_length() const
    {
        return word8(5);
    }

    std::string name() const
    {
        name_length();
        return decode_string(data_, offset_ + 6, name_length());
    }
};

class FreesatRegionDescriptor : public Descriptor
{
public:
    FreesatRegionDescriptor(const Descriptor &d) : Descriptor(d)
    {}

    std::vector<FreesatRegionData> get_region_data() const;
};

std::vector<FreesatRegionData> FreesatRegionDescriptor::get_region_data() const
{
    std::vector<FreesatRegionData> v;
    unsigned i = 0;
    unsigned l = length();

    i = 0;
    while (i < l)
    {
        v.emplace_back(data_, offset_ + 2 + i);
        i += word8(2 + i + 5) + 6;
    }
    return v;
}


struct FreesatLCNPair : public SectionData
{
    FreesatLCNPair(std::vector<std::uint8_t> data, unsigned offset) :
        SectionData(data, offset)
    {}

    std::uint16_t lcn() const { return word16(0); }

    std::uint16_t region_code() const { return word16(2); }
};

class FreesatLCNData : public SectionData
{
public:
    FreesatLCNData(std::vector<std::uint8_t> data, unsigned offset) :
        SectionData(data, offset)
    {}

    std::uint16_t service_id() const { return word16(2); }

    std::uint16_t unknown() const { return word16(4); }

    std::uint8_t pairs_length() const { return word8(6); }

    std::vector<FreesatLCNPair> get_lcn_pairs() const;
};

std::vector<FreesatLCNPair> FreesatLCNData::get_lcn_pairs() const
{
    std::vector<FreesatLCNPair> v;
    unsigned i = 0;
    unsigned l = pairs_length();
    while (i < l)
    {
        v.emplace_back(data_, offset_ + 7 + i);
        i += 4;
    }
    return v;
}


void FreesatNITProcessor::process_descriptor(const Descriptor &desc)
{
    // All we're interested in from Freesat NIT is delivery system descriptors
    if (desc.tag() == Descriptor::SATELLITE_DELIVERY_SYSTEM)
    {
        mscanner_->process_delivery_system_descriptor(current_nw_id_,
                current_orig_nw_id_, current_ts_id_, desc);
    }
}

void FreesatBATProcessor::process_descriptor(const Descriptor &desc)
{
    // Bouquet mostly contains structures identical to those for network
    switch (desc.tag())
    {
        case Descriptor::BOUQUET_NAME:
            if (!network_name_.size())
            {
                network_name_ = NetworkNameDescriptor(desc).get_network_name();
                g_print("Discovered bouquet %d %s\n",
                        current_nw_id_, network_name_.c_str());
                mscanner_->process_network_name(current_nw_id_, network_name_);
            }
            break;
        case FREESAT_REGION_TAG:
            {
                const FreesatRegionDescriptor &d(desc);
                std::vector<FreesatRegionData> regs = d.get_region_data();
                for (const auto &r: regs)
                {
                    auto &rn = regions_[r.region_code()];
                    if (!rn.size())
                    {
                        rn = r.name();
                        g_print("Discovered region %s\n", rn.c_str());
                    }
                }
            }
            break;
    }
}

bool FreesatChannelScanner::get_filter_params(std::uint16_t &pid,
        std::uint8_t &table_id)
{
    switch (table_id)
    {
        case Section::NIT_TABLE:
            pid = FS_NIT_PID;
            table_id = Section::OTHER_NIT_TABLE;
            break;
        case Section::SDT_TABLE:
            return false;
        case Section::OTHER_SDT_TABLE:
            pid = FS_SDT_PID;
            break;
    }
    return true;
}

void FreesatChannelScanner::start(MultiScanner *multi_scanner)
{
    bat_status_ = TableTracker::BLANK;
    for (auto &bd: bouquets_)
    {
        bd.second->nit_proc->reset_tracker();
    }
    bat_filter_.reset(new SectionFilter<BATSection, FreesatChannelScanner>(
            multi_scanner->get_receiver(), *this,
            &FreesatChannelScanner::bat_filter_cb,
            FS_BAT_PID, Section::BAT_TABLE, 0, 5000, 0xff, 0));
    SingleChannelScanner::start(multi_scanner);
}

void FreesatChannelScanner::cancel()
{
    if (bat_filter_)
    {
        bat_filter_->stop();
        bat_filter_.reset();
    }
    SingleChannelScanner::cancel();
}

BouquetData *
FreesatChannelScanner::get_bouquet_data(std::uint16_t bouquet_id)
{
    auto bdi = bouquets_.find(bouquet_id);
    BouquetData *bd = nullptr;

    if (bdi == bouquets_.end())
    {
        bouquets_[bouquet_id] = std::unique_ptr<BouquetData>
                (bd = new BouquetData(new_bat_processor()));
    }
    else
    {
        bd = bdi->second.get();
    }
    return bd;
}

std::unique_ptr<NITProcessor> FreesatChannelScanner::new_nit_processor()
{
    return std::unique_ptr<NITProcessor>(new FreesatNITProcessor());
}

std::unique_ptr<NITProcessor> FreesatChannelScanner::new_bat_processor()
{
    return std::unique_ptr<NITProcessor>(new FreesatBATProcessor(regions_));
}

bool FreesatChannelScanner::filter_trackers_complete() const
{
    if (bat_status_ != TableTracker::COMPLETE &&
            bat_status_ != TableTracker::REPEAT_COMPLETE &&
            bat_status_ != TableTracker::ERROR)
    {
        return false;
    }
    return SingleChannelScanner::filter_trackers_complete();
}

void FreesatChannelScanner::bat_filter_cb(int reason,
        std::shared_ptr<BATSection> section)
{
    BouquetData *bd = section ?
        get_bouquet_data(section->network_id()) : nullptr;

    if (reason)
    {
        g_critical("BAT filter error: %s\n", std::strerror(reason));
        bat_status_ = TableTracker::ERROR;
    }
    else if (bd)
    {
        bd->nit_complete = bd->nit_proc->process(section, multi_scanner_);
        if (bd->nit_complete)
            bat_status_ = TableTracker::COMPLETE;
        else if (bat_status_ == TableTracker::BLANK)
            bat_status_ = TableTracker::OK;
    }

    if (!section || bat_status_ == TableTracker::COMPLETE
        || bat_status_ == TableTracker::ERROR)
    {
        bat_filter_->stop();
        g_debug("Testing completeness for BAT");
        if (filter_trackers_complete())
            finished(any_complete());
    }
}

}
