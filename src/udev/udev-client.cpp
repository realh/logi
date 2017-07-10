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

#include <gudev/gudev.h>

#include "udev-client.h"

namespace logi
{

char const *UdevClient::subsys_[] = { "dvb", nullptr };

UdevClient::UdevClient() :
    udev_client_ { g_udev_client_new(subsys_) },
    uevent_tag_ { g_signal_connect(udev_client_, "uevent",
            G_CALLBACK(handleUEvent), this) }
{
    g_object_ref(udev_client_);
}

UdevClient::~UdevClient()
{
    g_signal_handler_disconnect(udev_client_, uevent_tag_);
    g_object_unref(udev_client_);
}

DeviceList UdevClient::listFrontends()
{
    GUdevEnumerator *enumerator = g_udev_enumerator_new(udev_client_);

    g_udev_enumerator_add_match_subsystem(enumerator, "dvb");
    g_udev_enumerator_add_match_property(enumerator,
            "DVB_DEVICE_TYPE", "frontend");

    GList *frontends = g_udev_enumerator_execute(enumerator);
    DeviceList result;

    for (GList *link = g_list_first(frontends); link; link = g_list_next(link))
    {
        result.push_back((gudev::DeviceStruct *) link->data);
        // Creating a gudev::Device adds a reference to the DeviceStruct so
        // we need to remove the one that gudev created
        g_object_unref(link->data);
    }

    g_object_unref(enumerator);

    return result;
}

void UdevClient::handleUEvent(gudev::ClientStruct *, const char *action,
        gudev::DeviceStruct *device, UdevClient *self)
{
    if (!std::strstr(g_udev_device_get_name(device), "frontend"))
        return;
    if (!std::strcmp(action, "add"))
        self->add_sig_.emit(device);
    else if (!std::strcmp(action, "remove"))
        self->remove_sig_.emit(device);
}

}
