#include <cstdio>
#include <exception>

#include "game/game_instance.hpp"

int main() try {
	return vc::game::game_instance{}.run();
} catch (const std::exception &e) {
	std::printf("[main] Fatal error: %s\n", e.what());
	return EXIT_FAILURE;
}