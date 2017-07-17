/*
    logi - A DVB DVR designed for web-based clients.
    Copyright (C) 2012-2017 Tony Houghton <h@realh.co.uk>

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

#include <glibmm.h>

#include "decode-string.h"
#include "huffman.h"

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
constexpr static guint16 table[] =
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

Glib::ustring iso8859_n(int n)
{
    return Glib::ustring::compose("ISO_8859-%1", n);
}

Glib::ustring decode_string(const std::vector<std::uint8_t> &vec,
        unsigned offset, unsigned len)
{
    Glib::ustring from_enc;
    Glib::ustring decoded;
    std::vector<std::uint8_t> tmp;

    if (!len)
        return "NULL string";
    if (vec[offset] == 0x1f)
    {
        if (vec[offset + 1] == 1)
            return huffman_decode(vec.data() + offset, len, huffman_table1);
        else if (vec[offset + 1] == 2)
            return huffman_decode(vec.data() + offset, len, huffman_table2);
        else
            return "Invalid Huffman table number";
    }
    else if (vec[offset] >= 0x20)
    {
        return decode_latin(vec, offset, len);
    }
    else if (vec[offset] <= 0xb)
        from_enc = iso8859_n(vec[offset] + 4);
    else if (vec[offset] == 0x10 && vec[offset + 1] == 0 &&
            vec[offset + 2] > 0 && vec[offset + 2] <= 0xf)
        from_enc = iso8859_n(vec[offset + 2]);
    else if (vec[offset] == 0x11)
        from_enc = "ISO-10646";
    else if (vec[offset] == 0x13)
        from_enc = "GB2312";
    else if (vec[offset] == 0x14)
        from_enc = "BIG-5";
    else if (vec[offset] == 0x15)
        from_enc = "ISO-10646/UTF-8";
    else if (vec[offset] == 0x1f)
        from_enc = iso8859_n(vec[offset + 1]);

    if (vec[offset] == 0x10)
    {
        offset += 3;
        len -= 3;
    }
    else if (vec[offset] == 0x1f)
    {
        offset += 2;
        len -= 2;
    }
    else if (vec[offset] < 0x20)
    {
        offset += 1;
        offset -= 1;
    }

    std::string s(reinterpret_cast<const char *>(
                tmp.size() ? tmp.data() + offset : vec.data() + offset),
                len);

    if (from_enc.size())
    {
        try
        {
            decoded = Glib::convert_with_fallback(s, "UTF-8", from_enc);
        }
        catch (Glib::ConvertError &e)
        {
            g_warning("Failure decoding string which was supposedly %s: %s",
                    from_enc.c_str(), e.what().c_str());
            return Glib::ustring::compose("Failure (%1): %2",
                    from_enc, e.what());
        }
        return decoded;
    }
    else
    {
        decoded = s;
    }

    /* g_convert already usefully converts 0x80-0x9f to 0xc2,0x80-0x9f but we
     * now need to check for DVB's Euro extension to ISO_6937, which we replaced
     * with 0x90 above.
     */
    if (tmp.size())
    {
        const char *dec = decoded.data();
        int n, m;
        int dl = decoded.size();
        int n_euros = 0;

        for (n = 0; n < dl; ++n)
        {
            if ((guint8) dec[n] == 0xc2 && (guint8) dec[n + 1] == 0x90)
                n_euros += 1;
        }
        auto dec2 = std::vector<char>(dl + n_euros);
        for (m = n = 0; n < dl; ++n, ++m)
        {
            if ((guint8) dec[n] == 0xc2 && (guint8) dec[n + 1] == 0x90)
            {
                dec2[m++] = 0xe2;
                dec2[m++] = 0x82;
                dec2[m] = 0xac;
                ++n;
            }
            else
            {
                dec2[m] = dec[n];
            }
        }
        return Glib::ustring(dec2.begin(), dec2.end());
    }
    return decoded;
}

}
