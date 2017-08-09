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

#include "si/section.h"
#include "multi-scanner.h"
#include "freesat-channel-scanner.h"

namespace logi
{

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

std::unique_ptr<NITProcessor> new_bat_processor()
{
    return std::unique_ptr<NITProcessor>(new FreesatBATProcessor());
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
        get_network_data(section->network_id()) : nullptr;

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
