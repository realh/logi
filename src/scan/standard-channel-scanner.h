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

#include <map>
#include <memory>

#include "channel-scanner.h"
#include "nit-processor.h"
#include "section-filter.h"

namespace logi
{

/**
 * StandardChannelScanner:
 * Scans NIT and SDT.
 */
class StandardChannelScanner : public ChannelScanner
{
private:
    std::unique_ptr<SectionFilter<NITSection, StandardChannelScanner> >
        nit_filter_;
    bool nit_error_;

    struct NetworkData
    {
        std::unique_ptr<NITProcessor> nit_proc;
        bool nit_complete;
        NetworkData(std::unique_ptr<NITProcessor> &&np) :
            nit_proc(std::move(np)), nit_complete(false)
        {}
    };
    // In practice we're unlikely to see more than a couple of networks, so
    // a map is a horrible waste of CPU cycles etc, but it is scalable and 
    // makes my code much simpler. Or it would if std::pair wasn't incompatible
    // with itself, making large parts of std::map unusable.
    using NwMap = std::map<std::uint16_t, std::unique_ptr<NetworkData> >;
    using NwPair = std::pair<std::uint16_t, std::unique_ptr<NetworkData> >;
    using ConstNwPair = const std::pair<const std::uint16_t,
          std::unique_ptr<NetworkData> >;
    NwMap networks_;
public:
    StandardChannelScanner() : ChannelScanner(), nit_error_{false}
    {}

    void start(MultiScanner *multi_scanner) override;

    /**
     * @cancel:
     * Cancel the current scan, either to interrupt it, or when current channel
     * is finished with.
     */
    void cancel() override;

    /**
     * Returns: Whether a complete data set has been acquired.
     */
    bool is_complete() const override;
protected:
    std::unique_ptr<NITProcessor> new_nit_processor();
private:
    void nit_filter_cb(int reason, std::shared_ptr<NITSection> section);

    bool all_complete_or_error() const;

    bool any_complete() const;
};

}
