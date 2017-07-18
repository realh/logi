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

#include "service-list-descriptor.h"

namespace logi
{

std::vector<ServiceInfo>
ServiceListDescriptor::get_services() const
{
    std::vector<ServiceInfo> svcs;
    for (unsigned n = 0; n < get_services_length(); n += 3)
    {
        svcs.emplace_back(word16(n + 2), word8(n + 4));
    }
    return svcs;
}

}
