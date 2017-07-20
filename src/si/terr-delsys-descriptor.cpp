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

#include <utility>

#include "terr-delsys-descriptor.h"

#include "tuning.h"

namespace logi
{

fe_modulation_t TerrestrialDeliverySystemDescriptor::constellation() const
{
    switch ((word8(7) & 0xc0) >> 6)
    {
        case 0:
            return QPSK;
        case 1:
            return QAM_16;
        case 2:
            return QAM_64;
    }
    return QAM_AUTO;
}

fe_hierarchy_t TerrestrialDeliverySystemDescriptor::hierarchy() const
{
    switch ((word8(7) & 0x18) >> 3)
    {
        case 0:
            return HIERARCHY_NONE;
        case 1:
            return HIERARCHY_1;
        case 2:
            return HIERARCHY_2;
        case 3:
            return HIERARCHY_4;
    }
    return HIERARCHY_AUTO;
}

fe_guard_interval_t TerrestrialDeliverySystemDescriptor::guard_interval() const
{
    switch ((word8(8) & 0x18) >> 3)
    {
        case 0:
            return GUARD_INTERVAL_1_32;
        case 1:
            return GUARD_INTERVAL_1_16;
        case 2:
            return GUARD_INTERVAL_1_8;
        case 3:
            return GUARD_INTERVAL_1_4;
    }
    return GUARD_INTERVAL_AUTO;
}

fe_transmit_mode_t
TerrestrialDeliverySystemDescriptor::transmission_mode() const
{
    switch ((word8(8) & 6) >> 1)
    {
        case 0:
            return TRANSMISSION_MODE_2K;
        case 1:
            return TRANSMISSION_MODE_8K;
        case 2:
            return TRANSMISSION_MODE_4K;
    }
    return TRANSMISSION_MODE_AUTO;
}

/*
struct code_rate_table_t {
    fe_code_rate_t v;
    const char *s;
};

static struct code_rate_table_t code_rate_table[] = {
    { FEC_NONE, "NONE" },
    { FEC_1_2, "1/2" },
    { FEC_2_3, "2/3" },
    { FEC_3_4, "3/4" },
    { FEC_4_5, "4/5" },
    { FEC_5_6, "5/6" },
    { FEC_6_7, "6/7" },
    { FEC_7_8, "7/8" },
    { FEC_8_9, "8/9" },
    { FEC_AUTO, "AUTO" },
    { FEC_3_5, "3/5" },
    { FEC_9_10, "9/10" },
    { FEC_AUTO, NULL },
};

struct modulation_table_t {
    fe_modulation_t v;
    const char *s;
};

static struct modulation_table_t modulation_table[] = {
    { QPSK, "QPSK" },
    { QAM_16, "QAM16" },
    { QAM_32, "QAM32" },
    { QAM_64, "QAM64" },
    { QAM_128, "QAM128" },
    { QAM_256, "QAM256" },
    { QAM_AUTO, "AUTO" },
    { VSB_8, "8VSB" },
    { VSB_16, "16VSB" },
    { PSK_8, "8PSK" },
    { APSK_16, "APSK16" },
    { APSK_32, "APSK32" },
    { DQPSK, "DQPSK" },
    { QAM_AUTO, NULL },
};

struct guard_interval_table_t {
    fe_guard_interval_t v;
    const char *s;
};

static struct guard_interval_table_t guard_interval_table[] = {
    { GUARD_INTERVAL_1_32, "1/32" },
    { GUARD_INTERVAL_1_16, "1/16" },
    { GUARD_INTERVAL_1_8, "1/8" },
    { GUARD_INTERVAL_1_4, "1/4" },
    { GUARD_INTERVAL_1_128, "1/128" },
    { GUARD_INTERVAL_19_128, "19/128" },
    { GUARD_INTERVAL_19_256, "19/256" },
    { GUARD_INTERVAL_AUTO, "AUTO" },
    { GUARD_INTERVAL_AUTO, NULL },
};

struct hierarchy_table_t {
    fe_hierarchy_t v;
    const char *s;
};

static struct hierarchy_table_t hierarchy_table[] = {
    { HIERARCHY_NONE, "NONE" },
    { HIERARCHY_1, "1" },
    { HIERARCHY_2, "2" },
    { HIERARCHY_4, "4" },
    { HIERARCHY_AUTO, "AUTO" },
    { HIERARCHY_AUTO, NULL },
};

struct transmission_mode_table_t {
    fe_transmit_mode_t v;
    const char *s;
};

static struct transmission_mode_table_t transmission_mode_table[] = {
    { TRANSMISSION_MODE_2K, "2K"},
    { TRANSMISSION_MODE_8K, "8K"},
    { TRANSMISSION_MODE_4K, "4K"},
    { TRANSMISSION_MODE_1K, "1K"},
    { TRANSMISSION_MODE_16K, "16K"},
    { TRANSMISSION_MODE_32K, "32K"},
    { TRANSMISSION_MODE_AUTO, "AUTO"},
    { TRANSMISSION_MODE_AUTO, NULL}
};

struct roll_off_table_t {
    fe_rolloff_t v;
    const char *s;
};

static struct roll_off_table_t roll_off_table[] = {
    { ROLLOFF_35, "35"},
    { ROLLOFF_20, "20"},
    { ROLLOFF_25, "25"},
    { ROLLOFF_AUTO, "AUTO"},
    { ROLLOFF_AUTO, NULL}
};

struct generic_table_t { int val; const char *name; };

static const char *
lookup_prop_name(gconstpointer table_addr, int val)
{
    generic_table_t const * table = (generic_table_t const *) table_addr;
    int n;

    for (n = 0; table[n].name != NULL; ++n)
    {
        if (table[n].val == val)
            return table[n].name;
    }
    return "AUTO";
}
*/

TuningProperties *
TerrestrialDeliverySystemDescriptor::get_tuning_properties() const
{
    /*
    int bw = bandwidth();
    char *bw_s = (bw == 1712000) ? g_strdup("1.712") :
            g_strdup_printf("%d", bw / 1000000);
    g_print("T %d %sMHz %s %s %s %s %s %s\n",
            centre_frequency(),
            bw_s,
            lookup_prop_name(code_rate_table, code_rate_hp()),
            lookup_prop_name(code_rate_table, code_rate_lp()),
            lookup_prop_name(modulation_table, constellation()),
            lookup_prop_name(transmission_mode_table, transmission_mode()),
            lookup_prop_name(guard_interval_table, guard_interval()),
            lookup_prop_name(hierarchy_table, hierarchy()));
    */
    return new TuningProperties({
        { DTV_DELIVERY_SYSTEM, SYS_DVBT },
        { DTV_FREQUENCY, centre_frequency()},
        { DTV_BANDWIDTH_HZ, bandwidth() },
        { DTV_MODULATION, constellation() },
        { DTV_HIERARCHY, hierarchy() },
        { DTV_CODE_RATE_HP, code_rate_hp() },
        { DTV_CODE_RATE_LP, code_rate_lp() },
        { DTV_GUARD_INTERVAL, guard_interval() },
        { DTV_TRANSMISSION_MODE, transmission_mode() },
    });
    /*
    return new TuningProperties({
        { DTV_DELIVERY_SYSTEM, SYS_DVBT },
        { DTV_FREQUENCY, centre_frequency()},
        //{ DTV_BANDWIDTH_HZ, BANDWIDTH_AUTO },
        { DTV_MODULATION, QAM_AUTO },
        { DTV_HIERARCHY, HIERARCHY_AUTO },
        { DTV_CODE_RATE_HP, FEC_AUTO },
        { DTV_CODE_RATE_LP, FEC_AUTO },
        { DTV_GUARD_INTERVAL, GUARD_INTERVAL_AUTO },
        { DTV_TRANSMISSION_MODE, TRANSMISSION_MODE_AUTO }
    });
    */
}

}
