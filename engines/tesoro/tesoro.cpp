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
#include "audio/audiostream.h"
#include "audio/decoders/wave.h"

namespace Tesoro {

TesoroEngine *g_engine;

TesoroEngine::TesoroEngine(OSystem *syst, const ADGameDescription *gameDesc) : Engine(syst),
	_gameDescription(gameDesc), _randomSource("Tesoro"), _script(_catalog, *this) {
	g_engine = this;
}

TesoroEngine::~TesoroEngine() {
	_mixer->stopHandle(_speech);
	_mixer->stopHandle(_effect);
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
	if (_state == kAdventureMovie) {
		_state = kAdventure;
	} else if (_state == kNewGameMovie) {
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
	if (_state == kStartupMovie || _state == kNewGameMovie || _state == kAdventureMovie) {
		if (event.type == Common::EVENT_KEYDOWN && event.kbd.keycode == Common::KEYCODE_ESCAPE)
			stopMovie();
		return;
	}
	if (_state == kAdventure) {
		if (event.type == Common::EVENT_KEYDOWN && event.kbd.keycode == Common::KEYCODE_ESCAPE) {
			if (!_script.failure.empty())
				leaveAdventure();
			else
				openMainMenuDialog();
		} else if (_wait == kWaitSpeech && (event.type == Common::EVENT_LBUTTONDOWN ||
		           (event.type == Common::EVENT_KEYDOWN && event.kbd.keycode == Common::KEYCODE_SPACE))) {
			_mixer->stopHandle(_speech);
			_speechSkipped = true;
		}
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
		case 3:
			startAdventure();
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
	_lastHostTime = _system->getMillis();

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
	if (_state == kAdventure && _objects) {
		Common::Array<SceneSprite> sprites;
		_objects->appendSprites(_gameTime, _camera, sprites);
		_renderer.draw(*_screen->surfacePtr(), sprites);
		if (!_subtitle.empty() && _font && (ConfMan.getBool("subtitles") || ConfMan.getBool("speech_mute")))
			_renderer.drawNarration(*_screen->surfacePtr(), *_font, _subtitle, 0x001f);
	} else if (_state != kStartupMovie && _state != kNewGameMovie && _state != kAdventureMovie) {
		const Image *background = _state == kMainMenu ? _mainMenu.background.get() : _newGameMenu.background.get();
		if (_state == kCensorship && _newGameMenu.censorship)
			background = _newGameMenu.censorship.get();
		_screen->copyRectToSurface(background->pixels.data(), background->w * sizeof(uint32), 0, 0, 640, 480);
	}
	_screen->makeAllDirty();

}

void TesoroEngine::update() {
	uint32 now = _system->getMillis();
	uint32 elapsed = now - _lastHostTime;
	_lastHostTime = now;
	if (isPaused())
		return;
	if (_state == kAdventure && elapsed < 500)
		_gameTime += elapsed;
	if (_state == kStartupMovie || _state == kNewGameMovie || _state == kAdventureMovie) {
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
	else if (_state == kAdventure) {
		_script.update();
		if (!_script.failure.empty() && !_failureReported) {
			_failureReported = true;
			warning("Tesoro introduction paused: %s", _script.failure.c_str());
			GUI::MessageDialog dialog(Common::U32String("La introducción ha llegado a una función que todavía no está implementada. Pulsa Escape para volver al menú."));
			dialog.runModal();
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
	else {
		_lastHostTime = _system->getMillis();
		if (_state == kCensorship)
			_deadline += _lastHostTime - _pausedAt;
	}
}

void TesoroEngine::startAdventure() {
	_mixer->stopHandle(_speech);
	_mixer->stopHandle(_effect);
	_script.stop();
	_objects.reset();
	_requestError.clear();
	_subtitle.clear();
	_wait = kNoWait;
	_failureReported = false;
	_animationWait = nullptr;
	_gameTime = 0;
	_lastHostTime = _system->getMillis();
	memset(_variables, 0, sizeof(_variables));
	if (!_catalogLoaded) {
		_catalog.load(_archive);
		_font.reset(new Animation(readAnimation(_archive.read("FUENTES.ANM"))));
		_catalogLoaded = true;
	}
	_harbour = readScene(_archive.read("PUERTO.LVL"));
	RoomEntry entry;
	if (!findRoomEntry(_harbour, entry))
		error("Tesoro: Cannot resolve the harbour entry.");
	_camera.setBounds(entry.bounds);
	_camera.setPosition(entry.x, entry.y);
	_objects.reset(new SceneObjects(_harbour, _gameTime));
	_state = kAdventure;
	CursorMan.showMouse(false);
	_script.start("FANPRESENT", "main", _variables);
	debug(1, "Tesoro: starting FANPRESENT/main at (%d,%d)", entry.x, entry.y);
}

void TesoroEngine::leaveAdventure() {
	_script.stop();
	_mixer->stopHandle(_speech);
	_mixer->stopHandle(_effect);
	_wait = kNoWait;
	_subtitle.clear();
	_objects.reset();
	_state = kDifficultyMenu;
	CursorMan.showMouse(true);
}

bool TesoroEngine::request(const Request &r) {
	switch (r.kind) {
	case Request::kMovie:
		if (!startMovie(r.number, kAdventureMovie)) {
			_requestError = Common::String::format("Cannot play introduction movie %u", r.number);
			return false;
		}
		_wait = kWaitMovie;
		return true;
	case Request::kSpeech: {
		if (!r.target.empty()) {
			_requestError = "Character speech is not implemented: " + r.target;
			return false;
		}
		Audio::SeekableAudioStream *audio = Audio::makeWAVStream(_archive.stream(r.textId + ".WAV"), DisposeAfterUse::YES);
		if (!audio) {
			_requestError = "Cannot decode speech " + r.textId;
			return false;
		}
		_mixer->stopHandle(_speech);
		_subtitle = r.text;
		_speechSkipped = false;
		_speechMuted = ConfMan.getBool("speech_mute");
		_waitUntil = _gameTime + audio->getLength().msecs();
		if (_speechMuted)
			delete audio;
		else
			_mixer->playStream(Audio::Mixer::kSpeechSoundType, &_speech, audio);
		_wait = kWaitSpeech;
		return true;
	}
	case Request::kSleep:
		_waitUntil = _gameTime + r.number;
		_wait = kWaitSleep;
		return true;
	case Request::kSound: {
		Audio::SeekableAudioStream *audio = Audio::makeWAVStream(_archive.stream(r.textId + ".WAV"), DisposeAfterUse::YES);
		if (!audio) {
			_requestError = "Cannot decode sound " + r.textId;
			return false;
		}
		_mixer->stopHandle(_effect);
		_mixer->playStream(Audio::Mixer::kSFXSoundType, &_effect, audio);
		return false;
	}
	case Request::kNative:
		if (r.nativeType == 1) {
			if (r.number == 0 || r.number == 1 || r.number == 2 || r.number == 6)
				return false;
			if (r.number == 3) {
				SceneObject *ship = _objects->find("BARCO");
				if (!ship)
					break;
				ship->show(true);
				if (!ship->play(0, _gameTime, 0, -1, true))
					break;
				return false;
			}
		}
		_requestError = Common::String::format("Native action type %u ordinal %u is not implemented", r.nativeType, r.number);
		return false;
	case Request::kMessage: {
		SceneObject *object = _objects->find(r.target);
		if (!object || !r.scope.empty()) {
			_requestError = "Unsupported object message target: " + r.target;
			return false;
		}
		const Common::Array<Common::String> &w = r.words;
		if (w.size() == 2 && (w[0] == "activate" || w[0] == "show") && (w[1] == "on" || w[1] == "off")) {
			if (w[0] == "activate")
				object->setActive(w[1] == "on");
			else
				object->show(w[1] == "on");
			return false;
		}
		if (w.size() == 4 && w[0] == "play" && (w[3] == "stop" || w[3] == "play" || w[3] == "loop" || w[3] == "wait")) {
			uint first = atoi(w[2].c_str());
			if (object->play(atoi(w[1].c_str()), _gameTime, first, w[3] == "stop" ? int(first) : -1, w[3] == "loop")) {
				if (w[3] == "wait") {
					_wait = kWaitAnimation;
					_animationWait = object;
					return true;
				}
				return false;
			}
		}
		_requestError = "Unsupported message to " + r.target;
		return false;
	}
	default:
		_requestError = "Unsupported media request";
		return false;
	}
	_requestError = "Cannot resolve the introduction action";
	return false;
}

bool TesoroEngine::complete() {
	switch (_wait) {
	case kWaitMovie:
		if (_state == kAdventureMovie)
			return false;
		break;
	case kWaitSleep:
		if (int32(_gameTime - _waitUntil) < 0)
			return false;
		break;
	case kWaitSpeech:
		if (!_speechSkipped && (_speechMuted ? int32(_gameTime - _waitUntil) < 0 : _mixer->isSoundHandleActive(_speech)))
			return false;
		_mixer->stopHandle(_speech);
		_subtitle.clear();
		break;
	case kWaitAnimation:
		if (_animationWait && (_animationWait->animationState().flags & AnimationPlayer::kPlaying))
			return false;
		_animationWait = nullptr;
		break;
	default:
		break;
	}
	_wait = kNoWait;
	return true;
}

Common::Error TesoroEngine::syncGame(Common::Serializer &s) {
	return Common::Error(s.isLoading() ? Common::kReadingFailed : Common::kWritingFailed,
		"Tesoro save state is not implemented yet.");
}

} // End of namespace Tesoro
