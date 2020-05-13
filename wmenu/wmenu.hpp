/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.

	Copyright (C) 2020 Aleksa Ilic <aleksa.d.ilic@gmail.com>
*/

#ifndef WMENU_HPP
#define WMENU_HPP

#include "wmenu_config.hpp"

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

#include <iostream>
#include <vector>
#include <exception>
#include <deque>

class wmenu {
public:
	wmenu(const wmenu_config& config, const std::string& prompt = "");
	wmenu(wmenu_config&& config, const std::string& prompt = "");

	void load(std::istream& stream);
	void load(const std::vector<std::string>& items);

	void init();
	void run();
private:
	template <class InputIterator>
	using range = std::pair<InputIterator, InputIterator>;
	using item_type = sf::Text;
	using raw_item_type = sf::String;
	
	void filter_items(const sf::String& pattern);
	void position_items();
	void render_items();

	void print_selected();

	void render();
	bool process_event();

	wmenu_config conf;
	size_t selected = 0;

	sf::Font font;
	sf::RenderWindow window;

	std::vector<sf::String>	raw_items;
	std::deque<item_type> items;
	sf::Text searchbar;
	sf::Text prompt;

	range<decltype(raw_items)::const_iterator> filtered_range = { raw_items.cbegin(), raw_items.cend() };
};

struct wmenu_exception : public std::exception {
	using std::exception::exception;
};

#endif // !WMENU_HPP
