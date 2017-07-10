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

#include <mutex>
#include <vector>

#include <linux/dvb/dmx.h>
#include <linux/dvb/frontend.h>

#include <glibmm/error.h>

#include "tuning.h"

namespace logi
{

enum class FrontendError
{
    NONE,
    BUSY,
    OPEN,
    READ,
    TUNE,
    FILTER,
};

class Section;

class Frontend
{
private:
    using LockGuard = std::lock_guard<std::recursive_mutex>;
    std::recursive_mutex mtx_;
    const int adapter_;
    const int frontend_;
    int fe_fd_;
    std::string dmx_name_;
public:
    Frontend(int adapter, int frontend);

    ~Frontend();

    Frontend(const Frontend &) = delete;
    Frontend(Frontend &&) = delete;
    Frontend &operator=(const Frontend &) = delete;
    Frontend &operator=(Frontend &&) = delete;

    int get_adapter() const
    {
        return adapter_;
    }

    /**
     * get_frontend:
     * Refers to the frontend number as opposed to this object.
     */
    int get_frontend() const
    {
        return frontend_;
    }

    /**
     * open:
     * This can be called again even if the frontend is already open.
     * Returns: The file handle of the frontend.
     */
    int open();

    /**
     * close:
     * This can be called safely even if the frontend isn't open.
     */
    void close();

    /**
     * enum_delivery_systems:
     * Enumerates the delivery systems supported by this frontend.
     */
    std::vector<fe_delivery_system_t> enum_delivery_systems();

    /**
     * tune:
     * Tunes the frontend. If successful a "lock" signal is raised, otherwise
     * "nolock" is raised after the timeout.
     * @tuning_props: A set of tuning properties.
     * If an error occurs immediately a glib::Error will be raised.
     */
    void tune(const TuningProperties &tuning_props);

    std::string get_device_name(const char *basename) const;

    std::string get_dmx_name();

    Glib::Error report_error(FrontendError code, const char *msg);

    Glib::Error report_errno(FrontendError code, const char *msg);

    static GQuark get_error_quark()
    {
        return g_quark_from_static_string("logi-rontend-error");
    }
};

}
