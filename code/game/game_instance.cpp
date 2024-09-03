#include "game/game_instance.hpp"

namespace vc::game {

int game_instance::run() {
	while (!m_window.is_closing()) {
		m_window.pull_events();
	}
	m_device.wait_for_idle();

	return EXIT_SUCCESS;
}

} // namespace vc::game
