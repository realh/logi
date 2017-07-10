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

#include <gudev/gudev.h>

#include "udev-device.h"

namespace gudev
{

const char *Device::get_name()
{
    return g_udev_device_get_name(device_);
}

const char *Device::get_subystem()
{
    return g_udev_device_get_subsystem(device_);
}

const char *Device::get_device_file()
{
    return g_udev_device_get_device_file(device_);
}

const char *Device::get_property(const char *name)
{
    return g_udev_device_get_property(device_, name);
}

int Device::get_property_as_int(const char *name)
{
    return g_udev_device_get_property_as_int(device_, name);
}

Device Device::get_parent()
{
    Device result{g_udev_device_get_parent(device_)};
    // Device creates an extra ref so we need to remove one
    g_object_unref(result.device_);
    return result;
}

}
