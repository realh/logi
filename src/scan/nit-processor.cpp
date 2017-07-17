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
            g_print("Repeat NIT section number %d\n", sec->section_number());
            return false;
        case TableTracker::REPEAT_COMPLETE:
            g_print("Repeat and complete (%d)\n", sec->section_number());
            return true;
        case TableTracker::COMPLETE:
            complete = true;
        case TableTracker::OK:
            break;
        case TableTracker::OLD_VERSION:
            g_print("Old NIT section version %d\n", sec->version_number());
            return false;
    }

    //g_print("********\n");
    //sec->dump_to_stdout();
    //g_print("********\n");

    g_print("%02x section %d/%d for nw_id %d, len %d\n",
            sec->table_id(),
            sec->section_number(), sec->last_section_number(),
            sec->network_id(),
            sec->section_length());

    g_print("Network descriptors (len %d):\n  ",
            sec->network_descriptors_length());
    auto descs = sec->get_network_descriptors();
    for (auto &desc: descs)
    {
        g_print("0x%02x+%d  ", desc.tag(), desc.length());
    }
    g_print("\n");
    for (auto &desc: descs)
    {
        process_descriptor(desc);
    }
    g_print("\n");
    if (network_name_.size())
        g_print("Network name '%s'\n", network_name_.c_str());

    g_print("Transport streams (len %d):\n",
            sec->transport_stream_loop_length());
    for (auto &ts: sec->get_transport_stream_loop())
    {
        //g_print("  TS subsection offset %d len %d (%x) + 6\n",
        //        ts.get_offset(), ts.word12(4), ts.word12(4));
        //g_print("  ts.transport_descriptors_length %d\n",
        //        ts.transport_descriptors_length());
        //g_print("  from parent section %d\n",
        //        sec->word12(ts.get_offset() + 4));
        process_ts_data(ts);
    }
    g_print("\n\n");

    return complete;
}

void NITProcessor::process_ts_data(const TSSectionData &ts)
{
    g_print("  transport_stream_id: %d,\toriginal_network_id %d\n",
            ts.transport_stream_id(), ts.original_network_id());
    g_print("  Transport descriptors (len %d):\n    ",
            ts.transport_descriptors_length());
    current_ts_id_ = ts.transport_stream_id();
    auto descs = ts.get_transport_descriptors();
    for (auto &desc: descs)
    {
        g_print("0x%02x+%d  ", desc.tag(), desc.length());
    }
    g_print("\n");
    for (auto &desc: descs)
    {
        process_descriptor(desc);
    }
    g_print("\n\n");
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
