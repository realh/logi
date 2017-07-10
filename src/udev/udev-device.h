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
#include <string>

#include <glibmm.h>

namespace gudev
{

using DeviceStruct = struct _GUdevDevice;

class Device
{
private:
    DeviceStruct *device_;
public:
    Device(DeviceStruct *dev) : device_{ dev }
    {
        g_object_ref(dev);
    }

    ~Device()
    {
        g_clear_object(&device_);
    }

    Device(const Device &other) : Device(other.device_)
    {}

    Device(Device &&other) : device_{other.device_}
    {
        other.device_ = nullptr;
    }

    Device &operator=(const Device &other)
    {
        g_object_unref(device_);
        g_object_ref(device_ = other.device_);
        return *this;
    }

    Device &operator=(Device &&other)
    {
        device_ = other.device_;
        other.device_ = nullptr;
        return *this;
    }

    const char *get_name();

    const char *get_subystem();

    const char *get_device_file();

    const char *get_property(const char *name);

    const char *get_property(const std::string &name)
    {
        return get_property(name.c_str());
    }

    int get_property_as_int(const char *name);

    int get_property_as_int(const std::string &name)
    {
        return get_property_as_int(name.c_str());
    }

    Device get_parent();
};

}
