#pragma once

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

#include <vector>

#include <glibmm/ustring.h>

namespace logi
{

/**
 * decode_string:
 * Global function to decode a string from SI data.
 * @vec: Vector of bytes holding the string data.
 * @offset: Offset of start of string in the vector
 *          (including any leading control code).
 * @len: Length of SI-encoded string (including above control code).
 */
Glib::ustring decode_string(std::vector<std::uint8_t> vec,
        unsigned offset, unsigned len);

}
