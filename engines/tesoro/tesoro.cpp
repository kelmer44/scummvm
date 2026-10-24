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

namespace {

// The scenes of the game by number, which is how scripts refer to them (LEVELn).
const char *const kSceneNames[] = {
	"DESPACHO.LVL", "PUERTO.LVL", "TABERNA.LVL", "CUBIERTA.LVL", "CAMAROTE.LVL", "ALMACEN.LVL",
	"COCINA.LVL", "PLAYA.LVL", "MAPA.LVL", "POBLADO.LVL", "PANTANO.LVL", "CAMPAMENTO.LVL",
	"VOLCAN.LVL", "ARMARIO.LVL", "TABLERO.LVL", "TIERRA.LVL", "RISCO.LVL", "CUBIERTA2.LVL",
	"CAMAROTE2.LVL", "ALMACEN2.LVL", "COCINA2.LVL", "LUCHA.LVL", "TRONO.LVL", "ENTRADA.LVL",
	"1.LVL", "2.LVL", "3.LVL", "4.LVL", "5.LVL", "6.LVL", "7.LVL", "8.LVL", "9.LVL", "10.LVL",
	"11.LVL", "12.LVL", "13.LVL", "14.LVL", "15.LVL", "16.LVL", "17.LVL", "18.LVL", "19.LVL",
	"20.LVL", "RISCOFINAL.LVL", "FINAL.LVL", "CREDITOS.LVL"
};

} // namespace

// TEMPORARY, to speed up testing: skips the splash movie, the new game movie and the censorship
// screen. They are meant to come from the original scripts; remove this when they do.
static const bool kSkipStartupScreens = true;

// How many times faster the game clock runs in fast mode (Ctrl+F).
static const uint kFastModeFactor = 5;

Common::SharedPtr<Scene> TesoroEngine::loadScene(const Common::String &name) {
	Common::SharedPtr<Scene> &scene = _scenes[name];
	if (!scene)
		scene.reset(new Scene(readScene(_archive.read(name)), _gameTime));
	return scene;
}

void TesoroEngine::placeJim(const Common::SharedPtr<Scene> &scene, int32 x, int32 y) {
	_jim->leave();
	scene->addObject(_jim, 0);
	_jim->place(*scene, x, y, _gameTime);
	scene->camera().setPosition(x, y);
	scene->camera().setActor(_jim.get());
	_currentScene = scene;
	if (scene->entry().musicCue)
		playMusic(scene->entry().musicCue);
}

void TesoroEngine::init() {
	// The game always shows the text of what is said; the option can still turn it off.
	ConfMan.registerDefault("subtitles", true);
	_archive.open();
	_mainMenu = readMenu(_archive.read("PRINCIPAL.LVL"));
	_newGameMenu = readMenu(_archive.read("NUEVO.LVL"));
	_library.load(_archive);
	Common::Array<Common::SharedPtr<Animation>> jimBanks;
	for (uint i = 0; i < JimObject::bankCount(); ++i)
		jimBanks.push_back(Common::SharedPtr<Animation>(new Animation(readAnimation(_archive.read(JimObject::bankName(i))))));
	_jim.reset(new JimObject(jimBanks));
	_font.reset(new Animation(readAnimation(_archive.read("FUENTES.ANM"))));
	// Temporary pointer until the original interface cursor is ported.
	uint32 cursor[12 * 18] = {};
	for (uint y = 0; y < 18; ++y)
		for (uint x = 0; x <= y / 2 && x < 12; ++x)
			cursor[y * 12 + x] = x == 0 || x == y / 2 || y == 17 ? 0xff000000 : 0xffffffff;
	CursorMan.replaceCursor(cursor, 12, 18, 0, 0, 0, false, &_screen->format);

	_state = kMainMenu;
	if (!kSkipStartupScreens)
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
		_movieHold = true;
		_movieHoldUntil = _system->getMillis() + 500;
		return;
	} else if (_state == kNewGameMovie) {
		_state = kCensorship;
		_deadline = _system->getMillis() + 5500; // FANMMENU/nuevo in ANIMS.DLG.
	} else {
		_state = kMainMenu;
	}
	CursorMan.showMouse(_state == kMainMenu);
}

