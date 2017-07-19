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
#include "sdt-processor.h"

#include "si/service-descriptor.h"

namespace logi
{

bool SDTProcessor::process(std::shared_ptr<SDTSection> sec, MultiScanner *ms)
{
    TableTracker &trkr = sec->table_id() == 0x42 ?
        this_ts_tracker_ : other_ts_tracker_;

    mscanner_ = ms;

    switch (trkr.track(*sec))
    {
        case TableTracker::REPEAT:
            g_debug("Repeat SDT section number %d", sec->section_number());
            return false;
        case TableTracker::REPEAT_COMPLETE:
            g_debug("Repeat and complete SDT (%d)", sec->section_number());
            return this_ts_tracker_.complete() && other_ts_tracker_.complete();
        case TableTracker::COMPLETE:
        case TableTracker::OK:
            break;
        case TableTracker::OLD_VERSION:
            g_debug("Old SDT section version %d", sec->version_number());
            return false;
    }

    /*
    g_print("********\n");
    sec->dump_to_stdout();
    g_print("********\n");
    */

    g_print("%02x SDT section %d/%d for orig_nw_id %d, len %d\n",
            sec->table_id(),
            sec->section_number(), sec->last_section_number(),
            sec->original_network_id(),
            sec->section_length());

    auto services = sec->get_services();

    g_print("%ld services:\n", services.size());
    for (auto &svc: services)
    {
        process_service_data(svc);
    }

    return this_ts_tracker_.complete() && other_ts_tracker_.complete();
}

void SDTProcessor::process_service_data(const SDTSectionServiceData &svc)
{
    g_print("  service_id 0x%04x at offset %d\n",
            svc.service_id(), svc.get_offset());
    auto descs = svc.get_descriptors();
    for (auto &desc: descs)
    {
        process_descriptor(desc);
    }
}

void SDTProcessor::process_descriptor(const Descriptor &desc)
{
    g_print("Descriptor tag %02x size %d @ %d\n",
            desc.tag(), desc.length(), desc.get_offset());
    switch (desc.tag())
    {
        case Descriptor::SERVICE:
            process_service_descriptor(desc);
            break;
    }
}

void SDTProcessor::process_service_descriptor(const Descriptor &desc)
{
    ServiceDescriptor sdesc(desc);
    g_print("  Type %d, provider_name '%s', name '%s'\n",
            sdesc.service_type(),
            sdesc.service_provider_name().c_str(),
            sdesc.service_name().c_str());
}

}
