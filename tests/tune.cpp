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

#include <cstdio>
#include <cstdlib>

#include "receiver.h"

using namespace logi;

using MLPtr = Glib::RefPtr<Glib::MainLoop>;

static void timeout_cb(MLPtr main_loop)
{
    g_print("Lock failed\n");
    main_loop->quit();
}

static void lock_cb(MLPtr main_loop)
{
    g_print("Locked\n");
    main_loop->quit();
}

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        fprintf(stderr, "Usage: tune ADAPTER FRONTEND 'TUNING STRING'\n");
        return 1;
    }

    try
    {
        std::shared_ptr<TuningProperties> props(new TuningProperties(argv[3]));
        std::shared_ptr<Frontend> frontend(new Frontend(std::atoi(argv[1]),
                        std::atoi(argv[2])));
        Receiver rcv(frontend, SYS_DVBS);
        MLPtr main_loop(Glib::MainLoop::create());

        rcv.lock_signal().connect(
                sigc::bind<MLPtr>(sigc::ptr_fun(lock_cb), main_loop));
        rcv.nolock_signal().connect(
                sigc::bind<MLPtr>(sigc::ptr_fun(timeout_cb), main_loop));

        rcv.tune(props, 10000);

        main_loop->run();
    }
    catch (Glib::Exception &x)
    {
        g_critical("%s", x.what().c_str());
        return 1;
    }
    return 0;
}
