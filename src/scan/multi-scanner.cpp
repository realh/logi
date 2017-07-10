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

#include "channel-scanner.h"
#include "multi-scanner.h"

namespace logi
{

MultiScanner::MultiScanner(std::shared_ptr<Receiver> rcv,
        std::shared_ptr<ChannelScanner> channel_scanner,
        std::shared_ptr<TuningIterator> iter)
    : rcv_{rcv}, channel_scanner_{channel_scanner}, iter_{iter},
    status_{BLANK}, finished_{false}
{}

void MultiScanner::start()
{
    lock_conn_ = rcv_->lock_signal().connect(
            sigc::mem_fun(*this, &MultiScanner::lock_cb));
    nolock_conn_ = rcv_->nolock_signal().connect(
            sigc::mem_fun(*this, &MultiScanner::nolock_cb));
    next();
}

void MultiScanner::cancel()
{
    bool finished = finished_;

    finished_ = true;

    lock_conn_.disconnect();
    nolock_conn_.disconnect();

    channel_scanner_->cancel();

    if (channel_scanner_->is_complete())
        status_ = COMPLETE;

    if (!finished)
        finished_signal_.emit(status_);
}

void MultiScanner::channel_finished(bool success)
{
    bool complete = channel_scanner_->is_complete();

    if (complete)
    {
        status_ = COMPLETE;
        cancel();
    }
    else if (success)
    {
        status_ = PARTIAL;
    }
    
    if (!complete)
        next();
}

void MultiScanner::next()
{
    while (true)
    {
        auto props = iter_->next();
        if (!props)
        {
            cancel();
            return;
        }

        g_print("Tuning to %s... ", props->describe().c_str());
        try
        {
            rcv_->tune(props, 5000);
            break;
        }
        catch (Glib::Exception &x)
        {
            g_log(nullptr, G_LOG_LEVEL_CRITICAL, "%s", x.what().c_str());
        }
    }
}

void MultiScanner::lock_cb()
{
    g_print("Locked\n");
    channel_scanner_->start(this);
}

void MultiScanner::nolock_cb()
{
    g_print("No lock\n");
    next();
}

}
