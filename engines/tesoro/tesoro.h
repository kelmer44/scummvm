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

#ifndef TESORO_H
#define TESORO_H

#include "common/scummsys.h"
#include "common/system.h"
#include "common/error.h"
#include "common/fs.h"
#include "common/events.h"
#include "common/hash-str.h"
#include "common/random.h"
#include "common/serializer.h"
#include "common/util.h"
#include "engines/engine.h"
#include "engines/savestate.h"
#include "graphics/screen.h"
#include "audio/mixer.h"
#include "video/avi_decoder.h"

#include "tesoro/detection.h"
#include "tesoro/resources.h"
#include "tesoro/jim.h"
#include "tesoro/renderer.h"
#include "tesoro/scene.h"
#include "tesoro/scriptexec.h"

namespace Tesoro {

struct TesoroGameDescription;

class TesoroEngine : public Engine, public ScriptHost, public PointerHost {
private:
	const ADGameDescription *_gameDescription;
	Common::RandomSource _randomSource;

	Archive _archive;

	enum State { kStartupMovie, kMainMenu, kNewGameMovie, kCensorship, kDifficultyMenu, kAdventureMovie, kAdventure };
	State _state = kMainMenu;
	Menu _mainMenu, _newGameMenu;
	Video::AVIDecoder _movie;
	uint32 _deadline = 0, _pausedAt = 0;

	// Scene clock in ms. It only advances while a scene is shown, and a single host
	// delta of 500 ms or more (a stall or a dialog) is ignored, as in the original.
	uint32 _gameTime = 0, _lastHostTime = 0;
	uint _decodedMovieFrames = 0;
	// A movie of the adventure leaves its last frame on the screen for half a second.
	bool _movieHold = false;
	uint32 _movieHoldUntil = 0;

	SceneRenderer _renderer;
	// Every scene that has been loaded stays loaded, with whatever state the game left in it.
	Common::HashMap<Common::String, Common::SharedPtr<Scene>> _scenes;
	Common::SharedPtr<Scene> _currentScene;
	Common::SharedPtr<JimObject> _jim;
	// The pointer's probe and the scene that it is in. Whatever it overlaps each frame is what the
	// pointer can pick.
	Common::SharedPtr<PointerProbe> _probe;
	Scene *_probeScene = nullptr;
	// What the pointer is over: the objects that were hovered on the last frame, the text that
	// shows their name, and the buttons pressed since the last frame.
	Common::Array<SceneObject *> _hovered;
	Common::String _pointerText;
	bool _leftClick = false, _rightClick = false;
	Common::Point _clickAt;

	ScriptLibrary _library;
	Common::Array<Common::SharedPtr<Script>> _scripts;
	// The variable block the opening scripts run with; its ownership is still to be recovered.
	uint8 _variables[256] = {};
	bool _inputEnabled = true;
	// Ctrl+F: the game clock runs faster. Speech with sound keeps its normal speed and ends on its audio or a click (muted lines run on the faster clock); movies of the adventure are skipped.
	bool _fastMode = false;

	Common::SharedPtr<Animation> _font;
	Audio::SoundHandle _speech, _effect;
	Common::String _subtitle;
	bool _speaking = false, _speechMuted = false, _speechSkipped = false;
	// Who is talking. The narrator is Jim, with his mouth closed and the text on the middle of the screen.
	SceneObject *_talker = nullptr;
	bool _narration = false;
	uint32 _speechUntil = 0;

	// Loads a scene by file name the first time it is asked for.
	Common::SharedPtr<Scene> loadScene(const Common::String &name);
	// Moves Jim into a scene, at a position, and makes it the scene that is shown.
	void placeJim(const Common::SharedPtr<Scene> &scene, int32 x, int32 y);
	void startAdventure();
	void stopAdventure();
	void updateSpeech();
	void updateScripts();
	Audio::SeekableAudioStream *openWav(const Common::String &name);

