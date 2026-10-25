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
#include "tesoro/objects.h"
#include "tesoro/testing.h"
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
	_hovered.clear();
	_pointerText.clear();
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
	_probe.reset(new PointerProbe());
	_font.reset(new Animation(readAnimation(_archive.read("FUENTES.ANM"))));
	_renderer.setShadowTable(_archive.read("TABLA1.TBL"));
	// Temporary pointer until the original interface cursor is ported.
	uint32 cursor[12 * 18] = {};
	for (uint y = 0; y < 18; ++y)
		for (uint x = 0; x <= y / 2 && x < 12; ++x)
			cursor[y * 12 + x] = x == 0 || x == y / 2 || y == 17 ? 0xff000000 : 0xffffffff;
	CursorMan.replaceCursor(cursor, 12, 18, 0, 0, 0, false, &_screen->format);

	_state = kMainMenu;
	if (!TestingShortcuts::kSkipStartupScreens)
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
	// The pointer (drawn by the game, see draw) shows while the player can act; scripts take the control away.
}

void TesoroEngine::clickWalk(const Common::Point &mouse) {
	// The pointer is measured from the middle of the screen, y downwards, and limited to the screen.
	int32 x, y;
	worldPoint(mouse, x, y);
	// Nothing in the room answers clicks yet, so every click is a click on the ground.
	_jim->startWalk(x, y, nullptr, _gameTime);
}

void TesoroEngine::worldPoint(const Common::Point &mouse, int32 &x, int32 &y) const {
	// The pointer is measured from the middle of the screen, y downwards, and limited to the screen.
	const int32 screenX = CLIP<int32>(mouse.x - 320, -320, 319);
	const int32 screenY = CLIP<int32>(mouse.y - 240, -240, 239);
	const SceneView &camera = _currentScene->camera().position();
	x = screenX + camera.x;
	y = camera.y - screenY;
}

const TextEntry *TesoroEngine::findText(const Common::String &name) const {
	return _library.findText(encodeName(name));
}

void TesoroEngine::walkJim(int32 x, int32 y, ActionWaiter *waiter) {
	_jim->startWalk(x, y, waiter, _gameTime);
}

void TesoroEngine::turnJim(int direction) {
	_jim->look(ScriptOp::Direction(direction), _gameTime);
}

void TesoroEngine::jimSays(const TextEntry &text) {
	startSpeech(_jim.get(), text, 0);
	setInputEnabled(false);
	_inputAfterSpeech = true;
}

void TesoroEngine::clearPointer() {
	_hovered.clear();
	_interfaceHovered.clear();
	_pointerText.clear();
	_leftClick = _rightClick = false;
}

int TesoroEngine::reactPointer(PointerProbe &probe, Common::Array<SceneObject *> &hovered) {
	// The objects that the probe overlapped on the last frame, nearest first, are asked in turn
	// until one takes the pointer. One that was not hovered before is only hovered now.
	const Common::Array<SceneObject *> &candidates = probe.candidates();
	const int verb = _cursor ? _cursor->verb() : 0;
	Common::Array<SceneObject *> before = hovered;
	hovered.clear();
	int result = 0;
	for (uint i = 0; i < candidates.size() && result == 0; ++i) {
		SceneObject *object = candidates[i];
		uint wasAt = 0;
		while (wasAt < before.size() && before[wasAt] != object)
			++wasAt;
		if (wasAt == before.size()) {
			if (object->hitTest(*this)) {
				hovered.push_back(object);
				result = object->onHover(*this);
			}
		} else {
			if (_leftClick)
				result = object->onLeftClick(*this, verb);
			else if (_rightClick)
				result = object->onRightClick(*this, verb);
			else
				result = object->onHover(*this);
			if (result == 3)
				break;
			hovered.push_back(object);
			before.remove_at(wasAt);
		}
	}
	for (uint i = 0; i < before.size(); ++i)
		before[i]->onUnhover(*this);
	return result;
}

void TesoroEngine::updatePointer() {
	// The interface sees the pointer first.
	int result = _interface ? reactPointer(*_interfaceProbe, _interfaceHovered) : 0;
	bool onInterface = result != 0;
	if (!onInterface)
		result = reactPointer(*_probe, _hovered);
	if (_hovered.empty() && _interfaceHovered.empty())
		_pointerText.clear();

	// A click that nothing took is a click on the ground, if the pointer shows the default verb.
	if (_leftClick && result == 0 && _currentScene && (!_cursor || _cursor->verb() == 0))
		clickWalk(_clickAt);
	_leftClick = _rightClick = false;
}

