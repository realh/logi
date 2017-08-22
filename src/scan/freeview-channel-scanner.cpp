/*
    logi - A DVB DVR designed for web-based clients.
    Copyright (C) 2017 Tony Houghton <h@realh.co.uk>

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

#include "freeview-channel-scanner.h"
#include "multi-scanner.h"

namespace logi
{

std::vector<FreeviewLCNPair> FreeviewLCNDescriptor::get_lcn_pairs() const
{
    std::vector<FreeviewLCNPair> lcns;

    for (unsigned o = 2; o < unsigned(length()) + 2; o += 4)
    {
        lcns.emplace_back(word16(o), word16(o + 2));
    }
    return lcns;
}

void FreeviewNITProcessor::process_descriptor(const Descriptor &desc)
{
    if (desc.tag() == FreeviewLCNDescriptor::FREEVIEW_LCN_DESCRIPTOR_TAG)
    {
        FreeviewLCNDescriptor lcn_desc(desc);
        g_debug("  LCNS:");
        for (const auto &l: lcn_desc.get_lcn_pairs())
        {
            g_debug("    sid %04x lcn %d", l.service_id(), l.lcn()); 
            mscanner_->set_lcn(current_nw_id_, l.service_id(), 0, l.lcn(), 0);
        }
    }
    else
    {
        NITProcessor::process_descriptor(desc);
    }
}

std::unique_ptr<NITProcessor> FreeviewChannelScanner::new_nit_processor()
{
    return std::unique_ptr<NITProcessor>(new FreeviewNITProcessor());
}

}
