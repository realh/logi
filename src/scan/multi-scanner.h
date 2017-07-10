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

#include "receiver.h"
#include "tuning-iterator.h"

namespace logi
{

class ChannelScanner;

/**
 * MultiScanner:
 * Manages a scan of a whole set of frequencies for a network/bouqet.
 */
class MultiScanner
{
public:
    enum Status
    {
        BLANK,      /// No data has ben collected.
        PARTIAL,    /// Some data has been collected.
        COMPLETE    /// All data has been collected.
    };
private:
    std::shared_ptr<Receiver> rcv_;
    std::shared_ptr<ChannelScanner> channel_scanner_;
    std::shared_ptr<TuningIterator> iter_;
    Status status_;
    sigc::signal<void, Status> finished_signal_;
    sigc::connection lock_conn_, nolock_conn_;
    bool finished_;
public:
    MultiScanner(std::shared_ptr<Receiver> rcv,
            std::shared_ptr<ChannelScanner> channel_scanner,
            std::shared_ptr<TuningIterator> iter);

    ~MultiScanner()
    {
        cancel();
    }

    void start();

    /**
     * cancel:
     * May cause finished_signal.
     */
    void cancel();

    /**
     * finished_signal:
     * Emitted when all the channels have been scanned.
     * Takes a Status parameter.
     */
    sigc::signal<void, Status> finished_signal()
    {
        return finished_signal_;
    }

    std::shared_ptr<Receiver> get_receiver()
    {
        return rcv_;
    }

    std::shared_ptr<Frontend> get_frontend()
    {
        return rcv_->get_frontend();
    }

    friend ChannelScanner;
private:
    /**
     * Called by ChannelScanner when it's completed scanning a channel.
     */
    void channel_finished(bool success);

    void next();

    void lock_cb();

    void nolock_cb();
};

}
