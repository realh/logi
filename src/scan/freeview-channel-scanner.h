#pragma once
/*
    logi - A DVB DVR designed for web-based clients.
    Copyright (C) 2017 Tony Houghton <h@realh.co.uk>

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

#include "nit-processor.h"
#include "standard-channel-scanner.h"

namespace logi
{

class FreeviewLCNPair
{
private:
    std::uint16_t service_id_;
    std::uint16_t lcn_;
public:
    FreeviewLCNPair(std::uint16_t sid, std::uint16_t l) :
        service_id_(sid), lcn_(l)
    {}

    std::uint16_t service_id() const { return service_id_; }

    std::uint16_t lcn() const { return lcn_ & 0x3ff; }
};

class FreeviewLCNDescriptor: public Descriptor
{
public:
    constexpr static std::uint8_t FREEVIEW_LCN_DESCRIPTOR_TAG = 0x83;

    FreeviewLCNDescriptor(const Descriptor &source) : Descriptor(source) {}

    std::vector<FreeviewLCNPair> get_lcn_pairs() const;
};

class FreeviewNITProcessor: public NITProcessor
{
public:
    constexpr static std::uint8_t LCN_DESCRIPTOR_TAG = 0x83;
protected:
    virtual void process_descriptor(const Descriptor &desc);
};

class FreeviewChannelScanner: public StandardChannelScanner
{
public:
    FreeviewChannelScanner(): StandardChannelScanner()
    {}
protected:
    virtual std::unique_ptr<NITProcessor> new_nit_processor() override;
};

}
