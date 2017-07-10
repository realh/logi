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

#include <list>

#include "udev-device.h"

namespace gudev
{

using ClientStruct = struct _GUdevClient;

}

namespace logi
{

using DeviceList = std::list<gudev::Device>;
using DeviceSignal = sigc::signal<void, gudev::Device>;

class UdevClient
{
private:
    gudev::ClientStruct *udev_client_;
    gulong uevent_tag_;
    DeviceSignal add_sig_, remove_sig_;
public:
    UdevClient();

    ~UdevClient();

    UdevClient(const UdevClient &) = delete;
    UdevClient(UdevClient &&) = delete;
    UdevClient &operator=(const UdevClient &) = delete;
    UdevClient &operator=(UdevClient &&) = delete;

    DeviceList listFrontends();

    DeviceSignal getAddSignal()
    {
        return add_sig_;
    }

    DeviceSignal getRemoveSignal()
    {
        return remove_sig_;
    }
private:
    static void handleUEvent(gudev::ClientStruct *client, const char *action,
            gudev::DeviceStruct *device, UdevClient *self);

    static char const *subsys_[];
};

}