void TesoroEngine::setInputEnabled(bool enabled) {
	_inputEnabled = enabled;
	// The pointer shows while the player can act; scripts take the control away.
	if (_state == kAdventure)
		CursorMan.showMouse(enabled);
}

void TesoroEngine::clickWalk(const Common::Point &mouse) {
	// The pointer is measured from the middle of the screen, y downwards, and limited to the screen.
	const int32 screenX = CLIP<int32>(mouse.x - 320, -320, 319);
	const int32 screenY = CLIP<int32>(mouse.y - 240, -240, 239);
	const SceneView &camera = _currentScene->camera().position();
	// Nothing in the room answers clicks yet, so every click is a click on the ground.
	_jim->startWalk(screenX + camera.x, camera.y - screenY, nullptr, _gameTime);
}

void TesoroEngine::handleEvent(const Common::Event &event) {
	if (event.type == Common::EVENT_QUIT || event.type == Common::EVENT_RETURN_TO_LAUNCHER) {
		_movie.close();
		return;
	}
	if (event.type == Common::EVENT_KEYDOWN && event.kbd.keycode == Common::KEYCODE_f &&
	    event.kbd.hasFlags(Common::KBD_CTRL)) {
		_fastMode = !_fastMode;
		_system->displayMessageOnOSD(Common::U32String(_fastMode ? "Fast mode on" : "Fast mode off"));
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
		} else if (_inputEnabled && _currentScene && event.type == Common::EVENT_LBUTTONDOWN) {
			clickWalk(event.mouse);
		}
		return;
	}
	if (_state == kStartupMovie || _state == kNewGameMovie || _state == kAdventureMovie) {
		if (_movieHold)
			return;
		// A movie of the adventure can also be skipped with the mouse.
		if ((event.type == Common::EVENT_KEYDOWN && event.kbd.keycode == Common::KEYCODE_ESCAPE) ||
		    (_state == kAdventureMovie && event.type == Common::EVENT_LBUTTONDOWN))
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
			if (kSkipStartupScreens) {
				_state = kDifficultyMenu;
				CursorMan.showMouse(true);
				break;
			}
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
		_currentScene->appendSprites(_gameTime, _randomSource, sprites);
		_renderer.draw(*_screen->surfacePtr(), sprites);
		if (_speaking && _talker && _font && (ConfMan.getBool("subtitles") || ConfMan.getBool("speech_mute"))) {
			if (_narration) {
				_renderer.drawNarration(*_screen->surfacePtr(), *_font, _subtitle, _talker->narrationColour());
			} else {
				// What a character says is centred above its head.
				int32 x, y;
				_talker->speechAnchor(_currentScene->camera().groupView(_talker->parallax()), x, y);
				_renderer.drawText(*_screen->surfacePtr(), *_font, _subtitle, _talker->speechColour(), x, y, _talker->speechWidth());
			}
		}
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
		_gameTime += _fastMode ? elapsed * kFastModeFactor : elapsed;
	if (_state == kAdventureMovie && _movieHold) {
		if (int32(now - _movieHoldUntil) >= 0) {
			_movieHold = false;
			_state = kAdventure;
			CursorMan.showMouse(_inputEnabled);
		}
	} else if (_state == kStartupMovie || _state == kNewGameMovie || _state == kAdventureMovie) {
		if (_movie.endOfVideo() || (_fastMode && _state == kAdventureMovie)) {
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
		if (_jim->isWalking())
			_jim->update(_gameTime);
		updateSpeech();
		_jim->commitPosition();
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
	_scripts.clear();
	_scenes.clear();
	_movieHold = false;
	_gameTime = 0;
	_lastHostTime = _system->getMillis();
	// The game puts Jim at the floor's origin of the harbour, active and shown, before the script starts.
	Common::SharedPtr<Scene> harbour = loadScene("PUERTO.LVL");
	placeJim(harbour, harbour->entry().x, harbour->entry().y);
	_state = kAdventure;
	CursorMan.showMouse(false);
	_jim->setActive(true);
	_jim->show(true);
	const ScriptBlock *block = _library.find("FANPRESENT");
	if (!block)
		error("Tesoro: The introduction script is missing.");
	startScript(*block, encodeName("main"), _variables, *_currentScene);
}

void TesoroEngine::stopAdventure() {
	_scripts.clear();
	_jim->leave();
	_mixer->stopHandle(_speech);
	_mixer->stopHandle(_effect);
	_speaking = false;
	_subtitle.clear();
	_talker = nullptr;
	_currentScene.reset();
	_scenes.clear();
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
		if (_talker)
			_talker->endSpeech();
		_talker = nullptr;
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
	switch (scope) {
	case ScriptOp::kCurrentScene:
		return _currentScene.get();
	case ScriptOp::kLevel:
		if (level < 0 || uint(level) >= ARRAYSIZE(kSceneNames))
			error("Tesoro: A script names scene %d, which does not exist.", level);
		return loadScene(kSceneNames[level]).get();
	case ScriptOp::kInventory:
		break;
	}
	error("Tesoro: A script sends a message to the inventory, which is not implemented.");
}

void TesoroEngine::startScript(const ScriptBlock &block, uint64 labelId, uint8 *variables, Scene &scene) {
	Common::SharedPtr<Script> script(new Script(*this, block, labelId, variables, scene));
	_scripts.push_back(script);
	script->start();
}

void TesoroEngine::startSpeech(SceneObject *speaker, const TextEntry &text, int option) {
	// The narrator is Jim, who does not move his mouth.
	SceneObject *talker = speaker ? speaker : _jim.get();
	if (!talker->canTalk())
		error("Tesoro: %s cannot talk.", text.name.c_str());
	_mixer->stopHandle(_speech);
	Audio::SeekableAudioStream *audio = openWav(text.name + ".WAV");
	talker->beginSpeech(_gameTime, speaker != nullptr);
	_talker = talker;
	_narration = speaker == nullptr;
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
	return (speaker ? speaker : _jim.get())->isSpeaking();
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

void TesoroEngine::doNative(uint scriptType, uint ordinal, Script &script) {
	if (scriptType == 1) {
		switch (ordinal) {
		case 0: // Jim is hidden and activated, and the view goes to him.
			_jim->show(false);
			_jim->setActive(true);
			_currentScene->camera().setPosition(_jim->x(), _jim->y());
			return;
		case 1:
		case 2:
		case 6: // These have no effect in the original.
			return;
		case 4:
			_jim->show(true);
			return;
		case 3: {
			// The ship of the harbour appears and sails in a loop.
			Common::SharedPtr<Scene> harbour = loadScene("PUERTO.LVL");
			SceneMessage show;
			show.kind = SceneMessage::kShow;
			show.on = true;
			harbour->send(_gameTime, "BARCO", show);
			SceneMessage play;
			play.kind = SceneMessage::kPlay;
			play.mode = SceneMessage::kLoop;
			harbour->send(_gameTime, "BARCO", play);
			return;
		}
		case 5: {
			// Jim goes through the harbour's door into the office. The door is left active but
			// hidden, the script continues in the office, and Jim stands at its door.
			SceneObject *door = loadScene("PUERTO.LVL")->find("PUERTA1");
			if (!door)
				error("Tesoro: The harbour has no door.");
			door->setActive(true);
			door->show(false);
			Common::SharedPtr<Scene> office = loadScene("DESPACHO.LVL");
			script.setScene(*office);
			placeJim(office, 550, 100);
			return;
		}
		default:
			break;
		}
	}
	error("Tesoro: Native action %u of script type %u is not implemented.", ordinal, scriptType);
}

void TesoroEngine::walk(SceneObject &object, int x, int y, Script &waiter) {
	if (&object != _jim.get())
		error("Tesoro: Only Jim can walk so far.");
	_jim->startWalk(x, y, &waiter, _gameTime);
}

void TesoroEngine::look(SceneObject &object, ScriptOp::Direction direction) {
	object.turn(direction, _gameTime);
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
