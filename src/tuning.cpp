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

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <sstream>

#include "tuning.h"

namespace logi
{

struct prop_table_t_ {
    std::uint32_t v;
    const char *s;
};

static struct prop_table_t_ code_rate_table[] = {
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

static struct prop_table_t_ modulation_table[] = {
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

static struct prop_table_t_ guard_interval_table[] = {
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

static struct prop_table_t_ hierarchy_table[] = {
    { HIERARCHY_NONE, "NONE" },
    { HIERARCHY_1, "1" },
    { HIERARCHY_2, "2" },
    { HIERARCHY_4, "4" },
    { HIERARCHY_AUTO, "AUTO" },
    { HIERARCHY_AUTO, NULL },
};

static struct prop_table_t_ transmission_mode_table[] = {
    { TRANSMISSION_MODE_2K, "2K"},
    { TRANSMISSION_MODE_8K, "8K"},
    { TRANSMISSION_MODE_4K, "4K"},
    { TRANSMISSION_MODE_1K, "1K"},
    { TRANSMISSION_MODE_16K, "16K"},
    { TRANSMISSION_MODE_32K, "32K"},
    { TRANSMISSION_MODE_AUTO, "AUTO"},
    { TRANSMISSION_MODE_AUTO, NULL}
};

static struct prop_table_t_ roll_off_table[] = {
    { ROLLOFF_35, "35"},
    { ROLLOFF_20, "20"},
    { ROLLOFF_25, "25"},
    { ROLLOFF_AUTO, "AUTO"},
    { ROLLOFF_AUTO, NULL}
};



TuningProperties::TuningProperties(const char *s)
{
    if (s[0] != 'S' && s[0] != 'T')
    {
        throw report_parse_error("Only satellite and terrestrial are supported",
                s);
    }

    std::unique_ptr<char *, void (*)(char **)>
        tokens(g_strsplit(s, " ", 0), g_strfreev);

    // Allow # comments at end of lines
    gboolean comment = FALSE;
    int n;
    for (n = 0; tokens.get()[n]; ++n)
    {
        if (tokens.get()[n][0] == '#')
        {
            comment = TRUE;
        }
        // Free all members from comment onwards
        if (comment)
        {
            g_free(tokens.get()[n]);
            tokens.get()[n] = NULL;
        }
    }

    if (s[0] == 'S')
        parse_dvb_s(n, tokens.get(), s);
    else
        parse_dvb_t(n, tokens.get(), s);

    append_prop_priv(DTV_INVERSION, INVERSION_AUTO);
    append_prop_priv(DTV_TUNE, 1);

    fix_props();
}

TuningProperties::TuningProperties(
        std::initializer_list<std::pair<guint32, guint32> >&&props)
{
    for (const auto &prop: props)
    {
        append_prop_priv(prop.first, prop.second);
    }
    fix_props();
}

TuningProperties::TuningProperties(const TuningProperties &other) :
    props_v_(other.props_v_)
{
    props_.props = props_v_.data();
    props_.num = props_v_.size();
}

TuningProperties::TuningProperties(TuningProperties &&other) :
    props_v_(std::move(other.props_v_))
{
    props_.props = props_v_.data();
    props_.num = props_v_.size();
    other.props_.props = nullptr;
    other.props_.num = 0;
}

TuningProperties &TuningProperties::operator=(const TuningProperties &other)
{
    props_v_ = other.props_v_;
    props_.props = props_v_.data();
    props_.num = props_v_.size();
    return *this;
}

TuningProperties &TuningProperties::operator=(TuningProperties &&other)
{
    props_v_ = std::move(other.props_v_);
    props_.props = props_v_.data();
    props_.num = props_v_.size();
    other.props_.props = nullptr;
    other.props_.num = 0;
    return *this;
}

bool TuningProperties::operator==(const TuningProperties &other) const
{
    fe_delivery_system_t t1, t2;
    guint32 f1, f2;
    fe_sec_voltage_t v1, v2;
    guint32 d1, d2;

    query_key_props(t1, f1, v1, d1);
    other.query_key_props(t2, f2, v2, d2);

    return t1 == t2 && (f1 / d1 / 2 == f2 / d2 / 2) && v1 && v2;
}

std::uint32_t TuningProperties::get_equivalence_value() const
{
    fe_delivery_system_t t1;
    guint32 f1;
    fe_sec_voltage_t v1;
    guint32 d1;

    query_key_props(t1, f1, v1, d1);
    //bool gen2 = d1 == SYS_DVBT2 || d1 == SYS_DVBS2;

    return (f1 / d1 / 2) | (v1 << 29) /* | (gen2 ? (1 << 28) : 0) */;
}

std::string TuningProperties::describe() const
{
    fe_delivery_system_t t;
    guint32 f;
    fe_sec_voltage_t v;
    guint32 d;

    query_key_props(t, f, v, d);

    char *s = nullptr;

    if (t == SYS_DVBS)
    {
        s = g_strdup_printf("%dMHz %c", int((float) f / (float) d),
                v == SEC_VOLTAGE_13 ? 'V' : 'H');
    }
    else if (t == SYS_DVBT)
    {
        float ff = ((float) f / 1000000.0f - 306.0f) / 8.0f;
        if (ff == floor(ff) && ff >= 21.0f && ff <= 67.0f)
        {
            s = g_strdup_printf("%dMHz (chan %d)", (f / d), (int) ff);
        }
    }
    if (!s)
        s = g_strdup_printf("%dMHz", (f / d));
    std::string result{s};
    g_free(s);
    return result;
}

void TuningProperties::clear()
{
    props_.num = 0;
    props_.props = nullptr;
    props_v_.clear();
}

GQuark TuningProperties::get_error_quark()
{
    return g_quark_from_static_string("logi-tuning-properties-error");
}

void TuningProperties::query_key_props(fe_delivery_system_t &t, guint32 &f,
        fe_sec_voltage_t &v, guint32 &d) const
{
    t = SYS_UNDEFINED;
    f = 0;
    v = SEC_VOLTAGE_OFF;
    d = 1000000;

    for (guint n = 0; n < props_v_.size(); ++n)
    {
        switch (props_v_[n].cmd)
        {
            case DTV_DELIVERY_SYSTEM:
                t = (fe_delivery_system_t) props_v_[n].u.data;
                switch (t)
                {
                    case SYS_DVBS2:
                        t = SYS_DVBS;
                    case SYS_DVBS:
                        d = 1000;
                        break;
                    case SYS_DVBT2:
                        t = SYS_DVBT;
                    default:
                        d = 1000000;
                        break;
                }
                break;
            case DTV_FREQUENCY:
                f = props_v_[n].u.data;
                break;
            case DTV_VOLTAGE:
                v = (fe_sec_voltage_t) props_v_[n].u.data;
                break;
            default:
                break;
        }
    }
}

void TuningProperties::parse_dvb_t(unsigned n, char **tokens, const char *s)
{
    if (n < 9)
    {
        throw report_parse_error("Too few parameters", s);
    }

    std::uint32_t val;
    unsigned i;

    switch (tokens[0][1])
    {
        case '2':
            val = SYS_DVBT2;
            break;
        case '1':
            val = SYS_DVBT;
            break;
        case 0:
            val = n > 9 ? SYS_DVBT2 : SYS_DVBT;
            break;
        default:
            throw report_parse_error("Invalid type", s);
    }
    append_prop_priv(DTV_DELIVERY_SYSTEM, val);

    if (val == SYS_DVBT2)
    {
        i = 3;
        append_prop_priv(DTV_STREAM_ID, parse_number(tokens[1], s));
    }
    else
    {
        i = 1;
    }

    val = parse_number(tokens[i], s);
    if (val < 1000)
        val *= 1000000;
    else if (val < 1000000)
        val *= 1000;
    append_prop_priv(DTV_FREQUENCY, val);

    val = parse_bandwidth(tokens[i + 1], s);
    if (val != G_MAXUINT32)
        append_prop_priv(DTV_BANDWIDTH_HZ, val);

    append_prop_priv(DTV_CODE_RATE_HP, parse_code_rate(tokens[i + 2], s));
    append_prop_priv(DTV_CODE_RATE_LP, parse_code_rate(tokens[i + 3], s));
    append_prop_priv(DTV_MODULATION, parse_modulation(tokens[i + 4], s));
    append_prop_priv(DTV_TRANSMISSION_MODE,
            parse_transmission_mode(tokens[i + 5], s));
    append_prop_priv(DTV_GUARD_INTERVAL,
            parse_guard_interval(tokens[i + 6], s));
    append_prop_priv(DTV_HIERARCHY, parse_hierarchy(tokens[i + 7], s));

    fix_props();
}

void TuningProperties::parse_dvb_s(guint n, char **tokens, const char *s)
{
    constexpr static long SLOF = 11700000;
    constexpr static long LOF1 = 9750000;
    constexpr static long LOF2 = 10600000;

    if (n < 5)
    {
        throw report_parse_error("Too few parameters", s);
    }

    guint32 val;
    switch (tokens[0][1])
    {
        case '2':
            val = SYS_DVBS2;
            break;
        case '1':
            val = SYS_DVBS;
            break;
        case 0:
            val = n > 5 ? SYS_DVBS2 : SYS_DVBS;
            break;
        default:
            throw report_parse_error("Invalid type", s);
    }
    append_prop_priv(DTV_DELIVERY_SYSTEM, val);

    val = parse_number(tokens[1], s);
    if (val < SLOF)
    {
        val -= LOF1;
    }
    else
    {
        val -= LOF2;
    }
    append_prop_priv(DTV_FREQUENCY, val);

    if (tokens[2][0] == 'H' || tokens[2][0] == 'L')
        val = SEC_VOLTAGE_18;
    else if (tokens[2][0] == 'V' || tokens[2][0] == 'R')
        val = SEC_VOLTAGE_13;
    else
        throw report_parse_error("Invalid polarity", s);
    append_prop_priv(DTV_VOLTAGE, val);

    append_prop_priv(DTV_SYMBOL_RATE, parse_number(tokens[3], s));
    append_prop_priv(DTV_INNER_FEC, parse_code_rate(tokens[4], s));

    if (n > 5 && tokens[0][1] != '1')
    {
        append_prop_priv(DTV_PILOT, PILOT_AUTO);
        append_prop_priv(DTV_ROLLOFF, parse_roll_off(tokens[5], s));
        append_prop_priv(DTV_MODULATION, parse_modulation(tokens[6], s));
    }

    fix_props();
}

guint32 TuningProperties::parse_number(const char *n, const char *s)
{
    char *endptr;
    long result = std::strtol(n, &endptr, 10);

    if (endptr == n || *endptr != 0)
        throw report_parse_error("Invalid number", s);
    return (guint32) result;
}

guint32 TuningProperties::parse_bandwidth(const char *n, const char *)
{
    int l;
    double result;
    char *end = NULL;

    if (!strcmp(n, "AUTO"))
        return G_MAXUINT32;
    l = strlen(n);
    if (n[l - 3] != 'M' || n[l - 2] != 'H' ||
            (n[l - 1] != 'z' && n[l - 1] != 'Z'))
    {
        return G_MAXUINT32;
    }
    if (!strncmp(n, "1.712", 5))
        return 1712000;
    result = strtod(n, &end);
    if (result == 0 && end == n)
        return G_MAXUINT32;
    return (guint32) (result * 1000000);
}

guint32 TuningProperties::parse_code_rate(const char *v, const char *s)
{
    for (int n = 0; code_rate_table[n].s; ++n)
    {
        if (!std::strcmp(code_rate_table[n].s, v))
            return code_rate_table[n].v;
    }
    throw report_parse_error("Invalid code rate", s);
}

guint32 TuningProperties::parse_transmission_mode(const char *fragment,
            const char *whole)
{
    int n;

    for (n = 0; transmission_mode_table[n].s; ++n)
    {
        if (!std::strcmp(transmission_mode_table[n].s, fragment))
            return transmission_mode_table[n].v;
    }
    throw report_parse_error("Invalid transmission mode", whole);
}

guint32 TuningProperties::parse_guard_interval(const char *fragment,
            const char *whole)
{
    int n;

    for (n = 0; guard_interval_table[n].s; ++n)
    {
        if (!std::strcmp(guard_interval_table[n].s, fragment))
            return guard_interval_table[n].v;
    }
    throw report_parse_error("Invalid guard_interval", whole);
}

guint32 TuningProperties::parse_hierarchy(const char *fragment,
            const char *whole)
{
    int n;

    for (n = 0; hierarchy_table[n].s; ++n)
    {
        if (!std::strcmp(hierarchy_table[n].s, fragment))
            return hierarchy_table[n].v;
    }
    throw report_parse_error("Invalid hierarchy", whole);
}

guint32 TuningProperties::parse_roll_off(const char *v, const char *s)
{
    int n;

    for (n = 0; roll_off_table[n].s; ++n)
    {
        if (!std::strcmp(roll_off_table[n].s, v))
            return roll_off_table[n].v;
    }
    throw report_parse_error("Invalid roll-off", s);
}

guint32 TuningProperties::parse_modulation(const char *v, const char *s)
{
    int n;

    for (n = 0; modulation_table[n].s; ++n)
    {
        if (!std::strcmp(modulation_table[n].s, v))
            return modulation_table[n].v;
    }
    throw report_parse_error("Invalid modulation", s);
}

void TuningProperties::append_prop(guint32 cmd, guint32 data)
{
    auto &back = props_v_.back();
    if (back.cmd == DTV_TUNE)
    {
        back.cmd = cmd;
        back.u.data = data;
    }
    else
    {
        append_prop_priv(cmd, data);
    }
    fix_props();
}

void TuningProperties::append_prop_priv(guint32 cmd, guint32 data)
{
    auto l = props_v_.size();

    props_v_.resize(l + 1);
    props_v_[l].cmd = cmd;
    props_v_[l].u.data = data;
}

void TuningProperties::fix_props()
{
    if (props_v_.back().cmd != DTV_TUNE)
        append_prop_priv(DTV_TUNE, 1);
    props_.props = props_v_.data();
    props_.num = props_v_.size();
}

Glib::Error TuningProperties::report_error(TuningError code, const char *desc)
{
    char *s = g_strdup_printf("Tuning props error: %s", desc);
    Glib::Error err(get_error_quark(), (int) code, s);
    g_free(s);
    return err;
}

Glib::Error TuningProperties::report_parse_error(const char *desc,
        const char *ts)
{
    char *s = g_strdup_printf("Error parsing tuing string '%s': %s", ts, desc);
    Glib::Error err(get_error_quark(), (int) TuningError::PARSE, s);
    g_free(s);
    return err;
}

static const char *
lookup_prop_val_name(TuningProperties::prop_map_t &m,
        const prop_table_t_ *table, std::uint32_t key)
{
    if (!m.count(key))
        return "AUTO";
    auto val = m[key];
    unsigned n;
    for (n = 0; table[n].s != NULL; ++n)
    {
        if (table[n].v == val)
            return table[n].s;
    }
    return "AUTO";
}

static std::string dvbt_description(TuningProperties::prop_map_t &m)
{
    std::ostringstream s;

    if (m[DTV_DELIVERY_SYSTEM] == SYS_DVBT2)
    {
        s << "T2";
        if (m.count(DTV_STREAM_ID))
            s << " " << m[DTV_STREAM_ID];
        else
            s << " 0";
        /* We can't currently read system_id from tuner */
        s << " 0";
    }
    else
    {
        s << "T";
    }
    s << " " << m[DTV_FREQUENCY];
    int bw = m[DTV_BANDWIDTH_HZ];

    s << " " << ((bw == 1712000) ? 1.712 : bw / 1000000) << "MHZ";
    s << " " << lookup_prop_val_name(m, code_rate_table, DTV_CODE_RATE_HP);
    s << " " << lookup_prop_val_name(m, code_rate_table, DTV_CODE_RATE_LP);
    s << " " << lookup_prop_val_name(m, modulation_table, DTV_MODULATION);
    s << " " << lookup_prop_val_name(m, transmission_mode_table,
            DTV_TRANSMISSION_MODE);
    s << " " << lookup_prop_val_name(m, guard_interval_table,
            DTV_GUARD_INTERVAL);
    s << " " << lookup_prop_val_name(m, hierarchy_table, DTV_HIERARCHY);
    return s.str();
}

std::string TuningProperties::linuxtv_description() const
{
    auto pmap = map_props();
    auto ds = pmap[DTV_DELIVERY_SYSTEM];
    if (ds == SYS_DVBT || ds == SYS_DVBT2)
    {
        return dvbt_description(pmap);
    }
    else
    {
        return describe();
    }
}

TuningProperties::prop_map_t &TuningProperties::map_props(prop_map_t &m) const
{
    for (unsigned n = 0; n < props_.num; ++n)
    {
        if (props_v_[n].cmd != DTV_TUNE)
            m[props_v_[n].cmd] = props_v_[n].u.data;
    }
    return m;
}

TuningProperties &TuningProperties::merge(const TuningProperties &other)
{
    auto m = other.map_props();
    map_props(m);
    props_v_.clear();
    for (const auto &p: m)
    {
        append_prop_priv(p.first, p.second);
    }
    return *this;
}

}
