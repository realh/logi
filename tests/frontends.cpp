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

// This test finds DVB frontends with udev and prints information about them.

#include "frontend.h"
#include "udev/udev-client.h"

void describe_device(gudev::Device &device)
{
    g_print("Name:\t\t%s\n", device.get_name());
    g_print("File:\t\t%s\n", device.get_device_file());
    gudev::Device parent = device.get_parent();
    g_print("Parent model:\t%s\n",
            parent.get_property("ID_MODEL_FROM_DATABASE"));
}

int main()
{
    logi::UdevClient client;
    auto devs = client.listFrontends();
    for (auto &dev: devs)
    {
        describe_device(dev);

        try
        {
            logi::Frontend fe(dev.get_property_as_int("DVB_ADAPTER_NUM"),
                    dev.get_property_as_int("DVB_FRONTEND_NUM"));
            auto delsys = fe.enum_delivery_systems();
            g_print("Delivery systems: ");
            for (auto d: delsys)
                g_print("%d ", (int) d);
            g_print("\n");
        }
        catch (Glib::Exception &x)
        {
            g_print("Exception: %s\n\n", x.what().c_str());
        }
    }
    return 0;
}
