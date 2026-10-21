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

#include "common/scummsys.h"
#include "common/config-manager.h"
#include "common/debug-channels.h"
#include "common/events.h"
#include "common/system.h"
#include "engines/util.h"
#include "graphics/cursorman.h"
#include "graphics/paletteman.h"
#include "graphics/surface.h"
#include "gui/message.h"

#include "tesoro/tesoro.h"
#include "tesoro/detection.h"
#include "tesoro/console.h"
#include "graphics/framelimiter.h"

namespace Tesoro {

TesoroEngine *g_engine;

TesoroEngine::TesoroEngine(OSystem *syst, const ADGameDescription *gameDesc) : Engine(syst),
	_gameDescription(gameDesc), _randomSource("Tesoro") {
	g_engine = this;
}

TesoroEngine::~TesoroEngine() {
	_movie.close();
	delete _screen;
}

uint32 TesoroEngine::getFeatures() const {
	return _gameDescription->flags;
}

Common::String TesoroEngine::getGameId() const {
	return _gameDescription->gameId;
}

void TesoroEngine::init() {
	_archive.open();
	_mainMenu = readMenu(_archive.read("PRINCIPAL.LVL"));
	_newGameMenu = readMenu(_archive.read("NUEVO.LVL"));

	// Temporary pointer until the original interface cursor is ported.
	uint32 cursor[12 * 18] = {};
	for (uint y = 0; y < 18; ++y)
		for (uint x = 0; x <= y / 2 && x < 12; ++x)
			cursor[y * 12 + x] = x == 0 || x == y / 2 || y == 17 ? 0xff000000 : 0xffffffff;
	CursorMan.replaceCursor(cursor, 12, 18, 0, 0, 0, false, &_screen->format);

	_state = kMainMenu;
	startMovie(0, kStartupMovie);
	CursorMan.showMouse(_state == kMainMenu);
}

bool TesoroEngine::startMovie(uint number, State state) {
	_movie.close();
	Common::String filename = Common::String::format("data%02u.bin", number);
	Common::Path gamePath = ConfMan.getPath("path");
	Common::Array<Common::Path> directories;
	if (ConfMan.hasKey("tesoro_movie_path"))
		directories.push_back(ConfMan.getPath("tesoro_movie_path"));
	directories.push_back(gamePath);
	directories.push_back(gamePath.join("bin"));
	directories.push_back(gamePath.join("Install").join("bin"));
	Common::SeekableReadStream *stream = nullptr;
	for (uint i = 0; i < directories.size() && !stream; ++i)
		stream = Common::FSNode(directories[i].join(filename)).createReadStream();
	if (!stream) {
		warning("Tesoro: cannot open movie %s", filename.c_str());
		return false;
	}
	// Validate before transferring ownership: AVI only takes the stream after
	// recognizing its RIFF/AVI header, and frees it on subsequent load failures.
	byte header[12];

	if (stream->read(header, sizeof(header)) != sizeof(header) ||
		memcmp(header, "RIFF", 4) || memcmp(header + 8, "AVI ", 4)) {
		delete stream;
		warning("Tesoro: invalid AVI movie %s", filename.c_str());
		return false;
	}

	stream->seek(0);
	if (!_movie.loadStream(stream)) {
		// After the validated RIFF/AVI header, AVIDecoder owns the stream,
		// including when a later parse/codec failure closes it.
		warning("Tesoro: cannot decode movie %s", filename.c_str());
		return false;
	}

	_state = state;
	_screen->fillRect(Common::Rect(640, 480), 0xff000000);
	_movie.start();
	return true;
}

void TesoroEngine::stopMovie() {
	debug("Tesoro: stopping movie");
	_movie.close();
	if (_state == kNewGameMovie) {
		_state = kCensorship;
		_deadline = _system->getMillis() + 5500; // FANMMENU/nuevo in ANIMS.DLG.
	} else {
		_state = kMainMenu;
	}
	CursorMan.showMouse(_state == kMainMenu);
}

void TesoroEngine::handleEvent(const Common::Event &event) {
	if (event.type == Common::EVENT_QUIT || event.type == Common::EVENT_RETURN_TO_LAUNCHER) {
		_movie.close();
		return;
	}
	if (_state == kStartupMovie || _state == kNewGameMovie) {
		if (event.type == Common::EVENT_KEYDOWN && event.kbd.keycode == Common::KEYCODE_ESCAPE)
			stopMovie();
		return;
	}
	if (event.type == Common::EVENT_KEYDOWN && event.kbd.keycode == Common::KEYCODE_ESCAPE) {
		if (_state == kDifficultyMenu) {
			_state = kMainMenu;
			CursorMan.showMouse(true);
		} else if (_state == kMainMenu) {
			openMainMenuDialog();
		}
		return;
	}
	if (event.type != Common::EVENT_LBUTTONDOWN || _state == kCensorship)
		return;
	const Menu &menu = _state == kMainMenu ? _mainMenu : _newGameMenu;
	for (uint i = 0; i < menu.buttons.size(); ++i) {
		const MenuButton &button = menu.buttons[i];
		if (!button.bounds.contains(event.mouse))
			continue;
		switch (button.action) {
		case 0:
			_state = kNewGameMovie;
			if (!startMovie(1, kNewGameMovie))
				stopMovie();
			break;
		case 1: {
			GUI::MessageDialog dialog(Common::U32String("Cargar partidas todavía no está disponible."));
			dialog.runModal();
			break;
		}
		default: {
			GUI::MessageDialog dialog(Common::U32String("Este modo de juego todavía no está disponible."));
			dialog.runModal();
			break;
		}
		}
		return;
	}
}

Common::Error TesoroEngine::run() {

	Graphics::PixelFormat format(4, 8, 8, 8, 8, 16, 8, 0, 24);
	initGraphics(640, 480, &format);
	_screen = new Graphics::Screen();

	// Set the engine's debugger console
	setDebugger(new Console());
	init();

	// A host pacing limit; movie decoding and card duration use elapsed time.
	Graphics::FrameLimiter limiter(_system, 60, false);
	limiter.initialize();
	Common::Event event;
	while (!shouldQuit()) {
		limiter.startFrame();
		while (_eventMan->pollEvent(event))
			handleEvent(event);
		if (shouldQuit())
			break;
		update();
		draw();
		limiter.delayBeforeSwap();
		_screen->update();
	}
	_movie.close();

	return Common::kNoError;
}

void TesoroEngine::draw() {
	if (_state != kStartupMovie && _state != kNewGameMovie) {
		const Image *background = _state == kMainMenu ? _mainMenu.background.get() : _newGameMenu.background.get();
		if (_state == kCensorship && _newGameMenu.censorship)
			background = _newGameMenu.censorship.get();
		_screen->copyRectToSurface(background->pixels.data(), background->w * sizeof(uint32), 0, 0, 640, 480);
	}
	_screen->makeAllDirty();

}

void TesoroEngine::update() {
	if (isPaused())
		return;
	if (_state == kStartupMovie || _state == kNewGameMovie) {
		if (_movie.endOfVideo()) {
			stopMovie();
		} else if (_movie.needsUpdate()) {
			debug("Tesoro: decoding movie frame %u", _decodedMovieFrames);
			const Graphics::Surface *frame = _movie.decodeNextFrame();
			if (frame) {
				Graphics::Surface *converted = frame->convertTo(_screen->format);
				_screen->copyRectToSurface(converted->getPixels(), converted->pitch, 0, 0, converted->w, converted->h);
				converted->free();
				delete converted;
				++_decodedMovieFrames;
			}
		}
	}
	else if (_state == kCensorship && int32(_system->getMillis() - _deadline) >= 0) {
		_state = kDifficultyMenu;
		CursorMan.showMouse(true);
	}
}


void TesoroEngine::pauseEngineIntern(bool pause) {
	Engine::pauseEngineIntern(pause);
	_movie.pauseVideo(pause);
	if (pause)
		_pausedAt = _system->getMillis();
	else if (_state == kCensorship)
		_deadline += _system->getMillis() - _pausedAt;
}

Common::Error TesoroEngine::syncGame(Common::Serializer &s) {
	return Common::Error(s.isLoading() ? Common::kReadingFailed : Common::kWritingFailed,
		"Tesoro save state is not implemented yet.");
}

} // End of namespace Tesoro
