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

/*
 * Finds an available DVB-T adapter and scans for Freeview.
 */

#include "db/logi-sqlite.h"
#include "scan/freesat-tuning-iterator.h"
#include "scan/multi-scanner.h"
#include "scan/freesat-channel-scanner.h"
#include "udev/udev-client.h"

using namespace logi;

std::shared_ptr<Receiver> get_receiver()
{
    UdevClient udev;
    auto frontends { udev.listFrontends() };

    for (auto &dev: frontends)
    {
        auto ad_n { dev.get_property_as_int("DVB_ADAPTER_NUM") };
        auto fe_n { dev.get_property_as_int("DVB_FRONTEND_NUM") };
        auto parent { dev.get_parent() };

        g_print("(%d, %d) : %s : ", ad_n, fe_n,
                parent.get_property("ID_MODEL_FROM_DATABASE"));
        try
        {
            std::shared_ptr<Frontend> frontend { new Frontend(ad_n, fe_n) };
            auto delsyss { frontend->enum_delivery_systems() };

            for (auto ds: delsyss)
            {
                if (ds == SYS_DVBS)
                {
                    g_print("DVB-S\n");
                    return std::shared_ptr<Receiver> 
                            { new Receiver(frontend, SYS_DVBT) };
                }
            }
            g_print("Not DVB-S\n");
        }
        catch (Glib::Exception &x)
        {
            g_print("%s\n", x.what().c_str());
        }
    }

    return std::shared_ptr<Receiver> { nullptr };
}

static Glib::RefPtr<Glib::MainLoop> main_loop;

static std::shared_ptr<Sqlite3Database> database;

void finished_cb(MultiScanner &scanner, MultiScanner::Status status)
{
    const char *s;

    switch (status)
    {
        case MultiScanner::BLANK:
            s = "no data collected";
            break;
        case MultiScanner::PARTIAL:
            s = "partial data collected";
            break;
        case MultiScanner::COMPLETE:
            s = "complete data collected";
            break;
    }
    g_print("Scan finished:- %s\n", s);

    if (status == MultiScanner::COMPLETE || status == MultiScanner::PARTIAL)
    {
        // Use shared_ptr to keep db alive in its own thread
        // when we exit this scope. Scanner is kept alive in main().
        scanner.commit_to_database(*database, "Freesat");
        database->queue_callback([]()
        {
            g_print("Committed all data to database\n");
            database.reset();
            main_loop->quit();
        });
    }
    else
    {
        // If main_loop isn't running it means the scan failed
        // as soon as it started
        if (main_loop->is_running())
            main_loop->quit();
        else
            main_loop.reset();
    }
}

int main()
{
    std::shared_ptr<Receiver> rcv { get_receiver() };
    
    if (!rcv)
        return 1;

    database = std::make_shared<Sqlite3Database>();
    database->start();
    database->ensure_tables("Freesat");
    auto vp = std::make_shared<std::vector<std::tuple<Glib::ustring>>>();
    vp->emplace_back("Freesat");
    database->queue_statement(database->get_insert_source_statement(), vp);

    MultiScanner scanner { rcv,
        std::shared_ptr<SingleChannelScanner> { new FreesatChannelScanner() },
        std::shared_ptr<FreesatTuningIterator>
            { new FreesatTuningIterator() } };

    main_loop = Glib::MainLoop::create();

    scanner.finished_signal().connect(sigc::ptr_fun(finished_cb));
    scanner.start();

    // An immediate fail call of finished_cb deletes main_loop
    if (main_loop)
        main_loop->run();

    return 0;
}
