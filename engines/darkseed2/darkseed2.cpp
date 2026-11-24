/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "darkseed2/darkseed2.h"
#include "graphics/framelimiter.h"
#include "darkseed2/detection.h"
#include "darkseed2/console.h"
#include "common/scummsys.h"
#include "common/config-manager.h"
#include "common/debug-channels.h"
#include "common/events.h"
#include "common/system.h"
#include "engines/util.h"
#include "graphics/paletteman.h"

namespace DarkSeed2 {

DarkSeed2Engine *g_engine;

DarkSeed2Engine::DarkSeed2Engine(OSystem *syst, const ADGameDescription *gameDesc) : Engine(syst),
	_gameDescription(gameDesc), _randomSource("DarkSeed2") {
	g_engine = this;
}

DarkSeed2Engine::~DarkSeed2Engine() {
	delete _screen;
}

uint32 DarkSeed2Engine::getFeatures() const {
	return _gameDescription->flags;
}

Common::String DarkSeed2Engine::getGameId() const {
	return _gameDescription->gameId;
}

Common::Error DarkSeed2Engine::run() {
	int32 width = 640, height = 480;
	const Graphics::PixelFormat format(2, 5, 6, 5, 0, 11, 5, 0, 0);

	if (!initGraphics(width, height, &format))
		return Common::kUnknownError;

	_screen = new Graphics::Screen(width, height, g_system->getScreenFormat());

	// Set the engine's debugger console
	setDebugger(new Console());

	// If a savegame was selected from the launcher, load it
	int saveSlot = ConfMan.getInt("save_slot");
	if (saveSlot != -1)
		(void)loadGameState(saveSlot);

	Common::Event e;
	_screen->clear();
	Graphics::FrameLimiter limiter(g_system, 60);
	while (!shouldQuit()) {
		while (g_system->getEventManager()->pollEvent(e)) {
		}

		// Delay for a bit. All events loops should have a delay
		// to prevent the system being unduly loaded
		limiter.delayBeforeSwap();
		_screen->update();
		limiter.startFrame();
	}

	return Common::kNoError;
}

bool DarkSeed2Engine::initGraphics(int32 width, int32 height, const Graphics::PixelFormat *format) {
	debug(-1, "Setting up graphics...");

	::initGraphics(width, height, format);

	return true;
}

Common::Error DarkSeed2Engine::syncGame(Common::Serializer &s) {
	// The Serializer has methods isLoading() and isSaving()
	// if you need to specific steps; for example setting
	// an array size after reading it's length, whereas
	// for saving it would write the existing array's length
	int dummy = 0;
	s.syncAsUint32LE(dummy);

	return Common::kNoError;
}

} // End of namespace DarkSeed2
