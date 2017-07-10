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

#include <memory>

#include <glibmm/main.h>

#include "frontend.h"

namespace logi
{

/**
 * Receiver:
 * A Receiver is associated with a #Frontend, but decoupled to allow for
 * frontends which support multiple types and therefore, presumably, multiple
 * tuners which can be used simultaneously. But I don't know how such frontends
 * works in practice.
 */
class Receiver
{
private:
    std::shared_ptr<Frontend> frontend_;
    std::shared_ptr<TuningProperties> tuned_to_;
    sigc::connection lock_conn_, timeout_conn_;
    fe_delivery_system_t sd_delsys_, hd_delsys_;
    sigc::signal<void> lock_signal_, nolock_signal_, detune_signal_;
public:
    /**
     * Receiver:
     * @sd_delsys:  The primary/SD delivery system. The frontend is queried to
     *              see if it can also receive HD.
     */
    Receiver(std::shared_ptr<Frontend> frontend,
            fe_delivery_system_t sd_delsys);

    Receiver(const Receiver &) = delete;
    Receiver(Receiver &&) = delete;
    Receiver &operator=(const Receiver &) = delete;
    Receiver &operator=(Receiver &&) = delete;

    ~Receiver()
    {
        cancel();
    }

    bool is_tuned() const
    {
        return tuned_to_ && !lock_conn_.connected();
    }

    bool is_hd_capable() const
    {
        return hd_delsys_ != SYS_UNDEFINED;
    }

    /**
     * tune:
     * Tunes the receiver. If successful a ::lock signal is raised, otherwise
     * ::nolock is raised after the timeout. ::lock is also raised if the
     * receiver is already tuned to this frequency, and ::nolock is also raised
     * if there is an immediate error; in this case the signal is followed by an
     * exception.
     * A ::detune signal is raised if the receiver was already tuned to a
     * different channel.
     * @timeout: In milliseconds.
     */
    void tune(std::shared_ptr<TuningProperties> tuning_props, guint timeout);

    /**
     * cancel:
     * Cancels any tuning operation etc. Does not cause a ::detune signal.
     */
    void cancel();

    std::shared_ptr<Frontend> get_frontend()
    {
        return frontend_;
    }

    sigc::signal<void> lock_signal()
    {
        return lock_signal_;
    }

    sigc::signal<void> nolock_signal()
    {
        return nolock_signal_;
    }
    
    sigc::signal<void> detune_signal()
    {
        return detune_signal_;
    }
private:

    bool lock_cb(Glib::IOCondition cond);

    bool timeout_cb();
};

}
