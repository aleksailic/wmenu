#ifndef WMENU_CONFIG_HPP
#define WMENU_CONFIG_HPP

#include <SFML/Graphics/Color.hpp>
#include <string>

struct wmenu_config {
	struct scheme {
		sf::Color fg;
		sf::Color bg;
	};
	struct {
		const scheme normal = { sf::Color{187, 187, 187}, sf::Color{34, 34, 34} };
		const scheme selected = { sf::Color{238, 238, 238}, sf::Color{0, 85, 119} };
		const scheme out = { sf::Color{0, 0, 0}, sf::Color{0, 255, 255} };
	} schemes;
	struct {
		size_t size = 15;
		std::string path = "C:\\Windows\\Fonts\\seguiemj.ttf";
	} font;

	unsigned int height = 40;
	unsigned int padding = 20;
	float search_margin = 0.2f; //in percentage

	enum { horizontal, vertical };
	enum { top, bottom };
	
	uint_fast8_t orientation = horizontal;
	uint_fast8_t position = top;

	size_t limit = 10;
	bool insensitive = false;
	std::string delimiters = ";\r\n";
};

#endif // !WMENU_CONFIG_HPP
