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
 * Finds an available DVB-S adapter and scans for Freesat.
 */

#include "db/logi-sqlite.h"
#include "scan/freesat-channel-scanner.h"
#include "scan/freesat-lcn-processor.h"
#include "scan/freesat-tuning-iterator.h"
#include "scan/multi-scanner.h"
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

static const char *bouquet_name;
static const char *region_name;

static bool check_bouquet_name(const char *nm,
        const std::vector<std::tuple<std::uint32_t, Glib::ustring>> &bqs)
{
    for (const auto &bq: bqs)
    {
        if (std::get<1>(bq) == nm)
        {
            return true;
        }
    }
    return false;
}

static bool check_region_name(const char *nm,
        const std::vector<std::tuple<std::uint32_t, Glib::ustring>> &regs)
{
    for (const auto &reg: regs)
    {
        if (std::get<1>(reg) == nm)
        {
            return true;
        }
    }
    return false;
}

static void lcn_fn()
{
    g_print("Committed SI data to database\n");
    auto nq = database->get_all_network_ids_query("Freesat");
    auto bqs = database->run_query(nq);
    if (!bqs.size())
    {
        g_critical("No bouquets found");
        return;
    }
    if (bouquet_name)
    {
        if (!check_bouquet_name(bouquet_name, bqs))
        {
            g_critical("Bouquet '%s' not found, using 'England HD'",
                    bouquet_name);
            bouquet_name = nullptr;
        }
    }
    if (!bouquet_name)
    {
        bouquet_name = "England HD";
        if (!check_bouquet_name(bouquet_name, bqs))
        {
            bouquet_name = std::get<1>(bqs[0]).c_str();
            g_critical("Bouquet 'England HD' not found, using '%s'",
                    bouquet_name);
        }
    }

    auto rq = database->get_regions_for_bouquet_query("Freesat");
    auto regs = database->run_query(rq, {std::get<0>(bqs[0])});
    if (region_name)
    {
        if (!check_region_name(region_name, regs))
        {
            g_critical("Region '%s' not found in bouquet '%s', "
                    "using 'South/Meridian S'",
                    region_name, bouquet_name);
            region_name = nullptr;
        }
    }
    if (!region_name)
    {
        region_name = "South/Meridian S";
        if (!check_region_name(region_name, regs))
        {
            region_name = std::get<1>(regs[0]).c_str();
            g_critical("Region 'South/Meridian S' not found in bouquet '%s', "
                    "using '%s'", bouquet_name, region_name);
        }
    }

    g_print("Processing LCNs for bouquet '%s', region '%s'\n",
            bouquet_name, region_name);
    FreesatLCNProcessor lp(*database);
    lp.process("Freesat", bouquet_name, region_name);
}

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

    if (argc > 2)
    {
        bouquet_name = argv[1];
        region_name = argv[2];
    }
    else
    {
        bouquet_name = nullptr;
        region_name = nullptr;
    }

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
