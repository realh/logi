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

#include <glibmm/convert.h>

#include "decode-string.h"

namespace logi
{

// Only bottom 16 bits of c are significant, this generates a gunichar from
// a 16-bit UTF codepoint.
constexpr static gunichar u(gunichar c)
{
    return c >= 0x800 ?
        (0xe08080 | ((c & 0xf000) << 4) | ((c & 0x0fc0) << 2) | (c & 0x003f))
        : (c > 0x80 ? 
        (0xc080 | ((c & 0x07c0) << 2) | (c & 0x003f))
         : c);
}

// DVB byte to Unicode codepoint mapping for ETSI EN 300 468 table A1,
// for 0xa0 - 0xff.
static guint16 table[] =
{
    0x00a0, 0x00a1, 0x00a2, 0x00a3,     // a0
    0x20ac, 0x00a5, 0x00a6, 0x00a7,     // a4
    0x00a4, 0x2018, 0x201c, 0x00ab,     // a8
    0x2190, 0x2191, 0x2192, 0x2193,     // ac

    0x00b0, 0x00b1, 0x00b2, 0x00b3,     // b0
    0x00d7, 0x00b5, 0x00b6, 0x00b7,     // b4
    0x00f7, 0x2019, 0x201d, 0x00bb,     // b8
    0x00bc, 0x00bd, 0x00be, 0x00bf,     // bf

    // 0xc1 - 0xcf are combining accents, I think these can translate
    // directly into UTF combining characters
    0x00c0, 0x0300, 0x0301, 0x0302,     // c0
    0x0303, 0x0304, 0x0306, 0x0307,     // c4
    0x0308, 0x00c9, 0x030a, 0x0327,     // c8
    0x00cc, 0x030b, 0x0328, 0x030c,     // cc

    0x2014, 0x00b9, 0x00ae, 0x00a9,     // d0
    0x2122, 0x266a, 0x00ac, 0x00a6,     // d4
    0x00d8, 0x00d9, 0x00da, 0x00db,     // d8
    0x215b, 0x215c, 0x215d, 0x215e,     // dc

    0x2126, 0x00c6, 0x0110, 0x00aa,     // e0
    0x0126, 0x00e5, 0x0132, 0x013f,     // e4
    0x0141, 0x00d8, 0x0152, 0x00ba,     // e8
    0x00de, 0x0166, 0x014a, 0x0149,     // ec

    0x0138, 0x00e6, 0x0111, 0x00f0,     // f0
    0x0127, 0x0131, 0x0133, 0x0140,     // f4
    0x0142, 0x00f8, 0x0153, 0x00df,     // f8
    0x00fe, 0x0167, 0x014b, 0x00ad,     // fc
};

static Glib::ustring decode_latin(std::vector<std::uint8_t> v,
        unsigned o, unsigned l)
{
    Glib::ustring s;
    for (auto i = o; i < o + l; ++i)
    {
        auto c = v[i];
        if (c < 0x80)
            s.push_back(char (c));
        else if (c >= 0x80 && c <= 0x9f)        // Control codes
            s.push_back(gunichar(0xc200 + c));
        else 
            s.push_back(u(table[c - 0xa0]));
    }
    return s;
}

static Glib::ustring decode_iso_8859_1(std::vector<std::uint8_t> v,
        unsigned o, unsigned l)
{
    try
    {
        return Glib::convert_with_fallback(std::string(v.begin() + o,
                    v.begin() + o + l), "utf-8", "iso-8859-1");
    }
    catch (Glib::Exception &x)
    {
        return Glib::ustring::compose("Conversion from ISO-8859-1 failed: %1",
                x.what());
    }
}

Glib::ustring decode_string(std::vector<std::uint8_t> v,
        unsigned o, unsigned l)
{
    auto c = v[o];
    if (c >= 0x20)
    {
        return decode_latin(v, o, l);
    }
    else if (c == 0x10)
    {
        if (v[o + 1] == 0 && v[o + 2] == 0)
        {
            return decode_iso_8859_1(v, o + 3, l - 3);
        }
        else
        {
            return Glib::ustring::compose(
                "Can't decode string with encoding %1-%2-%3",
                v[o], v[o + 1], v[o + 2]);
        }
    }
    return Glib::ustring::compose("Can't decode string beginning with %1",
            v[o]);
}

}
