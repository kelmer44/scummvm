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
	_gameDescription(gameDesc), _randomSource("Tesoro") {
	g_engine = this;
}

TesoroEngine::~TesoroEngine() {
	_scripts.clear();
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

void TesoroEngine::startScene(const Common::String &name) {
	_scripts.clear();
	_currentScene.reset();
	_scene = readScene(_archive.read(name));

	RoomEntry entry;
	if(!findRoomEntry(_scene, entry))
		error("Tesoro: Failed to find room entry.");
	_camera.setBounds(entry.bounds);
	_camera.setPosition(entry.x, entry.y);

	_gameTime = 0;
	_lastHostTime = _system->getMillis();
	_currentScene.reset(new Scene(_scene, _gameTime));

	_state = kAdventure;
	CursorMan.showMouse(false);
}

void TesoroEngine::init() {
	// The game always shows the text of what is said; the option can still turn it off.
	ConfMan.registerDefault("subtitles", true);
	_archive.open();
	_mainMenu = readMenu(_archive.read("PRINCIPAL.LVL"));
	_newGameMenu = readMenu(_archive.read("NUEVO.LVL"));
	_library.load(_archive);
	_font.reset(new Animation(readAnimation(_archive.read("FUENTES.ANM"))));
	// Temporary pointer until the original interface cursor is ported.
	uint32 cursor[12 * 18] = {};
	for (uint y = 0; y < 18; ++y)
		for (uint x = 0; x <= y / 2 && x < 12; ++x)
			cursor[y * 12 + x] = x == 0 || x == y / 2 || y == 17 ? 0xff000000 : 0xffffffff;
	CursorMan.replaceCursor(cursor, 12, 18, 0, 0, 0, false, &_screen->format);

	_state = kMainMenu;
	startMovie(0, kStartupMovie);
	CursorMan.showMouse(_state == kMainMenu);
	if (ConfMan.hasKey("tesoro_test_adventure")) startAdventure(); // LOCAL-TEST-HOOK
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
	if (_state == kAdventure) {
		if (event.type == Common::EVENT_KEYDOWN &&
			event.kbd.keycode == Common::KEYCODE_ESCAPE) {
			stopAdventure();
			_state = kMainMenu;
			CursorMan.showMouse(true);
		} else if (_speaking && (event.type == Common::EVENT_LBUTTONDOWN ||
		           (event.type == Common::EVENT_KEYDOWN && event.kbd.keycode == Common::KEYCODE_SPACE))) {
			_mixer->stopHandle(_speech);
			_speechSkipped = true;
		}
		return;
	}
	if (_state == kStartupMovie || _state == kNewGameMovie || _state == kAdventureMovie) {
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
		case 3: {
			startAdventure();
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
	if(_state == kAdventure && _currentScene) {
		Common::Array<SceneSprite> sprites;
		_currentScene->appendSprites(_gameTime, _camera, sprites);
		_renderer.draw(*_screen->surfacePtr(), sprites);
		if (!_subtitle.empty() && _font && (ConfMan.getBool("subtitles") || ConfMan.getBool("speech_mute")))
			_renderer.drawNarration(*_screen->surfacePtr(), *_font, _subtitle, 0x001f);
	}
	else if (_state != kStartupMovie && _state != kNewGameMovie && _state != kAdventureMovie) {
		const Image *background = _state == kMainMenu ? _mainMenu.background.get() : _newGameMenu.background.get();
		if (_state == kCensorship && _newGameMenu.censorship)
			background = _newGameMenu.censorship.get();
		_screen->fillRect(Common::Rect(640, 480), 0xff000000);
		drawImage(*_screen->surfacePtr(), *background, 0, 0);
	}
	_screen->makeAllDirty();

}

void TesoroEngine::update() {
	//clock update
	const uint32 now = _system->getMillis();
	const uint32 elapsed = now - _lastHostTime;
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
		updateSpeech();
		updateScripts();
	}
	else if (_state == kCensorship && int32(_system->getMillis() - _deadline) >= 0) {
		_state = kDifficultyMenu;
		CursorMan.showMouse(true);
	}
}


void TesoroEngine::startAdventure() {
	memset(_variables, 0, sizeof(_variables));
	_inputEnabled = true;
	startScene("PUERTO.LVL");
	const ScriptBlock *block = _library.find("FANPRESENT");
	if (!block)
		error("Tesoro: The introduction script is missing.");
	startScript(*block, encodeName("main"), _variables, *_currentScene);
}

void TesoroEngine::stopAdventure() {
	_scripts.clear();
	_mixer->stopHandle(_speech);
	_mixer->stopHandle(_effect);
	_speaking = false;
	_subtitle.clear();
	_currentScene.reset();
}

void TesoroEngine::updateScripts() {
	for (uint i = 0; i < _scripts.size(); ++i)
		_scripts[i]->update();
	for (uint i = 0; i < _scripts.size();) {
		if (_scripts[i]->finished())
			_scripts.remove_at(i);
		else
			++i;
	}
}

void TesoroEngine::updateSpeech() {
	if (!_speaking)
		return;
	const bool done = _speechSkipped || (_speechMuted ? int32(_gameTime - _speechUntil) >= 0
	                                                   : !_mixer->isSoundHandleActive(_speech));
	if (done) {
		_speaking = false;
		_subtitle.clear();
	}
}

Audio::SeekableAudioStream *TesoroEngine::openWav(const Common::String &name) {
	if (!_archive.has(name))
		error("Tesoro: Missing audio %s.", name.c_str());
	Audio::SeekableAudioStream *audio = Audio::makeWAVStream(_archive.stream(name), DisposeAfterUse::YES);
	if (!audio)
		error("Tesoro: Cannot decode audio %s.", name.c_str());
	return audio;
}

Scene *TesoroEngine::scene(ScriptOp::Scope scope, int level) {
	return scope == ScriptOp::kCurrentScene ? _currentScene.get() : nullptr;
}

void TesoroEngine::startScript(const ScriptBlock &block, uint64 labelId, uint8 *variables, Scene &scene) {
	Common::SharedPtr<Script> script(new Script(*this, block, labelId, variables, scene));
	_scripts.push_back(script);
	script->start();
}

void TesoroEngine::startSpeech(SceneObject *speaker, const TextEntry &text, int option) {
	if (speaker)
		error("Tesoro: Character speech is not implemented (%s).", text.name.c_str());
	// The narrator: the text is centred on the screen and read from the speech file.
	_mixer->stopHandle(_speech);
	Audio::SeekableAudioStream *audio = openWav(text.name + ".WAV");
	_subtitle = text.text;
	_speaking = true;
	_speechSkipped = false;
	_speechMuted = ConfMan.getBool("speech_mute");
	_speechUntil = _gameTime + audio->getLength().msecs();
	if (_speechMuted)
		delete audio;
	else
		_mixer->playStream(Audio::Mixer::kSpeechSoundType, &_speech, audio);
}

bool TesoroEngine::isSpeaking(SceneObject *speaker) const {
	if (speaker)
		error("Tesoro: Character speech is not implemented.");
	return _speaking;
}

int TesoroEngine::pickDialogueChoice(const Common::Array<const TextEntry *> &choices) {
	error("Tesoro: Dialogue choices are not implemented.");
}

void TesoroEngine::startMovie(uint number) {
	// A missing movie is skipped, as at the start of the game.
	if (!startMovie(number, kAdventureMovie))
		warning("Tesoro: Skipping movie %u.", number);
}

void TesoroEngine::playSound(const Common::String &name) {
	_mixer->stopHandle(_effect);
	_mixer->playStream(Audio::Mixer::kSFXSoundType, &_effect, openWav(name + ".WAV"));
}

void TesoroEngine::playMusic(uint track) {
	warning("Tesoro: CD audio is not implemented (track %u).", track);
}

void TesoroEngine::doNative(uint scriptType, uint ordinal) {
	if (scriptType == 1) {
		switch (ordinal) {
		case 0: // Jim is hidden and activated; he does not exist yet.
		case 1:
		case 2:
		case 6: // These have no effect in the original.
			return;
		case 3: {
			SceneMessage show;
			show.kind = SceneMessage::kShow;
			show.on = true;
			_currentScene->send(_gameTime, "BARCO", show);
			SceneMessage play;
			play.kind = SceneMessage::kPlay;
			play.mode = SceneMessage::kLoop;
			_currentScene->send(_gameTime, "BARCO", play);
			return;
		}
		default:
			break;
		}
	}
	error("Tesoro: Native action %u of script type %u is not implemented.", ordinal, scriptType);
}

void TesoroEngine::walk(SceneObject &object, int x, int y, Script &waiter) {
	error("Tesoro: Walking is not implemented.");
}

void TesoroEngine::look(SceneObject &object, ScriptOp::Direction direction) {
	error("Tesoro: Turning a character is not implemented.");
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

Common::Error TesoroEngine::syncGame(Common::Serializer &s) {
	return Common::Error(s.isLoading() ? Common::kReadingFailed : Common::kWritingFailed,
		"Tesoro save state is not implemented yet.");
}

} // End of namespace Tesoro
