/*
 *  nextpnr -- Next Generation Place and Route
 *
 *  Copyright (C) 2018  Clifford Wolf <clifford@symbioticeda.com>
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#ifdef MAIN_EXECUTABLE

#ifndef NO_GUI
#include <QApplication>
#include "application.h"
#include "mainwindow.h"
#endif
#ifndef NO_PYTHON
#include "pybindings.h"
#endif
#include <boost/filesystem/convenience.hpp>
#include <boost/program_options.hpp>
#include <fstream>
#include <iostream>
#include "log.h"
#include "nextpnr.h"
#include "version.h"

#include "design_utils.h"
#include "jsonparse.h"
#include "pack.h"
#include "place_sa.h"
#include "route.h"
#include "timing.h"

USING_NEXTPNR_NAMESPACE

int main(int argc, char *argv[])
{
    try {

        namespace po = boost::program_options;
        int rc = 0;

        log_files.push_back(stdout);

        po::options_description options("Allowed options");
        options.add_options()("help,h", "show help");
        options.add_options()("verbose,v", "verbose output");
        options.add_options()("force,f", "keep running after errors");
#ifndef NO_GUI
        options.add_options()("gui", "start gui");
#endif
        options.add_options()("json", po::value<std::string>(), "JSON design file to ingest");
        options.add_options()("seed", po::value<int>(), "seed value for random number generator");

        po::positional_options_description pos;
#ifndef NO_PYTHON
        options.add_options()("run", po::value<std::vector<std::string>>(), "python file to execute");
        pos.add("run", -1);
#endif
        options.add_options()("version,V", "show version");

        po::variables_map vm;
        try {
            po::parsed_options parsed = po::command_line_parser(argc, argv).options(options).positional(pos).run();

            po::store(parsed, vm);

            po::notify(vm);
        }

        catch (std::exception &e) {
            std::cout << e.what() << "\n";
            return 1;
        }

        if (vm.count("help") || argc == 1) {
            std::cout << boost::filesystem::basename(argv[0])
                      << " -- Next Generation Place and Route (git "
                         "sha1 " GIT_COMMIT_HASH_STR ")\n";
            std::cout << "\n";
            std::cout << options << "\n";
            return argc != 1;
        }

        if (vm.count("version")) {
            std::cout << boost::filesystem::basename(argv[0])
                      << " -- Next Generation Place and Route (git "
                         "sha1 " GIT_COMMIT_HASH_STR ")\n";
            return 1;
        }
        ArchArgs args;
        args.type = ArchArgs::LFE5U_25F;
        args.package = "CABGA381";
        args.speed = 6;
        Context ctx(args);

        if (vm.count("verbose")) {
            ctx.verbose = true;
        }

        if (vm.count("force")) {
            ctx.force = true;
        }

        if (vm.count("seed")) {
            ctx.rngseed(vm["seed"].as<int>());
        }

        if (vm.count("json")) {
            std::string filename = vm["json"].as<std::string>();
            std::ifstream f(filename);
            if (!parse_json_file(f, filename, &ctx))
                log_error("Loading design failed.\n");

            if (!pack_design(&ctx) && !ctx.force)
                log_error("Packing design failed.\n");
            if (vm.count("freq"))
                ctx.target_freq = vm["freq"].as<double>() * 1e6;
            assign_budget(&ctx);
            ctx.check();
            print_utilisation(&ctx);
            ctx.timing_driven = true;
            if (vm.count("no-tmdriv"))
                ctx.timing_driven = false;

            if (!place_design_sa(&ctx) && !ctx.force)
                log_error("Placing design failed.\n");
            ctx.check();
            if (!route_design(&ctx) && !ctx.force)
                log_error("Routing design failed.\n");
        }

#ifndef NO_PYTHON
        if (vm.count("run")) {
            init_python(argv[0], true);
            python_export_global("ctx", ctx);

            std::vector<std::string> files = vm["run"].as<std::vector<std::string>>();
            for (auto filename : files)
                execute_python_file(filename.c_str());

            deinit_python();
        }
#endif

#ifndef NO_GUI
        if (vm.count("gui")) {
            Application a(argc, argv);
            MainWindow w;
            w.show();

            rc = a.exec();
        }
#endif
        return rc;
    } catch (log_execution_error_exception) {
#if defined(_MSC_VER)
        _exit(EXIT_FAILURE);
#else
        _Exit(EXIT_FAILURE);
#endif
    }
}

#endif
