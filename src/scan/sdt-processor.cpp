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

    g_debug("%02x SDT section %d/%d for orig_nw_id %d, len %d",
            sec->table_id(),
            sec->section_number(), sec->last_section_number(),
            sec->original_network_id(),
            sec->section_length());

    current_ts_id_ = sec->transport_stream_id();

    auto services = sec->get_services();

    g_debug("%ld services", services.size());
    for (auto &svc: services)
    {
        process_service_data(svc);
    }

    return this_ts_tracker_.complete() && other_ts_tracker_.complete();
}

void SDTProcessor::process_service_data(const SDTSectionServiceData &svc)
{
    g_debug("  service_id 0x%04x at offset %d",
            svc.service_id(), svc.get_offset());
    current_service_id_ = svc.service_id();
    auto descs = svc.get_descriptors();
    for (auto &desc: descs)
    {
        process_descriptor(desc);
    }
}

void SDTProcessor::process_descriptor(const Descriptor &desc)
{
    g_debug("    Descriptor tag %02x size %d @ %d",
            desc.tag(), desc.length(), desc.get_offset());
    switch (desc.tag())
    {
        case Descriptor::SERVICE:
            mscanner_->process_service_descriptor(current_ts_id_,
                    current_service_id_, desc);
            break;
    }
}

}
