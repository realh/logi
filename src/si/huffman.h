#pragma once
/*
    logi - A DVB DVR designed for web-based clients.
    Copyright (C) 2012, 2017 Tony Houghton <h@realh.co.uk>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/* Freesat (and Freeview HD) Huffman decoding */

#include <glibmm.h>

namespace logi
{

struct HuffmanNode {
    guint8 left, right;
} ;

extern HuffmanNode *huffman_table1[];
extern HuffmanNode *huffman_table2[];

Glib::ustring huffman_decode(const guint8 *input, gsize input_len,
        HuffmanNode **o1table);

inline Glib::ustring huffman_decode(const std::vector<guint8> &input,
        gsize input_len, HuffmanNode **o1table)
{
    return huffman_decode(input.data(), input_len, o1table);
}

}