void TesoroEngine::setVerb(int verb) {
	if (_cursor)
		_cursor->setVerb(verb, _gameTime);
}

Scene *TesoroEngine::levelScene(int level) {
	return scene(ScriptOp::kLevel, level);
}

void TesoroEngine::startObjectScript(const char *block, const char *label, uint8 *variables, Scene &scene) {
	const ScriptBlock *found = _library.find(block);
	if (!found)
		error("Tesoro: The script %s does not exist.", block);
	// An object without variables of its own starts the script with a block of zeros.
	if (!variables) {
		memset(_noVariables, 0, sizeof(_noVariables));
		variables = _noVariables;
	}
	startScript(*found, encodeName(label), variables, scene);
}

int TesoroEngine::jimLevel() const {
	for (uint i = 0; i < ARRAYSIZE(kSceneNames); ++i) {
		Common::HashMap<Common::String, Common::SharedPtr<Scene>>::const_iterator it = _scenes.find(kSceneNames[i]);
		if (it != _scenes.end() && it->_value == _currentScene)
			return i;
	}
	return -1;
}

void TesoroEngine::transferJim(Scene &destination, int32 x, int32 y, int direction) {
	for (Common::HashMap<Common::String, Common::SharedPtr<Scene>>::iterator it = _scenes.begin(); it != _scenes.end(); ++it) {
		if (it->_value.get() == &destination) {
			Common::SharedPtr<Scene> scene = it->_value;
			placeJim(scene, x, y);
			_jim->look(ScriptOp::Direction(direction), _gameTime);
			return;
		}
	}
	error("Tesoro: Jim is sent to a scene that is not loaded.");
}

void TesoroEngine::loadInterface() {
	_interface.reset(new Scene(readScene(_archive.read("INTERFACE.LVL")), _gameTime));
	// The interface is the size of the screen and does not scroll; the pointer is measured from its corner.
	SceneBounds bounds;
	bounds.left = 0;
	bounds.bottom = 0;
	bounds.right = 640;
	bounds.top = 480;
	_interface->camera().setBounds(bounds);
	_interface->camera().setPosition(320, 240);
	_cursor = static_cast<CursorObject *>(_interface->find("CURSOR"));
	ControlIconObject *control = static_cast<ControlIconObject *>(_interface->find("CONTROL_ICO"));
	if (!_cursor || _cursor->kind() != 10 || !control || control->kind() != 11)
		error("Tesoro: The interface is missing its pointer.");
	control->bind(*_interface);
	_interfaceProbe.reset(new PointerProbe());
	_interface->addObject(_interfaceProbe, 0);
	_interfaceHovered.clear();
}

void TesoroEngine::placeCursor() {
	if (!_cursor)
		return;
	// The pointer in the interface's world: x to the right, y upwards, from the corner of the screen.
	const Common::Point mouse = _eventMan->getMousePos();
	_cursor->setPosition(CLIP<int32>(mouse.x, 0, 639), 479 - CLIP<int32>(mouse.y, 0, 479));
}

void TesoroEngine::updateInterface() {
	if (!_interface)
		return;
	placeCursor();
	const int32 x = _cursor->x(), y = _cursor->y();
	_interfaceProbe->clearCandidates();
	_interfaceProbe->placeAt(x, y);
	_interface->objectMoved(_interfaceProbe.get());
	_interface->resolveOverlaps(_gameTime);
}

void TesoroEngine::drawHotspots() {
	Graphics::Surface &surface = *_screen->surfacePtr();
	const Graphics::PixelFormat &format = surface.format;
	const uint32 green = format.ARGBToColor(255, 0, 255, 0), yellow = format.ARGBToColor(255, 255, 255, 0);
	const uint32 cyan = format.ARGBToColor(255, 0, 255, 255), magenta = format.ARGBToColor(255, 255, 0, 255);
	const uint32 red = format.ARGBToColor(255, 255, 0, 0);

	Common::Array<SceneDebugShape> shapes;
	_currentScene->debugShapes(shapes);
	const Common::Array<SceneObject *> &candidates = _probe->candidates();
	for (uint i = 0; i < shapes.size(); ++i) {
		const SceneDebugShape &shape = shapes[i];
		// Yellow: what the probe overlapped on the last frame. Cyan: Jim. Green: the rest.
		bool candidate = false;
		for (uint c = 0; c < candidates.size(); ++c)
			candidate |= candidates[c] == shape.object;
		const uint32 colour = candidate ? yellow : shape.object->overlapBody().isJim ? cyan : green;
		surface.frameRect(shape.box, colour);
		// The outline of the frame, when it has one, closed.
		for (uint p = 0; p + 1 < shape.outline.size(); ++p)
			surface.drawLine(shape.outline[p].x, shape.outline[p].y,
			                 shape.outline[p + 1].x, shape.outline[p + 1].y, magenta);
	}
	// The probe is where the mouse is.
	const Common::Point mouse = _eventMan->getMousePos();
	surface.drawLine(mouse.x - 6, mouse.y, mouse.x + 6, mouse.y, red);
	surface.drawLine(mouse.x, mouse.y - 6, mouse.x, mouse.y + 6, red);
}

