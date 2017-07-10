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

#include <cerrno>
#include <memory>

#include "receiver.h"
#include "si/section.h"

namespace logi_priv
{

using namespace logi;

class SectionFilterBase
{
private:
    std::shared_ptr<Receiver> rcv_;
    int fd_;
    sigc::connection detune_conn_;
    sigc::connection io_conn_;
protected:
    SectionFilterBase(std::shared_ptr<Receiver> rcv) :
        rcv_(rcv), fd_(-1)
    {}

    SectionFilterBase(std::shared_ptr<Receiver> rcv,
            struct dmx_sct_filter_params *params) :
        SectionFilterBase(rcv)
    {
        start(params);
    }

    SectionFilterBase(std::shared_ptr<Receiver> rcv,
            std::uint16_t pid, std::uint8_t table_id, std::uint16_t section_id,
            unsigned timeout = 5000,
            std::uint8_t table_id_mask = 0xff,
            std::uint16_t section_id_mask = 0xffff) :
        SectionFilterBase(rcv)
    {
        struct dmx_sct_filter_params params;
        get_params(params, pid, table_id, section_id,
                timeout, table_id_mask, section_id_mask);
        start(&params);
    }

    virtual Section *construct_section() = 0;

    virtual void callback(int reason, Section *section) = 0;
public:
    virtual ~SectionFilterBase()
    {
        stop();
    }

    /**
     * stop:
     * Can be called more than once. Does not cause a callback (it can't
     * because callback() is virtual and stop() is called from destructor).
     */
    void stop();
private:
    void start(struct dmx_sct_filter_params *params);

    bool io_cb(Glib::IOCondition cond);

    static void get_params(struct dmx_sct_filter_params &params,
            std::uint16_t pid, std::uint8_t table_id, std::uint16_t section_id,
            unsigned timeout = 5000,
            std::uint8_t table_id_mask = 0xff,
            std::uint16_t section_id_mask = 0xffff);
};

}

namespace logi
{

/**
 * SectionFilter:
 * S is the section class, T is the class of the object handling the sections.
 */
template<class S, class T> class SectionFilter :
    public logi_priv::SectionFilterBase
{
private:
    /**
     * Method:
     * This type of method handles received sections. reason is 0 or an errno.
     * If reason is 0 and section is null it means the filter has been stopped.
     */
    using Method = void (T::*)(int reason, std::shared_ptr<S> section);
    T &handler_;
    Method method_;
    std::shared_ptr<S> current_section_;
public:
    SectionFilter(std::shared_ptr<Receiver> rcv,
            struct dmx_sct_filter_params *params,
            T &handler, Method method) :
        logi_priv::SectionFilterBase(rcv, params),
        handler_{handler}, method_{method}
    {}

    SectionFilter(std::shared_ptr<Receiver> rcv,
            T &handler, Method method,
            std::uint16_t pid, std::uint8_t table_id, std::uint16_t section_id,
            unsigned timeout = 5000,
            std::uint8_t table_id_mask = 0xff,
            std::uint16_t section_id_mask = 0xffff) :
        logi_priv::SectionFilterBase(rcv, pid, table_id, section_id, timeout,
                table_id_mask, section_id_mask),
        handler_{handler}, method_{method}
    {}

    Section *construct_section() override
    {
        current_section_.reset(new S());
        return current_section_.get();
    }

    void callback(int reason, Section *section) override
    {
        if (!section)
            current_section_.reset();
        (handler_.*(method_))(reason, current_section_);
    }
};

}
