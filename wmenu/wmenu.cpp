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

constexpr auto has_pattern = [](const sf::String& pattern) {
    return [&pattern](const sf::String& item) { return item.find(pattern) != sf::String::InvalidPos; };
};
constexpr auto item_generator = [](sf::Font& font, size_t font_size) {
    return [&font, font_size](const sf::String& item) {
        return sf::Text(item, font, font_size);
    };
};

template <class InputIt, class OutputIt, class Predicate>
std::pair<InputIt, InputIt> filter_n(const InputIt begin, const InputIt end, 
                                     OutputIt result, Predicate pred, size_t limit)
{
    InputIt it, first_in_range = begin, last_in_range = end;
    size_t count = 0;
    for (it = begin; it != end && count < limit; ++it) {
        if (pred(*it)) {
            *result++ = *it;
            if (count++ == 0) {
                first_in_range = it;
            }
        }
    }
    last_in_range = it;
    return std::make_pair(first_in_range, last_in_range);
}

// Takes a single event from the event queue in a blocking manner. Returns whether it was processed usefully 
bool wmenu::process_event() {
    auto shift_range_right = [this](size_t amount) {
        std::vector<raw_item_type> buffer;
        buffer.reserve(amount);
        auto rpeek = filter_n(filtered_range.second, raw_items.cend(), 
                              std::back_inserter(buffer),
                              has_pattern(searchbar.getString()),
                              amount).second;
        if (!buffer.empty()) {
            std::transform(std::make_move_iterator(buffer.begin()), std::make_move_iterator(buffer.end()),
                           std::back_inserter(items), item_generator(font, conf.font.size));
            std::rotate(items.begin(), items.begin() + amount, items.end());
            filtered_range.first = filter_n(filtered_range.first, raw_items.cend(),
                                            buffer.begin(),
                                            has_pattern(searchbar.getString()),
                                            amount).second;
            filtered_range.second = rpeek;
            items.pop_back();
            position_items();
        }
    };
    auto shift_range_left = [this](size_t amount) {
        std::vector<raw_item_type> buffer;
        buffer.reserve(amount);
        auto lpeek = filter_n(std::make_reverse_iterator(filtered_range.first),
                              std::make_reverse_iterator(raw_items.cbegin()),
                              std::back_inserter(buffer),
                              has_pattern(searchbar.getString()),
                              amount).second.base();
        if (!buffer.empty()) {
            std::transform(std::make_move_iterator(buffer.begin()), std::make_move_iterator(buffer.end()),
                           std::front_inserter(items), item_generator(font, conf.font.size));
            filtered_range.first = lpeek;
            filtered_range.second = filter_n(std::make_reverse_iterator(filtered_range.second),
                                             std::make_reverse_iterator(raw_items.cbegin()),
                                             buffer.begin(),
                                             has_pattern(searchbar.getString()),
                                             amount).second.base();
            items.pop_back();
            position_items();
        }
    };

    sf::Event event;
    bool processed = false;
    if (window.waitEvent(event)) {
        processed = true;
        if (event.type == sf::Event::Closed || 
            (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape))
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

void wmenu::filter_items(const sf::String& pattern) {
    const auto item_found = has_pattern(pattern);

    std::vector<sf::String> filtered_items;
    filtered_range = filter_n(raw_items.cbegin(), raw_items.cend(),
                              std::back_inserter(filtered_items), item_found, conf.limit);

    items.clear();
    std::transform(
        std::make_move_iterator(filtered_items.begin()),
        std::make_move_iterator(filtered_items.end()),
        std::back_inserter(items),
        item_generator(font, conf.font.size)
    );

    position_items();
}

void wmenu::position_items() {
    float centering_offset = (conf.height - conf.font.size) / 2;

    if (conf.orientation == conf.horizontal) {
        float offset = conf.search_margin * sf::VideoMode::getDesktopMode().width + conf.padding;
        for (auto& item : items) {
            item.setPosition(offset, centering_offset);
            offset += item.getGlobalBounds().width + conf.padding;
        }
    } else if (conf.orientation == conf.vertical) {
        float offset = conf.height;
        for (auto& item : items) {
            item.setPosition(offset, centering_offset);
            offset += conf.height;
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