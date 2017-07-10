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
    else if (vec[offset] == 0 || vec[0] >= 0x20)
    {
        unsigned n;

        from_enc = "ISO_6937-2";
        /* DVB adds 0xA4 = Euro symbol to the standard. If we left this
         * unchanged, at best g_convert would convert it to the UTF-8 encoding
         * of the currency symbol which is a separate valid character in 6937.
         * So let's replace 0xA4 with 0x90 which should translate to a unique
         * "user-defined" code in UTF-8, which we can convert to Euro after.
         */
        for (n = 0; n < len; ++n)
        {
            if (vec[n + offset] == 0xa4)
            {
                if (!tmp.size())
                {
                    tmp = std::vector<std::uint8_t>(vec.begin() + offset,
                            vec.end());
                }
                tmp[n] = 0x90;
            }
        }
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

    if (vec[offset] == 0x10)
    {
        offset += 3;
        len -= 3;
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
