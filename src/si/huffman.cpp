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

/* Freesat Huffman decoding */

#include <glibmm.h>

#include "huffman.h"

namespace logi
{

static const guchar START_TOKEN = 0;
static const guchar STOP_TOKEN = 0;
static const guchar ESC_TOKEN = 1;

extern HuffmanNode *freesat_huffman_table1[],
        *freesat_huffman_table2[];

inline static guint8 get_bit(const guint8 *input,
        int *input_bit, gsize *input_byte)
{
    guint8 token = (input[*input_byte] >> *input_bit) & 1;
    
    if (!(*input_bit)--)
    {
        *input_bit = 7;
        ++*input_byte;
    }
    return token;
}

inline static guint8 get_byte(const guint8 *input, gsize *input_byte,
        int input_bit, gsize input_len)
{
    guint8 token = (input[(*input_byte)++] & ((1 << (input_bit + 1)) - 1)) <<
            (7 - input_bit);
    
    if (input_bit != 7 && *input_byte < input_len)
    {
        token |= (input[*input_byte] >> (input_bit + 1)) &
                ((1 << (7 - input_bit)) - 1);
    }
    return token;
}

inline static char hex(guint8 nibble)
{
    return nibble < 10 ? nibble + '0' : nibble + 'A' - 10;
}

static Glib::ustring &fail(Glib::ustring reason, Glib::ustring &output,
        const guint8 *input, gsize input_len)
{
    output += HUFFMAN_FAIL_STRING;
    output += reason;
    output += " --";
    for (gsize n = 0; n < input_len; ++n)
    {
        output += hex(input[n] >> 4);
        output += hex(input[n] & 15);
    }
    return output;
}

Glib::ustring huffman_decode(const guint8 *input, gsize input_len,
        HuffmanNode **o1table)
{
    guint8 token;
    HuffmanNode *o0table;
    gsize input_byte = 0;
    int input_bit = 7;
    unsigned char prev_char = START_TOKEN;
    Glib::ustring output;
    
    while (1)
    {
        char byte;
        
        while (prev_char < 128 && prev_char != ESC_TOKEN)
        {
            guint8 offset = 0;
            
            if ((o0table = o1table[prev_char]) == NULL)
            {
                return fail(Glib::ustring::compose(
                        "No O1 entry for prev_char %1", (guint) prev_char),
                         output, input, input_len);
            }
            do
            {
                if (input_byte >= input_len)
                {
                    return fail("Bit overrun", output, input, input_len);
                }
                if (get_bit(input, &input_bit, &input_byte))
                    token = o0table[offset].right;
                else
                    token = o0table[offset].left;
                if (token & 128)
                {
                    if (token == (128 | STOP_TOKEN))
                        return output;
                    else if (token != (128 | ESC_TOKEN))
                    {
                        output += char(token & 127);
                    }
                    prev_char = token & 127;
                }
                else
                {
                    offset = token;
                }
            } while (!(token & 128));
        }
        byte = get_byte(input, &input_byte, input_bit, input_len);
        if (input_byte >= input_len && input_bit != 7)
        {
            return fail("Byte overrun", output, input, input_len);
        }
        output += char(byte);
        prev_char = byte;
    }
    return "";
}

}
