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

#include <glib.h>

#include "multi-scanner.h"
#include "nit-processor.h"

#include "si/network-name-descriptor.h"
#include "si/service-list-descriptor.h"
#include "si/ts-data.h"

namespace logi
{

bool NITProcessor::process(std::shared_ptr<NITSection> sec, MultiScanner *ms)
{
    bool complete = false;

    mscanner_ = ms;

    switch (tracker_.track(*sec))
    {
        case TableTracker::REPEAT:
            g_debug("Repeat NIT section number %d", sec->section_number());
            return false;
        case TableTracker::REPEAT_COMPLETE:
            g_debug("Repeat and complete NIT (%d)", sec->section_number());
            return true;
        case TableTracker::COMPLETE:
            complete = true;
            break;
        case TableTracker::OK:
            break;
        case TableTracker::OLD_VERSION:
            g_debug("Old NIT section version %d", sec->version_number());
            return false;
    }

    //g_print("********\n");
    //sec->dump_to_stdout();
    //g_print("********\n");

    g_debug("%02x section %d/%d for nw_id %d, len %d",
            sec->table_id(),
            sec->section_number(), sec->last_section_number(),
            sec->network_id(),
            sec->section_length());

    g_debug("Network descriptors (len %d):",
            sec->network_descriptors_length());
    auto descs = sec->get_network_descriptors();
    for (auto &desc: descs)
    {
        g_debug("0x%02x+%d  ", desc.tag(), desc.length());
    }
    for (auto &desc: descs)
    {
        process_descriptor(desc);
    }
    if (network_name_.size())
        g_debug("Network name '%s'", network_name_.c_str());

    g_debug("Transport streams (len %d):",
            sec->transport_stream_loop_length());
    for (auto &ts: sec->get_transport_stream_loop())
    {
        //g_debug("  TS subsection offset %d len %d (%x) + 6",
        //        ts.get_offset(), ts.word12(4), ts.word12(4));
        //g_debug("  ts.transport_descriptors_length %d",
        //        ts.transport_descriptors_length());
        //g_debug("  from parent section %d",
        //        sec->word12(ts.get_offset() + 4));
        process_ts_data(ts);
    }

    return complete;
}

void NITProcessor::process_ts_data(const TSSectionData &ts)
{
    g_debug("  transport_stream_id: %d,\toriginal_network_id %d",
            ts.transport_stream_id(), ts.original_network_id());
    g_debug("  Transport descriptors (len %d):",
            ts.transport_descriptors_length());
    current_ts_id_ = ts.transport_stream_id();
    auto descs = ts.get_transport_descriptors();
    for (auto &desc: descs)
    {
        g_debug("    0x%02x+%d", desc.tag(), desc.length());
    }
    for (auto &desc: descs)
    {
        process_descriptor(desc);
    }
}

void NITProcessor::process_descriptor(const Descriptor &desc)
{
    switch (desc.tag())
    {
        case Descriptor::NETWORK_NAME:
            if (!network_name_.size())
            {
                network_name_ = NetworkNameDescriptor(desc).get_network_name();
            }
            break;
        case Descriptor::SERVICE_LIST:
            mscanner_->process_service_list_descriptor(current_ts_id_, desc);
            break;
        case Descriptor::TERRESTRIAL_DELIVERY_SYSTEM:
            mscanner_->process_delivery_system_descriptor(current_ts_id_, desc);
            break;
    }
}

}
