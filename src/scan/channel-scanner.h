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

#include "frontend.h"
#include "si/nit-section.h"
#include "si/sdt-section.h"
#include "section-filter.h"

namespace logi
{

class MultiScanner;

/**
 * ChannelScanner:
 * Interface of a channel scanner. It scans each channel in turn, but retains
 * state between channels to build up a databse of the entire network/bouquet.
 */
class ChannelScanner
{
protected:
    template<class T>
    using NITFilterPtr = std::unique_ptr<SectionFilter<NITSection, T>>;
    template<class T>
    using SDTFilterPtr = std::unique_ptr<SectionFilter<SDTSection, T>>;

    MultiScanner *multi_scanner_;
    std::shared_ptr<Frontend> frontend_;
public:
    ChannelScanner()
    {}

    ChannelScanner(const ChannelScanner &) = delete;
    ChannelScanner(ChannelScanner &&) = delete;
    ChannelScanner &&operator=(const ChannelScanner &) = delete;
    ChannelScanner &&operator=(ChannelScanner &&) = delete;

    virtual ~ChannelScanner()
    {}

    /**
     * @start:
     * Start scanning the current channel. The frontend will already be tuned
     * by the multi-scanner.
     */
    virtual void start(MultiScanner *multi_scanner) = 0;

    /**
     * @cancel:
     * Cancel the current scan, either to interrupt it, or when current channel
     * is finished with. Does not emit a signal.
     */
    virtual void cancel() = 0;

    /**
     * Returns: Whether a complete data set has been acquired.
     */
    virtual bool is_complete() const;
protected:
    /**
     * finished:
     * Called to notify the MultiScanner that this has finished scanning the
     * current channel, or that the section filters timed out.
     */
    virtual void finished(bool success = true);

    bool successful_ = false;
};

}
