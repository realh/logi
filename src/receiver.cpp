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

#include <cstring>
#include <cerrno>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "receiver.h"

namespace logi
{

Receiver::Receiver(std::shared_ptr<Frontend> frontend,
            fe_delivery_system_t sd_delsys) :
    frontend_{frontend}, sd_delsys_{sd_delsys}, hd_delsys_{SYS_UNDEFINED}
{
    auto delsyss = frontend->enum_delivery_systems();
    for (auto d: delsyss)
    {
        if (d == SYS_DVBS2 && sd_delsys == SYS_DVBS)
        {
            hd_delsys_ = SYS_DVBS2;
            break;
        }
        else if (d == SYS_DVBT2 && sd_delsys == SYS_DVBT)
        {
            hd_delsys_ = SYS_DVBT2;
            break;
        }
    }
}

void Receiver::tune(std::shared_ptr<TuningProperties> tuning_props,
        guint timeout)
{
    lock_conn_.disconnect();
    timeout_conn_.disconnect();

    try
    {
        int fd = frontend_->open();

        tuned_to_ = tuning_props;
        lock_conn_ = Glib::signal_io().connect(
                sigc::mem_fun(*this, &Receiver::lock_cb),
                fd, Glib::IO_IN | Glib::IO_ERR | Glib::IO_PRI | Glib::IO_HUP);
        timeout_conn_ = Glib::signal_timeout().connect(
                sigc::mem_fun(*this, &Receiver::timeout_cb), timeout);
        frontend_->tune(*tuned_to_);
    }
    catch (...)
    {
        tuned_to_.reset();
        nolock_signal_.emit();
        throw;
    }
}

void Receiver::cancel()
{
    if (lock_conn_.connected())
    {
        tuned_to_.reset();
        lock_conn_.disconnect();
    }
    timeout_conn_.disconnect();
}

bool Receiver::lock_cb(Glib::IOCondition cond)
{
    if (cond && (G_IO_IN | G_IO_PRI))
    {
        try
        {
            struct dvb_frontend_event e;

            errno = 0;
            if (ioctl(frontend_->open(), FE_GET_EVENT, &e) == 0)
            {
                if (e.status & FE_HAS_LOCK)
                {
                    lock_conn_.disconnect();
                    timeout_conn_.disconnect();
                    lock_signal_.emit();
                    return false;
                }
                else
                {
                    return true;
                }
            }
            // We may get harmless overflow errors here
            if (errno == EOVERFLOW)
            {
                return true;
            }
            g_log(nullptr, G_LOG_LEVEL_CRITICAL,
                    "%s waiting for lock", std::strerror(errno));
        }
        catch (Glib::Exception &x)
        {
            g_log(nullptr, G_LOG_LEVEL_CRITICAL,
                    "Waiting for lock: %s", x.what().c_str());
        }
    }
    else
    {
        g_log(nullptr, G_LOG_LEVEL_CRITICAL,
                "Error/HUP condition waiting for lock");
    }
    lock_conn_.disconnect();
    timeout_conn_.disconnect();
    nolock_signal_.emit();
    return false;
}

bool Receiver::timeout_cb()
{
    lock_conn_.disconnect();
    timeout_conn_.disconnect();
    nolock_signal_.emit();
    return false;
}

}