	// ScriptHost
	uint32 gameTime() const override { return _gameTime; }
	Common::RandomSource &randomSource() override { return _randomSource; }
	Scene *scene(ScriptOp::Scope scope, int level) override;
	void setInputEnabled(bool enabled) override;
	void startScript(const ScriptBlock &block, uint64 labelId, uint8 *variables, Scene &scene) override;
	void startSpeech(SceneObject *speaker, const TextEntry &text, int option) override;
	bool isSpeaking(SceneObject *speaker) const override;
	int pickDialogueChoice(const Common::Array<const TextEntry *> &choices) override;
	void startMovie(uint number) override;
	bool isMovieFinished() const override { return _state != kAdventureMovie; }
	void playSound(const Common::String &name) override;
	void playMusic(uint track) override;
	void doNative(uint scriptType, uint ordinal, Script &script) override;
	void walk(SceneObject &object, int x, int y, Script &waiter) override;
	void look(SceneObject &object, ScriptOp::Direction direction) override;

	void init();
	bool startMovie(uint number, State state);
	void stopMovie();
	void handleEvent(const Common::Event &event);
	// A left click on the ground sends Jim walking to that point.
	void clickWalk(const Common::Point &mouse);
	// Where a point of the screen is in the world of the current scene.
	void worldPoint(const Common::Point &mouse, int32 &x, int32 &y) const;
	// Keeps the probe in the current scene and puts it on the mouse.
	void updateProbe();
	// Draws the hit boxes and outlines of the objects and the probe.
	void drawHotspots();
	bool _showHotspots = false;
	// Reacts to the objects that the probe overlapped on the last frame (hover, click). A click
	// that no object takes is a click on the ground.
	void updatePointer();
	void clearPointer();

	// PointerHost
	void setPointerText(const Common::String &text) override { _pointerText = text; }
	const TextEntry *findText(const Common::String &name) const override;
	void walkJim(int32 x, int32 y, ActionWaiter *waiter) override;
	void turnJim(int direction) override;
	void draw();
	void update();

protected:
	// Engine APIs
	Common::Error run() override;
	void pauseEngineIntern(bool pause) override;

public:
	Graphics::Screen *_screen = nullptr;
public:
	TesoroEngine(OSystem *syst, const ADGameDescription *gameDesc);
	~TesoroEngine() override;

	// The hotspot debug layer; only the console switches it.
	bool showHotspots() const { return _showHotspots; }
	void setShowHotspots(bool show) { _showHotspots = show; }

	uint32 getFeatures() const;

	/**
	 * Returns the game Id
	 */
	Common::String getGameId() const;

	/**
	 * Gets a random number
	 */
	uint32 getRandomNumber(uint maxNum) {
		return _randomSource.getRandomNumber(maxNum);
	}

	bool hasFeature(EngineFeature f) const override {
		return
		    (f == kSupportsLoadingDuringRuntime) ||
		    (f == kSupportsSavingDuringRuntime) ||
		    (f == kSupportsReturnToLauncher);
	};

	bool canLoadGameStateCurrently(Common::U32String *msg = nullptr) override {
		return true;
	}
	bool canSaveGameStateCurrently(Common::U32String *msg = nullptr) override {
		return true;
	}

	/**
	 * Uses a serializer to allow implementing savegame
	 * loading and saving using a single method
	 */
	Common::Error syncGame(Common::Serializer &s);

	Common::Error saveGameStream(Common::WriteStream *stream, bool isAutosave = false) override {
		Common::Serializer s(nullptr, stream);
		return syncGame(s);
	}
	Common::Error loadGameStream(Common::SeekableReadStream *stream) override {
		Common::Serializer s(stream, nullptr);
		return syncGame(s);
	}
};

extern TesoroEngine *g_engine;
#define SHOULD_QUIT ::Tesoro::g_engine->shouldQuit()

} // End of namespace Tesoro

#endif // TESORO_H
