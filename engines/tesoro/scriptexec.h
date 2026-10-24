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

#ifndef TESORO_SCRIPTEXEC_H
#define TESORO_SCRIPTEXEC_H

#include "tesoro/scene.h"
#include "tesoro/script.h"

namespace Tesoro {

class Script;

// Smaller interface with the api needed by the scripts to run things from the rest of the game.
class ScriptHost {
public:
	virtual ~ScriptHost() {}

	// The scene clock, in milliseconds.
	virtual uint32 gameTime() const = 0;
	// The scene a sendmessage refers to when it names a scope; null if it is not loaded.
	virtual Scene *scene(ScriptOp::Scope scope, int level) = 0;
	virtual void setInputEnabled(bool enabled) = 0;
	virtual void startScript(const ScriptBlock &block, uint64 labelId, uint8 *variables, Scene &scene) = 0;

	// A speaker of null is the narrator.
	virtual void startSpeech(SceneObject *speaker, const TextEntry &text, int option) = 0;
	virtual bool isSpeaking(SceneObject *speaker) const = 0;
	virtual int pickDialogueChoice(const Common::Array<const TextEntry *> &choices) = 0;

	virtual void startMovie(uint number) = 0;
	virtual bool isMovieFinished() const = 0;
	virtual void playSound(const Common::String &name) = 0;
	virtual void playMusic(uint track) = 0;

	// The native code behind DO lines, selected by the script type of the block. The script
	// that runs it can be sent to another scene.
	virtual void doNative(uint scriptType, uint ordinal, Script &script) = 0;
	// Characters walk and turn; a walk ends by calling Script::actionDone.
	virtual void walk(SceneObject &object, int x, int y, Script &waiter) = 0;
	virtual void look(SceneObject &object, ScriptOp::Direction direction) = 0;
};

// One started script block. It runs until an op has to wait, and update resumes it.
class Script {
public:
	Script(ScriptHost &host, const ScriptBlock &block, uint64 labelId, uint8 *variables, Scene &scene);

	// Begins at the label and runs to the first wait, like the original's start.
	void start();
	void update();
	// The object the script is waiting on has finished its action (a walk).
	void actionDone();

	// The scene that the script's objects are looked up in, and the scene of its sendmessage lines
	// that name no other one. Native code moves a script to another scene with setScene.
	Scene &scene() const { return *_scene; }
	void setScene(Scene &scene) { _scene = &scene; }

	bool finished() const { return _finished; }
	const ScriptBlock &block() const { return _block; }
	uint pc() const { return _pc; }

private:
	void execute();
	void finish();
	void buildMenu();
	SceneObject *participant(int slot);

	ScriptHost &_host;
	const ScriptBlock &_block;
	uint8 *_variables;
	Scene *_scene;
	Common::Array<SceneObject *> _participants;

	uint _pc = 0;
	bool _finished = false;
	int _menuReturn = -1; // the menu op that a break returns to
	uint32 _sleepStart = 0;

	SceneObject *_speaker = nullptr;
	bool _speechEnded = false;
	uint32 _speechEndedAt = 0;

	SceneObject *_animationWait = nullptr;

	// Menu choices, as listed when the menu op ran.
	Common::Array<const TextEntry *> _choices;
	Common::Array<uint> _choiceTargets;
	int _chosen = -1;
};

} // End of namespace Tesoro

#endif
