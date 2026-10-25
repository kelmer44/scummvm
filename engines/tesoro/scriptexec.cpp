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

#include "common/textconsole.h"

#include "tesoro/scriptexec.h"

namespace Tesoro {

namespace {

// After a speaker stops talking the script waits a little before the next line.
const uint32 kSpeechPause = 201;

} // namespace

Script::Script(ScriptHost &host, const ScriptBlock &block, uint64 labelId, uint8 *variables, Scene &scene)
	: _host(host), _block(block), _variables(variables), _scene(&scene) {
	_participants.resize(block.participants.size());
	for (uint i = 0; i < _participants.size(); ++i)
		_participants[i] = scene.find(block.participants[i]);
	// A label that is not found starts the block from its first op.
	const int index = block.findLabel(labelId);
	_pc = index < 0 ? 0 : index;
}

void Script::start() {
	_host.setInputEnabled(false);
	execute();
}

SceneObject *Script::participant(int slot) {
	if (slot < 1 || uint(slot) > _participants.size())
		error("Tesoro: %s (%s): no participant %d.", _block.file.c_str(), _block.name.c_str(), slot);
	SceneObject *&object = _participants[slot - 1];
	if (!object)
		object = _scene->find(_block.participants[slot - 1]);
	if (!object)
		error("Tesoro: %s (%s): participant %d is not in the scene.", _block.file.c_str(), _block.name.c_str(), slot);
	return object;
}

void Script::finish() {
	// Whoever talked in the script goes back to what they were doing.
	for (uint i = 0; i < _participants.size(); ++i)
		if (_participants[i] && _participants[i]->hasSavedAnimation())
			_participants[i]->restoreAnimation(_host.gameTime());
	_finished = true;
	_host.setInputEnabled(true);
}

void Script::actionDone() {
	if (_finished)
		return;
	++_pc;
	execute();
}

void Script::buildMenu() {
	// The items form a chain; each one is followed by the line that the player says.
	_choices.clear();
	_choiceTargets.clear();
	_chosen = -1;
	uint item = _pc;
	for (;;) {
		const ScriptOp &op = _block.ops[item];
		if (op.type != ScriptOp::kMenuItem || item + 1 >= _block.ops.size() ||
		    _block.ops[item + 1].type != ScriptOp::kSpeech)
			error("Tesoro: %s:%u (%s): malformed menu.", _block.file.c_str(), op.line, _block.name.c_str());
		bool listed = op.var == 0xff;
		if (!listed)
			listed = op.invert ? _variables[op.var] == 0 : _variables[op.var] == 1;
		if (listed) {
			_choices.push_back(_block.ops[item + 1].text);
			_choiceTargets.push_back(item + 2);
		}
		if (op.next < 1)
			break;
		item = op.next;
	}
	_pc = item;
}

void Script::execute() {
	while (!_finished) {
		if (_pc >= _block.ops.size())
			error("Tesoro: %s (%s): script ran past its last op.", _block.file.c_str(), _block.name.c_str());
		const ScriptOp &op = _block.ops[_pc];
		switch (op.type) {
		case ScriptOp::kReturn:
		case ScriptOp::kEnd:
			finish();
			return;

		case ScriptOp::kSpeech:
			_speechEnded = false;
			_speaker = op.speaker > 0 ? participant(op.speaker) : nullptr;
			_host.startSpeech(_speaker, *op.text, op.option);
			return;

		case ScriptOp::kSleep:
			_sleepStart = _host.gameTime();
			return;

		case ScriptOp::kBreak:
			if (_menuReturn < 0) {
				finish();
				return;
			}
			_pc = _menuReturn;
			break;

		case ScriptOp::kJump:
			_pc = op.target;
			break;

		case ScriptOp::kMenu:
			_menuReturn = _pc;
			++_pc;
			buildMenu();
			return;

		case ScriptOp::kIf:
			_pc = _variables[op.var] == op.value ? op.target : _pc + 1;
			break;

		case ScriptOp::kRun:
			if (op.block) {
				_host.startScript(*op.block, op.labelId, _variables, *_scene);
				return;
			}
			// A run of FANMTRONO is a native action, chosen by its label.
			_host.doNative(20, op.labelId == encodeName("ir") ? 0 : 1, *this);
			++_pc;
			break;

		case ScriptOp::kMessage: {
			Scene *target = _scene;
			if (op.scope != ScriptOp::kCurrentScene) {
				target = _host.scene(op.scope, op.level);
				if (!target)
					error("Tesoro: %s:%u (%s): message to a scene that is not loaded.", _block.file.c_str(), op.line, _block.name.c_str());
			}
			SceneObject *object = target->find(op.object);
			if (!object)
				error("Tesoro: %s:%u (%s): message to an object that is not in the scene.", _block.file.c_str(), op.line, _block.name.c_str());
			switch (op.kind) {
			case ScriptOp::kWalk:
				_host.walk(*object, op.x, op.y, *this);
				return;
			case ScriptOp::kLook:
				_host.look(*object, op.direction);
				break;
			default: {
				SceneMessage message;
				message.kind = op.kind == ScriptOp::kPlay ? SceneMessage::kPlay
					: op.kind == ScriptOp::kShow ? SceneMessage::kShow : SceneMessage::kActivate;
				message.on = op.on;
				message.bank = op.bank;
				message.first = (uint16)(int16)op.first;
				message.mode = (SceneMessage::PlayMode)op.mode;
				_animationWait = target->send(_host.gameTime(), op.object, message);
				if (_animationWait)
					return;
				break;
			}
			}
			++_pc;
			break;
		}

		case ScriptOp::kAssign:
			_variables[op.var] = op.value;
			++_pc;
			break;

		case ScriptOp::kNative:
			_host.doNative(_block.nativeType, op.value, *this);
			++_pc;
			break;

		case ScriptOp::kMovie:
			_host.startMovie(op.value);
			return;

		case ScriptOp::kSound:
			_host.playSound(op.name);
			++_pc;
			break;

		case ScriptOp::kMusic:
			_host.playMusic(op.value);
			++_pc;
			break;

		case ScriptOp::kMenuItem:
			error("Tesoro: %s:%u (%s): a menu item was reached outside its menu.", _block.file.c_str(), op.line, _block.name.c_str());
		}
	}
}

void Script::update() {
	if (_finished)
		return;
	const ScriptOp &op = _block.ops[_pc];
	switch (op.type) {
	case ScriptOp::kSpeech:
		if (_host.isSpeaking(_speaker))
			return;
		if (!_speechEnded) {
			_speechEnded = true;
			_speechEndedAt = _host.gameTime();
		}
		if (_host.gameTime() - _speechEndedAt < kSpeechPause)
			return;
		++_pc;
		break;

	case ScriptOp::kSleep:
		if (_host.gameTime() - _sleepStart < uint32(op.value))
			return;
		++_pc;
		break;

	case ScriptOp::kMenu:
	case ScriptOp::kMenuItem:
		if (_chosen < 0) {
			_chosen = _host.pickDialogueChoice(_choices);
			if (_chosen < 0)
				return;
			// Jim says the chosen line before the script goes on.
			_host.startSpeech(_host.player(), *_choices[_chosen], 0);
			return;
		}
		if (_host.isSpeaking(_host.player()))
			return;
		_host.dialogueChoiceDone();
		_pc = _choiceTargets[_chosen];
		_chosen = -1;
		break;

	case ScriptOp::kRun:
		_host.setInputEnabled(false);
		++_pc;
		break;

	case ScriptOp::kMessage:
		if (op.kind != ScriptOp::kPlay) // a walk waits for actionDone
			return;
		_animationWait->prepareFrame(_host.gameTime(), _host.randomSource());
		if (_animationWait->isPlaying())
			return;
		++_pc;
		break;

	case ScriptOp::kMovie:
		if (!_host.isMovieFinished())
			return;
		++_pc;
		break;

	default:
		return;
	}
	execute();
}

} // End of namespace Tesoro
