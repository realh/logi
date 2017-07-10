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

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <linux/dvb/dmx.h>

#include <glibmm/main.h>

#include "section-filter.h"

namespace logi_priv
{

using namespace logi;

void SectionFilterBase::start(struct dmx_sct_filter_params *params)
{
    std::shared_ptr<Frontend> fe(rcv_->get_frontend());

    fd_ = ::open(fe->get_dmx_name().c_str(), O_RDONLY | O_NONBLOCK);
    g_print("Opened section filter fd %d\n", fd_);
    if (fd_ < 0)
    {
        throw fe->report_errno(FrontendError::FILTER,
                "Unable to open section filter");
    }

    params->flags |= DMX_IMMEDIATE_START;
    if (ioctl(fd_, DMX_SET_FILTER, params) < 0)
    {
        throw fe->report_errno(FrontendError::FILTER,
                "Unable to start section filter");
    }

    rcv_->detune_signal().connect(sigc::mem_fun(*this,
                &SectionFilterBase::stop));
    io_conn_ = Glib::signal_io().connect(
            sigc::mem_fun(*this, &SectionFilterBase::io_cb),
            fd_, Glib::IO_IN | Glib::IO_ERR | Glib::IO_PRI | Glib::IO_HUP);
}

void SectionFilterBase::stop()
{
    detune_conn_.disconnect();
    if (io_conn_.connected())
    {
        io_conn_.disconnect();
    }
    if (fd_ >= 0)
    {
        g_print("Closed section filter fd %d\n", fd_);
        ::close(fd_);
        fd_ = -1;
    }
}

bool SectionFilterBase::io_cb(Glib::IOCondition cond)
{
    g_print("Section filter condition %x\n", cond);
    if (cond && (G_IO_IN | G_IO_PRI))
    {
        Section *sec = construct_section();

        if (sec->read_from_fd(fd_) < 0)
            callback(errno, nullptr);
        else
            callback(0, sec);
    }
    return true;
}

void SectionFilterBase::get_params(struct dmx_sct_filter_params &params,
        std::uint16_t pid, std::uint8_t table_id, std::uint16_t section_id,
        unsigned timeout,
        std::uint8_t table_id_mask,
        std::uint16_t section_id_mask)
{
    std::memset(&params, 0, sizeof(params));
    params.pid = pid;
    params.filter.filter[0] = table_id;
    params.filter.filter[1] = section_id >> 8;
    params.filter.filter[2] = section_id & 0xff;
    params.filter.mask[0] = table_id_mask;
    params.filter.mask[1] = section_id_mask >> 8;
    params.filter.mask[2] = section_id_mask & 0xff;
    params.timeout = timeout;
    params.flags = DMX_CHECK_CRC | DMX_IMMEDIATE_START;
    g_print("pid %x, table_id %x/%x, section_id %x/%x\n",
            pid, table_id, table_id_mask, section_id, section_id_mask);
}

}
