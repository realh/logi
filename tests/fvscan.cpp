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
#include "scan/dvbt-tuning-iterator.h"
#include "scan/multi-scanner.h"
#include "scan/freeview-channel-scanner.h"
#include "scan/freeview-lcn-processor.h"
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
                if (ds == SYS_DVBT)
                {
                    g_print("DVB-T\n");
                    return std::shared_ptr<Receiver> 
                            { new Receiver(frontend, SYS_DVBT) };
                }
            }
            g_print("Not DVB-T\n");
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

static const char *network_name;

static void lcn_fn()
{
    g_print("Committed SI data to database\n");
    auto nq = database->get_all_network_ids_query("Freeview");
    auto nws = database->run_query(nq);
    if (!nws.size())
    {
        g_critical("No networks found");
        return;
    }
    std::string nn;
    if (network_name)
    {
        nn = network_name;
        for (const auto &nw: nws)
        {
            if (std::get<1>(nw) == nn)
            {
                network_name = nullptr;
                break;
            }
        }
        if (network_name)
        {
            nn = std::get<1>(nws[0]);
            g_critical("Network '%s' not found, using '%s'",
                    network_name, nn.c_str());
        }
    }
    else
    {
        nn = std::get<1>(nws[0]);
    }
    g_print("Processing LCNs for network '%s'\n", nn.c_str());
            
    FreeviewLCNProcessor lp(*database);
    lp.process("Freeview", nn, "");
}

static void finished_cb(MultiScanner &scanner, MultiScanner::Status status)
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
        scanner.commit_to_database(*database, "Freeview");
        database->queue_function(lcn_fn);
        database->queue_callback([]()
        {
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

int main(int argc, char **argv)
{
    std::shared_ptr<Receiver> rcv { get_receiver() };
    
    if (!rcv)
        return 1;

    if (argc > 1)
        network_name = argv[1];
    else
        network_name = nullptr;

    database = std::make_shared<Sqlite3Database>();
    database->start();
    database->ensure_tables("Freeview");
    auto vp = std::make_shared<std::vector<std::tuple<Glib::ustring>>>();
    vp->emplace_back("Freeview");
    database->queue_statement(database->get_insert_source_statement(), vp);

    MultiScanner scanner { rcv,
        std::shared_ptr<SingleChannelScanner> { new FreeviewChannelScanner() },
        std::shared_ptr<DvbtTuningIterator> { new DvbtTuningIterator() } };

    main_loop = Glib::MainLoop::create();

    scanner.finished_signal().connect(sigc::ptr_fun(finished_cb));
    scanner.start();

    // An immediate fail call of finished_cb deletes main_loop
    if (main_loop)
        main_loop->run();

    return 0;
}
