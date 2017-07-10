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

#include <utility>

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "frontend.h"

namespace logi
{

Frontend::Frontend(int adapter, int frontend) :
    adapter_{adapter}, frontend_{frontend}, fe_fd_{-1}
{
}

Frontend::~Frontend()
{
    close();
}

int Frontend::open()
{
    LockGuard lock(mtx_);
    
    if (fe_fd_ == -1)
    {
        fe_fd_ = ::open(get_device_name("frontend").c_str(), O_RDWR);
        if (fe_fd_ == -1)
        {
            throw report_errno(FrontendError::OPEN, "Unable to open frontend");
        }
    }
    return fe_fd_;
}

void Frontend::close()
{
    LockGuard lock(mtx_);
    
    if (fe_fd_ != -1)
    {
        ::close(fe_fd_);
        fe_fd_ = -1;
    }
}

std::vector<fe_delivery_system_t> Frontend::enum_delivery_systems()
{
    struct dtv_property prop = { .cmd = DTV_ENUM_DELSYS };
    struct dtv_properties props = { .num = 1, .props = &prop };

    mtx_.lock();
    int fd = ::ioctl(open(), FE_GET_PROPERTY, &props);
    mtx_.unlock();
    if (fd == -1)
    {
        throw report_errno(FrontendError::READ,
                "Unable to enumerate delivery systems");
    }

    std::vector<fe_delivery_system_t> result;
    for (guint n = 0; n < prop.u.buffer.len; ++n)
    {
        result.push_back((fe_delivery_system_t) prop.u.buffer.data[n]);
    }
    return result;
}

void Frontend::tune(const TuningProperties &tuning_props)
{
    LockGuard lock(mtx_);
    int fd = open();

    struct dtv_property clear_prop = { .cmd = DTV_CLEAR };
    struct dtv_properties clear_props = { .num = 1, .props = &clear_prop };
    if (ioctl(fd, FE_SET_PROPERTY, &clear_props) < 0)
    {
        throw report_errno(FrontendError::TUNE,
                "Unable to clear tuning properties");
    }

    if (ioctl(fd, FE_SET_PROPERTY, tuning_props.get_props()) < 0)
    {
        throw report_errno(FrontendError::TUNE,
                "Unable to set tuning properties");
    }
}

Glib::Error Frontend::report_error(FrontendError code, const char *msg)
{
    char *s = g_strdup_printf("%s (%d, %d)", msg, adapter_, frontend_);
    Glib::ustring us{s};
    Glib::Error err(get_error_quark(), (int) code, us);
    g_free(s);
    return err;
}

Glib::Error Frontend::report_errno(FrontendError code, const char *msg)
{
    char *s = g_strdup_printf("%s (%d, %d): %s", msg, adapter_, frontend_,
            std::strerror(errno));
    Glib::ustring us{s};
    Glib::Error err(get_error_quark(), (int) code, us);
    g_free(s);
    return err;
}

std::string Frontend::get_device_name(const char *basename) const
{
    char *s = g_strdup_printf("/dev/dvb/adapter%d/%s%d",
            adapter_, basename, frontend_);
    std::string result{s};
    g_free(s);
    return result;
}

std::string Frontend::get_dmx_name()
{
    LockGuard lock(mtx_);

    if (!dmx_name_.length())
        dmx_name_ = get_device_name("demux");
    return dmx_name_;
}

}