void TesoroEngine::updateProbe() {
	if (!_currentScene)
		return;
	if (_probeScene != _currentScene.get()) {
		if (_probeScene)
			_probeScene->removeObject(_probe.get());
		// The probe and Jim share the first group of a scene.
		_currentScene->addObject(_probe, 0);
		_probeScene = _currentScene.get();
	}
	// What was overlapped on the last frame has been used by now.
	_probe->clearCandidates();
	int32 x, y;
	worldPoint(_eventMan->getMousePos(), x, y);
	_probe->placeAt(x, y);
	_currentScene->objectMoved(_probe.get());
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
		} else if (_menuShown && event.type == Common::EVENT_LBUTTONDOWN) {
			_menuClick = true;
		} else if (_inputEnabled && _currentScene &&
		           (event.type == Common::EVENT_LBUTTONDOWN || event.type == Common::EVENT_RBUTTONDOWN)) {
			if (event.type == Common::EVENT_LBUTTONDOWN)
				_leftClick = true;
			else
				_rightClick = true;
			_clickAt = event.mouse;
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
			if (TestingShortcuts::kSkipStartupScreens) {
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
		if (_interface) {
			// The interface is in front of the whole room. The shade behind the verb bar is a shadow:
			// it darkens the room and sits behind the icons; the pointer is in front of everything.
			Common::Array<SceneSprite> interfaceSprites;
			_interface->appendSprites(_gameTime, _randomSource, interfaceSprites);
			for (uint i = 0; i < interfaceSprites.size(); ++i) {
				SceneSprite &sprite = interfaceSprites[i];
				const bool pointer = sprite.image == _cursor->image();
				if (pointer && !_inputEnabled && !_menuShown)
					continue;
				sprite.depth = pointer ? 0 : sprite.image->kind == 2 ? 2 : 1;
				sprites.push_back(sprite);
			}
		}
		_renderer.draw(*_screen->surfacePtr(), sprites);
		if (_menuShown && _font)
			drawMenu();
		if (_showHotspots)
			drawHotspots();
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
		// The name of what the pointer is over is shown above it, 37 pixels up, in white.
		if (_inputEnabled && !_pointerText.empty() && _font) {
			const Common::Point mouse = _eventMan->getMousePos();
			_renderer.drawText(*_screen->surfacePtr(), *_font, _pointerText, 0xffff, mouse.x, mouse.y - 37, 200);
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
		if (_inputAfterSpeech && !_speaking) {
			_inputAfterSpeech = false;
			setInputEnabled(true);
		}
		if (_currentScene) {
			if (_inputEnabled) {
				updatePointer();
				updateProbe();
				updateInterface();
			} else {
				clearPointer();
			}
			_currentScene->resolveOverlaps(_gameTime);
		}
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
	_inputAfterSpeech = false;
	_menuShown = _menuSession = false;
	_scripts.clear();
	_scenes.clear();
	_probeScene = nullptr;
	loadInterface();
	CursorMan.showMouse(false);
	_movieHold = false;
	_gameTime = 0;
	_lastHostTime = _system->getMillis();
	if (TestingShortcuts::kStartInOffice) {
		// What the introduction leaves behind: the harbour's door active but hidden, the captain
		// gone from the office and Jim at its door (native ordinal 5 and the end of FANPRESENT).
		Common::SharedPtr<Scene> harbour = loadScene("PUERTO.LVL");
		if (SceneObject *door = harbour->find("PUERTA1")) {
			door->setActive(true);
			door->show(false);
		}
		// The harbour as the script reveals it: no black backdrop, the gull and the recruiter
		// there and moving, the ship sailing.
		SceneMessage off, on, loop, show;
		off.kind = SceneMessage::kActivate;
		on.kind = SceneMessage::kActivate;
		on.on = true;
		loop.kind = SceneMessage::kPlay;
		loop.mode = SceneMessage::kLoop;
		show.kind = SceneMessage::kShow;
		show.on = true;
		harbour->send(_gameTime, "FONDONEGRO", off);
		harbour->send(_gameTime, "GAVIOTA", on);
		harbour->send(_gameTime, "GAVIOTA", loop);
		harbour->send(_gameTime, "RECLUTADOR", on);
		harbour->send(_gameTime, "RECLUTADOR", loop);
		harbour->send(_gameTime, "BARCO", show);
		harbour->send(_gameTime, "BARCO", loop);
		Common::SharedPtr<Scene> office = loadScene("DESPACHO.LVL");
		if (SceneObject *captain = office->find("CAPITAN"))
			captain->setActive(false);
		placeJim(office, 550, 100);
		_state = kAdventure;
		_jim->setActive(true);
		_jim->show(true);
		_jim->look(ScriptOp::kLeft, _gameTime);
		return;
	}
	// The game puts Jim at the floor's origin of the harbour, active and shown, before the script starts.
	Common::SharedPtr<Scene> harbour = loadScene("PUERTO.LVL");
	placeJim(harbour, harbour->entry().x, harbour->entry().y);
	_state = kAdventure;
	_jim->setActive(true);
	_jim->show(true);
	const ScriptBlock *block = _library.find("FANPRESENT");
	if (!block)
		error("Tesoro: The introduction script is missing.");
	startScript(*block, encodeName("main"), _variables, *_currentScene);
}

void TesoroEngine::stopAdventure() {
	_hovered.clear();
	_interfaceHovered.clear();
	_interface.reset();
	_interfaceProbe.reset();
	_cursor = nullptr;
	_pointerText.clear();
	if (_probeScene) {
		_probeScene->removeObject(_probe.get());
		_probeScene = nullptr;
	}
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
	// A script that shows a menu says so again on every frame.
	_menuShown = false;
	// The most recent script goes first, and a script that is still waiting after its update holds
	// back the older ones: the script that started it (a sub-conversation, say) resumes when it ends.
	for (int i = int(_scripts.size()) - 1; i >= 0; --i) {
		Common::SharedPtr<Script> script = _scripts[i];
		script->update();
		if (!script->finished())
			break;
	}
	for (uint i = 0; i < _scripts.size();) {
		if (_scripts[i]->finished())
			_scripts.remove_at(i);
		else
			++i;
	}
	if (_menuSession && _scripts.empty()) {
		// The conversation is over: the verb that was chosen before it comes back.
		_menuSession = false;
		setVerb(_menuSavedVerb);
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
		return _interface.get();
	}
	return nullptr;
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
	talker->beginSpeech(_gameTime, speaker != nullptr, option);
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
	if (!_font)
		error("Tesoro: A menu needs the font.");
	_menuShown = true;
	if (!_menuSession) {
		// The pointer is the arrow throughout the conversation; the verb comes back when it ends.
		_menuSession = true;
		_menuClick = false;
		_pointerText.clear();
		_menuSavedVerb = _cursor ? _cursor->verb() : 0;
		setVerb(0);
	}
	placeCursor();
	_menuChoices.clear();
	for (uint i = 0; i < choices.size(); ++i)
		_menuChoices.push_back(choices[i]->text);
	// A choice answers to the pointer from where it starts to where the next one does (the last
	// one down to the bottom edge of the screen).
	_menuLayout = _renderer.layoutMenu(*_screen->surfacePtr(), *_font, _menuChoices);

	const int mouseY = CLIP<int>(_eventMan->getMousePos().y, 0, 479);
	_menuHovered = -1;
	for (uint i = 0; i < choices.size(); ++i)
		if (_menuLayout.top(i) <= mouseY && mouseY < _menuLayout.bottom(i))
			_menuHovered = i;
	if (_menuHovered >= 0 && _menuClick) {
		const int chosen = _menuHovered;
		_menuClick = false;
		_menuShown = false;
		_menuHovered = -1;
		_menuChoices.clear();
		return chosen;
	}
	_menuClick = false;
	return -1;
}

void TesoroEngine::dialogueChoiceDone() {
	// The next menu of the conversation, if there is one, keeps the arrow; see updateScripts.
}

void TesoroEngine::drawMenu() {
	_renderer.drawMenu(*_screen->surfacePtr(), *_font, _menuChoices, _menuLayout, _menuHovered);
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
	if (scriptType == 21 && ordinal == 0) {
		// The captain leaves the office: the script has Jim walk to the door, and the door is used.
		SceneObject *door = loadScene("DESPACHO.LVL")->find("PUERTA1");
		if (!door || door->kind() != 6)
			error("Tesoro: The office has no door.");
		static_cast<DoorObject *>(door)->useFromScript(*this);
		return;
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
