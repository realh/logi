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

#include <initializer_list>
#include <string>
#include <vector>

#include <linux/dvb/frontend.h>

#include <glibmm/error.h>

namespace logi
{

enum class TuningError
{
    PARSE,
};

/**
 * TuningProperties:
 * Encapsulates frontend tuning properties and provides related utilities.
 */
class TuningProperties
{
private:
    struct dtv_properties props_;
    std::vector<struct dtv_property> props_v_;
public:
    TuningProperties() : props_{ .num = 0, .props = nullptr }
    {}

    /**
     * TuningProperties:
     * @T: Iterable container of std::pair<guint32, guint32>.
     * @props: A list of {cmd, data} properties.
     */
    TuningProperties(std::initializer_list<std::pair<guint32,guint32>> &&props);

    /**
     * @tuning_str: In the format used by other Linux DVB tools.
     */
    TuningProperties(const char *tuning_str);

    TuningProperties(const TuningProperties &other);
    TuningProperties(TuningProperties &&other);

    TuningProperties &operator=(const TuningProperties &other);
    TuningProperties &operator=(TuningProperties &&other);

    /**
     * Returns: Whether the properties are equivalent. Only checks a few key
     * properties, and frequencies are rounded.
     */
    bool operator==(const TuningProperties &other) const;

    /**
     * Returns: A rounded frequency or'd with other bits for testing
     * equivalence.
     */
    std::uint32_t get_equivalence_value() const;

    struct dtv_properties const *get_props() const
    {
        return &props_;
    }

    /**
     * Returns: A brief description of the frequency etc.
     */
    std::string describe() const;

    /**
     * Returns: A string in the format traditionally used by Linux DVB apps.
     */
    std::string linuxtv_description() const;

    void clear();

    static GQuark get_error_quark();

    operator bool() const
    {
        return props_.num;
    }

    // Public version also updates props_
    void append_prop(guint32 cmd, guint32 data);
private:
    // Private version doesn't update props_
    void append_prop_priv(guint32 cmd, guint32 data);

    void fix_props();

    /**
     * @div: Value to divide frequency by to get MHz.
     */
    void query_key_props(fe_delivery_system_t &t, guint32 &f,
            fe_sec_voltage_t &v, guint32 &div) const;

    void parse_dvb_s(guint n, char **tokens, const char *tuning_str);

    static guint32 parse_number(const char *fragment, const char *whole);

    static guint32 parse_code_rate(const char *fragment, const char *whole);

    static guint32 parse_roll_off(const char *fragment, const char *whole);

    static guint32 parse_modulation(const char *fragment, const char *whole);

    static Glib::Error report_error(TuningError code, const char *desc);

    static Glib::Error report_parse_error(const char *desc,
            const char *tuning_str);
};

}
