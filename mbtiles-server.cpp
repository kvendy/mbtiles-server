#include <iostream>
#include <vector>
#include <cstdlib>
#include <exception>

#include <boost/program_options.hpp>
#include <boost/shared_ptr.hpp>

namespace po = boost::program_options;

#include "crow.h"
#include "mbtilereader.h"

const std::string VERSION = "1.0.0";

// MAIN
int main(int argc, char* argv[])
{
	std::cout << "MBTILES - SERVER v" << VERSION << std::endl;

	int port = 8080;
	std::vector<std::string> sources;
	std::vector<boost::shared_ptr<MBTileReader>> readers;

	po::options_description generalOptions{ "General" };
	generalOptions.add_options()
		("help,h", "Help screen")
		("config", po::value<std::string>(), "Config file");

	po::options_description fileOptions{ "File" };
	fileOptions.add_options()
		("port,p", po::value<int>(), "Port")
		("source,s", po::value<std::vector<std::string> >(), "Input .mbtiles file");

	po::variables_map vm;
	try
	{
		po::parsed_options parsed = po::command_line_parser(argc, argv).options(generalOptions).allow_unregistered().run();
		po::store(parsed, vm);
		po::notify(vm);

		if (vm.count("help"))
		{
			std::cout << generalOptions << '\n';
			return 0;
		}
		else if (vm.count("config"))
		{
			std::ifstream ifs{ vm["config"].as<std::string>().c_str() };
			if (ifs)
				store(parse_config_file(ifs, fileOptions), vm);
		}
		else
		{
			std::cout << "No config file" << '\n';
			return 0;
		}
		notify(vm);

		if (vm.count("port"))
			port = vm["port"].as<int>();
		if (vm.count("source"))
			sources = vm["source"].as<std::vector<std::string> >();
	}
	catch (std::exception& ex)
	{
		std::cout << ex.what() << std::endl;
		return 0;
	}

	// init crow http server
	crow::SimpleApp app;

	// init mbtiles readers
	for (auto it : sources)
	{
		std::cout << "Loading file: " << it << std::endl;
		try
		{
			readers.push_back(boost::shared_ptr<MBTileReader>(new MBTileReader(it.c_str())));
		}
		catch (std::exception &e)
		{
			std::cout << "Loading failed: " << e.what() << std::endl;
		}
	}

	// declaration of routes
	CROW_ROUTE(app, "/")
	([]()
	{
		return "=== MBTILES-SERVER ===\nUsage: GET /{z}/{x}/{y}.{format}";
	});

	CROW_ROUTE(app, "/<int>/<int>/<int>.<str>")
	([&readers](int zoom, int col, int row, std::string format)
	{
		std::string blob;
		std::string readerFormat;
			
		for (auto reader : readers)
		{
			
			blob.clear();
			readerFormat = reader->GetMetadata("format");

			if (readerFormat == format)
			{
				// get the tile
				if (reader->GetTile(zoom, col, row, blob))
				{
					// prepare response
					auto res = crow::response(blob);
					if (format == "pbf")
					{
						res.set_header("Content-Type", "application/octet-stream");
						res.set_header("Content-Encoding", "gzip");
					}
					else if (format == "jpg")
					{
						res.set_header("Content-Type", "image/jpeg");
					}
					else if (format == "png")
					{
						res.set_header("Content-Type", "image/png");
					}
					else if (format == "webp")
					{
						res.set_header("Content-Type", "image/webp");
					}
					res.set_header("Access-Control-Allow-Origin", "*");
					res.set_header("Cache-Control", "public");
					return res;
				}
			}
		}
		
		auto res = crow::response(500, "tile not found");
		res.set_header("Access-Control-Allow-Origin", "*");
		return res;
	});

	crow::logger::setLogLevel(crow::LogLevel::Critical);

	std::cout << "API listening on port " + std::to_string(port) + "..." << std::endl;
	app.port(port).multithreaded().run();
}
