/*
    logi - A DVB DVR designed for web-based clients.
    Copyright (C) 2016 Tony Houghton <h@realh.co.uk>

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

#include <unistd.h>

#include <glib.h>

#include "section.h"

namespace logi
{

int Section::read_from_fd(int fd)
{
    // Maybe we should check bytes read match length field
    return ::read(fd, sec_.data(), sec_.size());
}

void Section::dump_to_stdout() const
{
    int i;

    for (i = 0; i < section_length(); ++i)
    {
        g_print("%02x  ", sec_[i]);
        if ((i % 16) == 15)
            g_print("\n\n");
    }
    if (i % 16)
        g_print("\n");
}

}
