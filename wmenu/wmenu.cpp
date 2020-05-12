#include "wmenu.hpp"

#ifdef _WIN32
#include <windows.h>
#endif
#include <SFML/Graphics.hpp>

#include <iostream>
#include <sstream>
#include <algorithm>
#include <iterator>

wmenu::wmenu(const wmenu_config& config, const std::string& prompt)
	: conf(config),
	prompt(sf::String::fromUtf8(prompt.begin(), prompt.end()), font, config.font.size),
	searchbar("", font, conf.font.size)
{}

wmenu::wmenu(wmenu_config&& config, const std::string& prompt)
	: conf(std::move(config)),
	prompt(sf::String::fromUtf8(prompt.begin(), prompt.end()), font, config.font.size),
	searchbar("", font, config.font.size)
{}

void wmenu::load(std::istream& stream) {
	if (!stream) throw wmenu_exception("Error reading from stream!");
	stream >> std::noskipws;

	std::ostringstream oss;
	for (auto iter = std::istream_iterator<char>(stream); iter != std::istream_iterator<char>(); ++iter) {
		if (conf.delimiters.find(*iter) == std::string::npos) {
			oss << *iter;
		}
		else {
			std::string token = oss.str();
			if (token.size() > 0) {
				raw_items.push_back(sf::String::fromUtf8(token.begin(), token.end()));
				oss.str("");
			}
		}
	}
}

void wmenu::load(const std::vector<std::string>& items) {
	std::stringstream ss;
	for (auto& item : items)
		ss << item << conf.delimiters[0];
	return wmenu::load(ss);
}

void wmenu::init() {
	if (raw_items.size() == 0) throw wmenu_exception{ "No items passed to wmenu" };
	if (!font.loadFromFile(conf.font.path)) throw wmenu_exception{ "Error loading font!\n" };

	const float centering_offset = (conf.height - conf.font.size) / 2;

	prompt.setPosition(conf.padding, centering_offset);
	searchbar.setPosition(prompt.getGlobalBounds().left + prompt.getGlobalBounds().width, centering_offset);

	filter_items("");

	const auto window_height = conf.orientation == conf.horizontal ? conf.height : ((items.size() + 1) * conf.height);
	window.create(sf::VideoMode(sf::VideoMode::getDesktopMode().width, window_height), "wmenu", sf::Style::None);
	SetWindowPos(window.getSystemHandle(), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	window.setPosition({ 0,0 });
	window.setFramerateLimit(60);
	render();
}

void wmenu::run() {
	while (window.isOpen()) {
		if (process_event()) // only render if something is going on, no need to waste cycles
			render();
	}
}

void wmenu::print_selected() {
	SetConsoleOutputCP(CP_UTF8);
	auto str = items[selected].getString().toUtf8();
	std::cout << str.c_str();
}

// Takes a single event from the event queue in a blocking manner. Returns whether it was processed usefully 
bool wmenu::process_event() {
	auto shift_range_right = [this](size_t amount) {
		std::vector<item_type> buffer;
		buffer.reserve(amount);
		auto rpeek = filter(filtered_range.second, raw_items.cend(),
			std::back_inserter(buffer), searchbar.getString(), amount).second;
		if (!buffer.empty()) {
			std::move(buffer.begin(), buffer.end(), std::back_inserter(items));
			std::rotate(items.begin(), items.begin() + amount, items.end());
			filtered_range.first = filter(filtered_range.first, raw_items.cend(),
				buffer.begin(), searchbar.getString(), amount).second;
			filtered_range.second = rpeek;
			items.pop_back();
			position_items();
		}
	};
	auto shift_range_left = [this](size_t amount) {
		std::vector<item_type> buffer;
		buffer.reserve(amount);
		auto lpeek = filter(
			std::make_reverse_iterator(filtered_range.first),
			std::make_reverse_iterator(raw_items.cbegin()),
			std::back_inserter(buffer),
			searchbar.getString(),
			amount).second.base();
		if (!buffer.empty()) {
			std::move(buffer.begin(), buffer.end(), std::front_inserter(items));
			filtered_range.first = lpeek;
			filtered_range.second = filter(
				std::make_reverse_iterator(filtered_range.second),
				std::make_reverse_iterator(raw_items.cbegin()),
				buffer.begin(),
				searchbar.getString(),
				amount).second.base();
			items.pop_back();
			position_items();
		}
	};

	sf::Event event;
	bool processed = false;
	if (window.waitEvent(event)) {
		processed = true;
		if (event.type == sf::Event::Closed)
			window.close();
		else if (event.type == sf::Event::KeyPressed && items.size() > 0) {
			if (event.key.code == sf::Keyboard::Right || event.key.code == sf::Keyboard::Down) {
				if (selected + 1 == items.size()) shift_range_right(1);
				else selected++;
			}
			else if (event.key.code == sf::Keyboard::Left || event.key.code == sf::Keyboard::Up) {
				if (selected == 0) shift_range_left(1);
				else selected--;
			}
			else if (event.key.code == sf::Keyboard::Escape)
				window.close();
			else if (event.key.code == sf::Keyboard::Enter) {
				if (selected < items.size()) {
					window.close();
					print_selected();
				}
			}
		}
		else if (event.type == sf::Event::TextEntered) {
			sf::String query = searchbar.getString();

			if (event.text.unicode == '\b') { // backspace
				if (query.getSize() > 0)
					query.erase(query.getSize() - 1, 1);
			}
			else if (event.text.unicode != 13) { // enter key
				query += event.text.unicode;
			}

			searchbar.setString(query);
			filter_items(query);
		}
		else processed = false;
	}
	return processed;
}

template <class InputIt, class OutputIt, class OutputType>
std::pair<InputIt, InputIt> wmenu::filter(
	const InputIt begin, const InputIt end, OutputIt result, const sf::String& pattern, size_t limit)
{
	InputIt it = begin, first_in_range, last_in_range;
	size_t count = 0;
	for (; it != end && count < limit; ++it) {
		if (it->find(pattern) != sf::String::InvalidPos) {
			if constexpr (std::is_same_v<OutputType, item_type>) {
				*result++ = std::move(item_type(*it, font, conf.font.size));
			} else {
				*result++ = std::move(*it);
			}
			if (count++ == 0) {
				first_in_range = it;
			}
		}
	}
	last_in_range = it;
	return std::make_pair(first_in_range, last_in_range);
}

void wmenu::filter_items(const sf::String& pattern) {
	items.clear();
	filtered_range = filter(raw_items.cbegin(), raw_items.cend(), std::back_inserter(items), pattern, conf.limit);
	position_items();
}

void wmenu::position_items() {
	float centering_offset = (conf.height - conf.font.size) / 2;

	for (size_t i = 0; i < items.size(); i++) {
		if (conf.orientation == conf.horizontal) {
			float offset = (i == 0 ? conf.search_margin * sf::VideoMode::getDesktopMode().width : (items[i - 1].getGlobalBounds().left + items[i - 1].getGlobalBounds().width)) + conf.padding;
			items[i].setPosition(offset, centering_offset);
		}
		else if (conf.orientation == conf.vertical) {
			float offset = (i + 1) * conf.height;
			items[i].setPosition(conf.padding, offset + centering_offset);
		}
	}
}

void wmenu::render_items() {
	for (size_t i = 0; i < items.size(); i++) {
		items[i].setFillColor(i == selected ? conf.schemes.selected.fg : conf.schemes.normal.fg);
		window.draw(items[i]);
	}
}

void wmenu::render() {
	window.clear(conf.schemes.normal.bg);
	render_items();
	window.draw(prompt);
	window.draw(searchbar);
	window.display();
}