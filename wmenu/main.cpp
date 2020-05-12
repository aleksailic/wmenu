/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.

	Copyright (C) 2020 Aleksa Ilic <aleksa.d.ilic@gmail.com>
*/

#include "wmenu.hpp"

#ifdef _WIN32
#include <windows.h>
#endif
#include <cxxopts.hpp>
#include <SFML/Graphics.hpp>

#include <fstream>

constexpr auto version = "0.1.0";

int main(int argc, char* argv[]) {
	cxxopts::Options options("wmenu", "wmenu is a generic menu for windows, inspired and (mostly) compatible with dmenu.");

	options.add_options()
		("h,help", "prints usage information to standard output, then exits")
		("v,version", "prints version information to standard output, then exits")
		("i,insensitive", "makes wmenu match menu entries case insensitively - not yet implemented")
		("b,bottom", "defines that wmenu appears at the bottom - not yet implemented")
		("p,prompt", "defines a prompt to be displayed before the input area. ", cxxopts::value<std::string>()->default_value(""))
		("f,file", "read from file insted of stdin/arguments", cxxopts::value<std::string>())
		("fn", "defines the font", cxxopts::value<std::string>())
		("s,size", "sets font size in points", cxxopts::value<size_t>())
		("l,limit", "limit number of itemes in menu", cxxopts::value<size_t>())
		("d,delimiters", "change list of delimiters", cxxopts::value<std::string>())
		("vertical", "sets wmenu orientation to vertical")
		("horizontal", "sets wmenu orientation to horizontal")
		("hide", "hide underlying console when calling wmenu")
		("items", "list of items", cxxopts::value<std::vector<std::string>>())
		;

	options.positional_help("<ITEM>");
	options.parse_positional({ "items" });

	int exit_code = 0;

	try {
		auto result = options.parse(argc, argv);

		if (result.count("help")) {
			std::cout << options.help() << std::endl;
			return exit_code;
		}
		if (result.count("version")) {
			std::cout << 'v' << version << std::endl;
			return exit_code;
		}

		wmenu_config config;

		if (result.count("insensitive"))
			config.insensitive = true;
		if (result.count("bottom"))
			config.position = config.bottom;
		if (result.count("fn"))
			config.font.path = result["fn"].as<std::string>();
		if (result.count("size"))
			config.font.size = result["size"].as<size_t>();
		if (result.count("limit"))
			config.limit = result["limit"].as<size_t>();
		if (result.count("delimiters"))
			config.delimiters = result["delimiters"].as<std::string>();
		if (result.count("vertical"))
			config.orientation = config.vertical;
		if (result.count("horizontal"))
			config.orientation = config.horizontal;
#ifdef _WIN32
		if (result.count("hide"))
			ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif
		
		wmenu w(std::move(config), result["prompt"].as<std::string>());

		// load items depending on the user specified method
		if (result.count("file")) {
			std::ifstream file(result["file"].as<std::string>(), std::ios_base::binary);
			w.load(file);
		} else if (result.count("items")) {
			w.load(result["items"].as<std::vector<std::string>>());
		} else {
			w.load(std::cin);
		}

		w.init();
		w.run();
	}
	catch (const std::exception& ex) {
		std::cerr << ex.what() << std::endl;
		exit_code = 1;
	}

	return exit_code;
}
