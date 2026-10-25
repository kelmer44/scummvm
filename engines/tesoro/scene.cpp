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

#include "tesoro/scene.h"
#include "tesoro/interface.h"
#include "tesoro/objects.h"
#include "tesoro/script.h"

namespace Tesoro {
namespace {

int32 add(int32 a, int32 b) {
	return int32(uint32(a) + uint32(b));
}

int32 subtract(int32 a, int32 b) {
	return int32(uint32(a) - uint32(b));
}

} // namespace

bool SceneCamera::setPosition(int32 x, int32 y, uint32 now, uint32 duration) {
	if (duration) {
		_glideDuration = duration;
		_glideStart = now;
		_from = _position;
		_target.x = x;
		_target.y = y;
		return true;
	}
	_target.x = x;
	_target.y = y;
	_position.x = x;
	_position.y = y;
	bool left = _bounds.left <= add(x, -320);
	if (!left)
		_position.x = add(_bounds.left, 320);
	bool bottom = _bounds.bottom <= add(y, -240);
	if (!bottom)
		_position.y = add(_bounds.bottom, 240);
	bool right = add(_position.x, 320) <= _bounds.right;
	if (!right)
		_position.x = add(_bounds.right, -320);
	bool top = add(_position.y, 240) <= _bounds.top;
	if (!top)
		_position.y = add(_bounds.top, -240);
	return left && bottom && right && top;
}

void SceneCamera::update(uint32 now) {
	// Once the actor gets within 75 pixels of the edge of the view, the camera moves 100 pixels
	// towards that side over 750 ms, as long as its target stays inside the bounds.
	if (_actor) {
		const int32 actorX = _actor->x();
		if (actorX < add(_target.x, -75) && add(_target.x, -320) > _bounds.left) {
			_glideDuration = 750;
			_glideStart = now;
			_target.x = add(_target.x, -100);
			_from = _position;
		}
		if (add(_target.x, 75) < actorX && add(_target.x, 319) < _bounds.right) {
			_glideDuration = 750;
			_glideStart = now;
			_target.x = add(_target.x, 100);
			_from = _position;
		}
	}
	if (_glideDuration) {
		const int32 elapsed = int32(now - _glideStart);
		const int32 duration = int32(_glideDuration);
		if (duration < elapsed) {
			_glideDuration = 0;
			const int32 x = _target.x, y = _target.y;
			setPosition(x, y);
			_target = _position;
		} else {
			const int32 x = add(int32(uint32(subtract(_target.x, _from.x)) * uint32(elapsed)) / duration, _from.x);
			const int32 y = add(int32(uint32(subtract(_target.y, _from.y)) * uint32(elapsed)) / duration, _from.y);
			const SceneView target = _target;
			setPosition(x, y);
			_target = target;
		}
	}
}

SceneView SceneCamera::groupView(int32 parallax) const {
	if (parallax <= 0)
		error("Tesoro: Invalid scene parallax divisor.");
	SceneView view;
	view.x = int32(uint32(_position.x) * 300) / parallax;
	view.y = int32(uint32(_position.y) * 300) / parallax;
	return view;
}

// The room record names the floor and gives the camera bounds.
// iterates through the resource for the scene to find the room record (kind 1)
static const SceneObjectRecord *findRoomRecord(const SceneData &scene) {
	const SceneObjectRecord *room = nullptr;
	for (uint i = 0; i < scene.groups.size(); ++i)
		for (uint j = 0; j < scene.groups[i].objects.size(); ++j)
			if (scene.groups[i].objects[j].kind == 1)
				room = &scene.groups[i].objects[j];
	return room;
}

/**
 * Reads the room entry (default position, bounds, music) from the scene's room object (kind 1)
 */
bool findRoomEntry(const SceneData &scene, RoomEntry &entry) {
	const SceneObjectRecord *room = findRoomRecord(scene);

	if (!room || room->parameters.size() < 17 || room->bankIndices.empty())
		return false;

	uint bankIndex = room->bankIndices[0];
	if (bankIndex >= scene.banks.size() || scene.banks[bankIndex]->frames.empty())
		return false;
	const Frame &frame = scene.banks[bankIndex]->frames[0];
	entry.bounds.left = add(room->x, frame.hitX);
	entry.bounds.top = add(room->y, frame.hitY);
	entry.bounds.right = add(entry.bounds.left, int16(frame.hitW));
	entry.bounds.bottom = add(entry.bounds.top, -int32(int16(frame.hitH)));
	entry.musicCue = room->parameters[14];
	Common::String floorName;
	for (uint i = 0; i < 12 && room->parameters[i]; ++i)
		floorName += char(room->parameters[i]);
	if (floorName.empty())
		return false;
	uint matches = 0;
	for (uint i = 0; i < scene.groups.size(); ++i) {
		for (uint j = 0; j < scene.groups[i].objects.size(); ++j) {
			const SceneObjectRecord &object = scene.groups[i].objects[j];
			if (encodeName(object.name) == encodeName(floorName)) {
				entry.floorGroup = i;
				entry.floorObject = j;
				entry.x = object.x;
				entry.y = object.y;
				++matches;
			}
		}
	}
	return matches == 1;
}

SceneObject::SceneObject(const SceneObjectRecord &record, const SceneGroup &group, const SceneData &scene)
	: _kind(record.kind), _nameId(encodeName(record.name)), _x(record.x), _y(record.y),
	  _depth(uint16(record.depth + group.depthBase + 5)), _name(record.name) {
	for (uint i = 0; i < record.bankIndices.size(); ++i) {
		uint index = record.bankIndices[i];
		if (index >= scene.banks.size())
			error("Tesoro: Invalid animation bank for %s.", record.name.c_str());
		_banks.push_back(scene.banks[index]);
		_players.push_back(Common::SharedPtr<AnimationPlayer>(new AnimationPlayer(*_banks.back())));
	}
}

SceneObject::SceneObject(uint32 kind, uint64 nameId, const Common::Array<Common::SharedPtr<Animation>> &banks,
                         int32 x, int32 y, uint16 depth)
	: _banks(banks), _kind(kind), _nameId(nameId), _x(x), _y(y), _depth(depth) {
	for (uint i = 0; i < _banks.size(); ++i)
		_players.push_back(Common::SharedPtr<AnimationPlayer>(new AnimationPlayer(*_banks[i])));
}

void SceneObject::setActive(bool active) {
	_flags = (_flags & ~uint32(kActive)) | (active ? kActive : 0);
}

void SceneObject::show(bool shown) {
	_flags = (_flags & ~uint32(kShown)) | (shown ? kShown : 0);
}

bool SceneObject::play(uint bank, uint32 now, uint16 first, int last, bool loop, bool cacheDuration) {
	if (bank >= _players.size() || !_players[bank]->play(now, first, last, loop, cacheDuration))
		return false;
	_currentBank = bank;
	// A new animation replaces the one that talking was going to put back.
	_animationSaved = false;
	return true;
}

void SceneObject::playBank(uint bank, uint32 now, uint16 first, int last, bool loop) {
	if (bank >= _players.size() || !_players[bank]->play(now, first, last, loop))
		error("Tesoro: Invalid animation for an object.");
	_currentBank = bank;
}

void SceneObject::turn(int, uint32) {
	error("Tesoro: An object that cannot turn is asked to.");
}

void SceneObject::selectTalkBank(uint32 now) {
	if (_talk.banks.empty())
		return;
	playBank(_talk.banks[0], now, 1, 1);
}

void SceneObject::beginSpeech(uint32 now, bool mouthMoves) {
	if (!_talkable)
		error("Tesoro: An object that cannot talk is asked to.");
	_speaking = true;
	_mouthMoves = mouthMoves;
	if (!_animationSaved) {
		// What it was showing is kept, frozen, until it is put back.
		_animationSaved = true;
		_players[_currentBank]->pause(now);
		_savedBank = _currentBank;
		_savedFlags = _players[_currentBank]->state().flags;
	}
	_lastMouthChange = now;
	selectTalkBank(now);
}

void SceneObject::restoreAnimation(uint32 now) {
	if (!_animationSaved)
		return;
	_animationSaved = false;
	_currentBank = _savedBank;
	_players[_currentBank]->restoreFlags(_savedFlags);
	_players[_currentBank]->resume(now);
}

void SceneObject::speechAnchor(const SceneView &view, int32 &outX, int32 &outY) const {
	// Above the head: the middle of the frame's picture, or of its hit box when it has none.
	const AnimationPlayer &player = selectedPlayer();
	const Frame *frame = player.currentFrame();
	const int32 originX = _x + 320, originY = 240 - _y;
	if (!frame) {
		outX = originX - view.x;
		outY = originY + view.y;
		return;
	}
	if (const Image *image = player.currentImage())
		outX = frame->x + originX + (image->w >> 1);
	else
		outX = int16(frame->hitW) / 2 + originX + frame->hitX;
	outY = originY - frame->hitY - 0x32;
	outX -= view.x;
	outY += view.y;
}

void SceneObject::prepareFrame(uint32 now, Common::RandomSource &random) {
	if (_players.empty())
		return;
	if (_talkable && showsTalk()) {
		// While talking, the first frame of the talking bank is a closed mouth and the others
		// are open ones. A mouth that moves picks an open frame at random every tenth of a second.
		const uint frames = _banks[_currentBank]->frames.size();
		if (!(_speaking && _mouthMoves) || frames < 2) {
			if (frames >= 2)
				playBank(_currentBank, now, 1, 1);
		} else if (now - _lastMouthChange > 100) {
			const uint frame = random.getRandomNumberRng(1, frames - 1);
			playBank(_currentBank, now, frame, frame);
			_lastMouthChange = now;
		}
	}
	_players[_currentBank]->update(now);
	if (dropsMovement())
		_players[_currentBank]->clearMovement();
	const Frame *frame = _players[_currentBank]->currentFrame();
	if (!frame)
		return;
	_hitBounds.left = frame->hitX;
	_hitBounds.top = frame->hitY;
	_hitBounds.right = add(frame->hitX, int16(frame->hitW));
	_hitBounds.bottom = add(frame->hitY, -int32(int16(frame->hitH)));
}

bool SceneObject::sprite(const SceneView &, SceneSprite &) const {
	return false;
}

bool SceneObject::backSprite(const SceneView &view, SceneSprite &sprite) const {
	if (!_talkable || !showsTalk() || _players.empty() ||
	    (flags() & (kActive | kShown)) != (kActive | kShown))
		return false;
	const Animation &bank = *_banks[_currentBank];
	if (bank.frames.empty() || bank.frames[0].image < 0)
		return false;
	const Frame &frame = bank.frames[0];
	const AnimationState &state = selectedPlayer().state();
	sprite.image = bank.images[frame.image].get();
	sprite.x = add(add(subtract(x(), view.x), state.movementX), 320 + int32(frame.x));
	sprite.y = add(add(subtract(view.y, y()), -int32(frame.y)), 240 - int32(state.movementY));
	sprite.depth = depth() + 1;
	return true;
}

PropObject::PropObject(const SceneObjectRecord &record, const SceneGroup &group,
                             const SceneData &scene, uint32 now)
	: SceneObject(record, group, scene) {
	if (record.kind != 3 || record.parameters.size() < 2 || record.bankIndices.empty())
		error("Tesoro: Invalid prop object %s.", record.name.c_str());
	for (uint i = 0; i < record.bankIndices.size(); ++i)
		if (scene.banks[record.bankIndices[i]]->isFont)
			error("Tesoro: Invalid prop bank for %s.", record.name.c_str());
	addFlags(kGrouped);
	show(!scene.banks[record.bankIndices[0]]->images.empty());
	uint first = record.parameters[0], mode = record.parameters[1];
	if (mode <= 3) {
		if (mode == 0)
			show(false);
		if (!play(0, now, first, mode <= 1 ? int(first) : -1, mode == 3))
			error("Tesoro: Invalid initial animation for %s.", record.name.c_str());
		Animation &bank = *scene.banks[record.bankIndices[0]];
		if (mode <= 1 && first < bank.frames.size()) {
			bank.frames[first].dx = 0;
			bank.frames[first].dy = 0;
		}
	}
}

bool SceneObject::frameSprite(const SceneView &view, SceneSprite &sprite) const {
	if (_players.empty())
		return false;
	const AnimationPlayer &player = selectedPlayer();
	const Frame *frame = player.currentFrame();
	if ((flags() & (kActive | kShown)) != (kActive | kShown) || !frame || !player.currentImage())
		return false;
	sprite.image = player.currentImage();
	sprite.x = add(add(subtract(x(), view.x), player.state().movementX), 320 + int32(frame->x));
	sprite.y = add(add(subtract(view.y, y()), -int32(frame->y)), 240 - int32(player.state().movementY));
	sprite.depth = depth();
	return true;
}

bool PropObject::sprite(const SceneView &view, SceneSprite &sprite) const {
	return frameSprite(view, sprite);
}

InteractiveObject::InteractiveObject(const SceneObjectRecord &record, const SceneGroup &group, const SceneData &scene, uint32 now, uint parameterOffset, uint variableSize)
	: SceneObject(record, group, scene) {
	allocateVariables(variableSize);
	if (record.parameters.size() <= parameterOffset)
		error("Tesoro: Invalid interactive object %s.", record.name.c_str());
	if (!record.bankIndices.empty()) {
		// shown when its first frame has an image
		show(selectedPlayer().currentImage() != nullptr);
		const uint first = record.parameters[parameterOffset];
		if (!play(0, now, first, int(first), false))
			error("Tesoro: Invalid initial animation for %s.", record.name.c_str());
	}
	// 0x40 is set by the original too; its meaning is not known yet.
	addFlags(kGrouped | kCollidable | kSelectable | 0x40);

	// Where Jim stands to use the object and which way he faces, the name it has in the dialogue
	// tables (empty for none) and the lines its verbs answer with, six of 12 bytes each from 0x13 on.
	const Bytes &p = record.parameters;
	const uint base = parameterOffset;
	auto string = [&](uint at) {
		Common::String text;
		for (uint i = at; i < at + 12 && i < p.size() && p[i]; ++i)
			text += char(p[i]);
		return text;
	};
	if (p.size() >= base + 7) {
		_approachX = add(record.x, int16(p[base + 2] | (p[base + 3] << 8)));
		_approachY = add(record.y, int16(p[base + 4] | (p[base + 5] << 8)));
		_approachDirection = p[base + 6] + 1;
		_textName = string(base + 7);
		static const uint kVerbOfText[6] = {0, 1, 3, 4, 5, 2};
		for (uint k = 0; k < 6; ++k)
			_verbTexts[kVerbOfText[k]] = string(base + 0x13 + 12 * k);
	}
}

int InteractiveObject::onHover(PointerHost &host) {
	// The text the object has in the dialogue tables, else its name in the scene file.
	const TextEntry *entry = _textName.empty() ? nullptr : host.findText(_textName);
	host.setPointerText(entry ? entry->text : name());
	return 1;
}

void InteractiveObject::onUnhover(PointerHost &host) {
	host.setPointerText(Common::String());
}

int InteractiveObject::onLeftClick(PointerHost &host, int frame) {
	_verb = frame;
	_host = &host;
	host.walkJim(_approachX, _approachY, this);
	return 1;
}

void InteractiveObject::actionDone() {
	arrive();
}

void InteractiveObject::arrive() {
	// Jim has arrived: he turns to the object and the verb runs.
	if (!_host)
		return;
	_host->turnJim(_approachDirection);
	runVerb(_verb);
}

int InteractiveObject::answerVerb(int verb, const char *genericLine) {
	const TextEntry *entry = nullptr;
	if (!_verbTexts[verb].empty())
		entry = _host->findText(_verbTexts[verb]);
	else if (*genericLine)
		entry = _host->findText(genericLine);
	else
		return 0;
	if (!entry)
		error("Tesoro: %s has no text for verb %d.", name().c_str(), verb);
	_host->jimSays(*entry);
	return 3;
}

int InteractiveObject::runVerb(int verb) {
	// Verb 0 answers only with a line of the object's own. The others fall back to a line that
	// is the same for every object. Verbs 5 and 6 do nothing by default. Objects that answer a
	// verb in their own way (puzzles, doors) override this in the original; those are not ported.
	switch (verb) {
	case 0:
		return answerVerb(0, "");
	case 1:
		return answerVerb(1, "OBJ125T03");
	case 2:
		return answerVerb(2, "OBJ096T07");
	case 3:
		return answerVerb(3, "OBJ106T05");
	case 4:
		return answerVerb(4, "OBJ106T05");
	case 5:
	case 6:
		return 0;
	}
	// Verbs 7 and up use an inventory item on the object, which does not exist yet.
	warning("Tesoro: using inventory item %d on %s does nothing yet.", verb - 7, name().c_str());
	return 0;
}

bool InteractiveObject::sprite(const SceneView &view, SceneSprite &sprite) const {
	return frameSprite(view, sprite);
}

// How many variables each kind of character owns.
static uint characterVariables(uint characterClass) {
	static const byte sizes[17] = {4, 14, 4, 0, 10, 3, 6, 4, 3, 0, 1, 1, 0, 15, 2, 11, 7};
	return characterClass < ARRAYSIZE(sizes) ? sizes[characterClass] : 0;
}

CharacterObject::CharacterObject(const SceneObjectRecord &record, const SceneGroup &group,
                                 const SceneData &scene, uint32 now)
	: InteractiveObject(record, group, scene, now, 4, record.parameters.size() > 5 ? characterVariables(record.parameters[5]) : 0) {
	_class = record.parameters.size() > 5 ? record.parameters[5] : 0;
	show(true);
	// The text of what a character says is coloured and wrapped as its record says (the first
	// four bytes). Its idle bank is the last one whose name starts with MIR (as in "mirar"), and
	// the banks that show it talking are the ones that start with HAB (as in "hablar").
	TalkConfig config;
	config.colour = record.parameters[0] | (record.parameters[1] << 8);
	config.width = record.parameters[2] | (record.parameters[3] << 8);
	for (uint i = 0; i < animationCount(); ++i) {
		if (animationName(i).hasPrefix("MIR"))
			_idleBank = i;
		else if (animationName(i).hasPrefix("HAB"))
			config.banks.push_back(i);
	}
	setTalkConfig(config);
}

int CharacterObject::runVerb(int verb) {
	PointerHost &game = *host();
	if (verb == 2 && _class == 0) {
		// The captain: what he says depends on how far two other characters have got.
		SceneObject *nudero = game.levelScene(2)->find("NUDERO");
		SceneObject *recruiter = game.levelScene(1)->find("RECLUTADOR");
		if (!nudero || !nudero->variables() || !recruiter || !recruiter->variables())
			error("Tesoro: The captain's conversation needs the tavern and the harbour.");
		variables()[1] = nudero->variables()[4] != 0;
		variables()[2] = recruiter->variables()[3];
		game.startObjectScript("FCAPITAN", "hablar", variables(), *scene());
		return 1;
	}
	if (verb == 2 && _class == 1) {
		// The recruiter: his conversation moves on once the captain has had his say and once Jim
		// has the half doubloon.
		SceneObject *captain = game.levelScene(0)->find("CAPITAN");
		SceneObject *doubloon = game.interfaceScene()->find("IHALFDOBLON");
		if (!captain || !captain->variables() || !doubloon)
			error("Tesoro: The recruiter's conversation needs the office and the inventory.");
		if (variables()[0] < 4 && captain->variables()[0] != 0)
			variables()[0] = 4;
		if (variables()[0] < 6 && doubloon->isActive())
			variables()[0] = 6;
		game.startObjectScript("FRECLUTADOR", "hablar", variables(), *scene());
		return 1;
	}
	return InteractiveObject::runVerb(verb);
}

void CharacterObject::turn(int direction, uint32 now) {
	if (!play(_idleBank, now, direction - 1, direction - 1, false))
		error("Tesoro: A character cannot face direction %d.", direction);
}

FloorObject::FloorObject(const SceneObjectRecord &record, const SceneGroup &group,
                         const SceneData &scene, uint32 now)
	: SceneObject(record, group, scene) {
	if (record.parameters.size() < 13 || record.bankIndices.empty())
		error("Tesoro: Invalid floor object %s.", record.name.c_str());
	const uint first = record.parameters[0];
	if (!play(0, now, first, int(first), false))
		error("Tesoro: Invalid floor frame for %s.", record.name.c_str());
	Animation &bank = *scene.banks[record.bankIndices[0]];
	bank.frames[first].dx = 0;
	bank.frames[first].dy = 0;
	// The floor is built on the same base as the interactive objects (0x40ecb0).
	addFlags(kGrouped | kCollidable | kSelectable | 0x40);
}

int FloorObject::scaleAt(int32 x, int32 y) const {
	const Frame *frame = selectedPlayer().currentFrame();
	if (!frame || frame->type != 4)
		return 256;
	// The outline is walked edge by edge. The first edge that is crossed above or below the
	// point gives one scale, the next edge on the other side of the point another one, and
	// the result is the blend of the two by the point's height between those edges.
	const Common::Array<FloorPoint> &points = frame->floorPoints;
	const int32 px = x - this->x(), py = y - this->y();
	bool found = false, below = false;
	int32 firstScale = 256, firstY = 0;
	for (uint i = 0; i + 1 < points.size(); ++i) {
		const FloorPoint *left = &points[i], *right = &points[i + 1];
		if (left->x >= right->x)
			SWAP(left, right);
		if (px < left->x || px > right->x || left->x == right->x)
			continue;
		const int32 width = right->x - left->x;
		const int32 edgeY = ((right->y - left->y) * (px - left->x)) / width + left->y;
		const int32 edgeScale = (int32(right->scale) - int32(left->scale)) * (px - left->x) / width + left->scale;
		if (!found) {
			found = true;
			firstScale = edgeScale;
			below = py < edgeY;
			firstY = edgeY;
			continue;
		}
		if (below ? edgeY <= py : py <= edgeY) {
			if (firstY == edgeY)
				return edgeScale;
			if (!below)
				return ((py - firstY) * (edgeScale - firstScale)) / (edgeY - firstY) + firstScale;
			return ((py - edgeY) * (firstScale - edgeScale)) / (firstY - edgeY) + edgeScale;
		}
	}
	return found ? firstScale : 256;
}

// Stands in for a kind that has no SceneObject subclass yet: it keeps the object
// in the scene but draws nothing and has no behaviour.
class UnportedObject : public SceneObject {
public:
	UnportedObject(const SceneObjectRecord &record, const SceneGroup &group, const SceneData &scene)
		: SceneObject(record, group, scene) {}
};

SceneObject *Scene::createObject(const SceneObjectRecord &record, const SceneGroup &group,
                                 const SceneData &scene, uint32 now) {
	const char *limits = nullptr;
	SceneObject *object = nullptr;
	switch (record.kind) {
	case 3:
		return new PropObject(record, group, scene, now);
	case 7:
		return new FloorObject(record, group, scene, now);
	case 6:
		object = new DoorObject(record, group, scene, now, record.parameters.size() > 1 ? record.parameters[1] : 0);
		limits = "it works, but only some of the doors' special behaviour is ported";
		break;
	case 12:
		object = new ScriptedObject(record, group, scene, now, record.parameters.size() > 1 ? record.parameters[1] : 0);
		limits = "it works, but only some of the objects' special behaviour is ported";
		break;
	case 2:
		return new IconGroupObject(record, group, scene);
	case 10:
		return new CursorObject(record, group, scene, now);
	case 11:
		return new ControlIconObject(record, group, scene, now);
	case 5: // characters
		object = new CharacterObject(record, group, scene, now);
		limits = "it is drawn, but its behaviour is not implemented";
		break;
	case 9: // interactive objects
	case 13:
	case 17:
	case 20:
		object = new InteractiveObject(record, group, scene, now);
		limits = "it is drawn, but its behaviour is not implemented";
		break;
	default:
		object = new UnportedObject(record, group, scene);
		limits = "it is kept but inert";
		break;
	}
	if (record.kind >= 32 || !(_warnedKinds & (1u << record.kind))) {
		warning("Tesoro: object kind %u (first seen: %s) is not implemented; %s.",
		        record.kind, record.name.c_str(), limits);
		if (record.kind < 32)
			_warnedKinds |= 1u << record.kind;
	}
	return object;
}

Scene::Scene(const SceneData &scene, uint32 now) {
	// Rooms without a floor of their own (the map and the board games, for instance) have no entry
	// point yet; the camera then stays where it is.
	if (findRoomEntry(scene, _entry)) {
		_camera.setBounds(_entry.bounds);
		_camera.setPosition(_entry.x, _entry.y);
	} else {
		warning("Tesoro: A scene has no room with a floor to enter.");
	}
	for (uint i = 0; i < scene.groups.size(); ++i) {
		const SceneGroup &source = scene.groups[i];
		Group group;
		group.parallax = source.parallax;
		for (uint j = 0; j < source.objects.size(); ++j) {
			const SceneObjectRecord &record = source.objects[j];
			Common::SharedPtr<SceneObject> object(createObject(record, source, scene, now));
			_objects.push_back(object);
			object->setScene(this);
			object->setParallax(source.parallax);
			object->setGroup(i);
			if (!(object->flags() & SceneObject::kGrouped))
				continue;
			uint at = 0;
			while (at < group.objects.size() && group.objects[at]->x() >= record.x)
				++at;
			group.objects.insert_at(at, object);
		}
		_groups.push_back(group);
	}
	// The room names its floor in the first twelve bytes of its record.
	const SceneObjectRecord *room = findRoomRecord(scene);
	if (room && room->parameters.size() >= 12 && room->parameters[0]) {
		Common::String name;
		for (uint i = 0; i < 12 && room->parameters[i]; ++i)
			name += char(room->parameters[i]);
		const SceneObject *floor = find(name);
		if (floor && floor->kind() == 7)
			_floor = static_cast<const FloorObject *>(floor);
	}
}

void Scene::addObject(const Common::SharedPtr<SceneObject> &object, uint group) {
	if (group >= _groups.size())
		error("Tesoro: The scene has no group %u.", group);
	_objects.push_back(object);
	object->setScene(this);
	object->setParallax(_groups[group].parallax);
	object->setGroup(group);
	Common::Array<Common::SharedPtr<SceneObject>> &list = _groups[group].objects;
	uint at = 0;
	while (at < list.size() && list[at]->x() >= object->x())
		++at;
	list.insert_at(at, object);
}

void Scene::removeObject(SceneObject *object) {
	for (uint i = 0; i < _moved.size(); ++i)
		if (_moved[i] == object) {
			_moved.remove_at(i);
			break;
		}
	for (uint i = 0; i < _pairs.size();)
		if (_pairs[i].first == object || _pairs[i].second == object)
			_pairs.remove_at(i);
		else
			++i;
	for (uint i = 0; i < _objects.size(); ++i)
		if (_objects[i].get() == object) {
			_objects.remove_at(i);
			break;
		}
	for (uint g = 0; g < _groups.size(); ++g) {
		Common::Array<Common::SharedPtr<SceneObject>> &list = _groups[g].objects;
		for (uint i = 0; i < list.size(); ++i)
			if (list[i].get() == object) {
				list.remove_at(i);
				break;
			}
	}
}

SceneObject *Scene::find(uint64 id) const {
	for (uint i = 0; i < _objects.size(); ++i)
		if (_objects[i]->nameId() == id)
			return _objects[i].get();
	return nullptr;
}

SceneObject *Scene::send(uint32 now, const Common::String &target, const SceneMessage &message) {
	return deliver(now, find(target), target.c_str(), message);
}

SceneObject *Scene::send(uint32 now, uint64 target, const SceneMessage &message) {
	return deliver(now, find(target), "(by name hash)", message);
}

SceneObject *Scene::deliver(uint32 now, SceneObject *object, const char *name, const SceneMessage &message) {
	if (!object)
		error("Tesoro: Message sent to unknown object %s.", name);
	switch (message.kind) {
	case SceneMessage::kActivate:
		object->setActive(message.on);
		break;
	case SceneMessage::kShow:
		object->show(message.on);
		break;
	case SceneMessage::kPlay: {
		const bool stop = message.mode == SceneMessage::kStop;
		if (!object->play(message.bank, now, message.first, stop ? int(message.first) : -1,
		                  message.mode == SceneMessage::kLoop))
			error("Tesoro: Invalid play message for %s.", name);
		if (message.mode == SceneMessage::kPlayAndWait)
			return object;
		break;
	}
	}
	return nullptr;
}

void SceneObject::updateOverlap(uint32 now) {
	OverlapBody &body = _overlap;
	body.x = _x;
	body.y = _y;
	body.depth = _depth;
	body.isJim = isPlayer();
	body.fromX = body.toX = _x;
	body.fromY = body.toY = _y;
	for (int i = 0; i < 4; ++i)
		body.box[i] = 0;
	body.polygon = OverlapPolygon();
	if (_players.empty())
		return;

	// Where the object is before and after its animation advances: its position plus what the
	// animation moves it by. If it was moved, it is going to be at the position it was moved to.
	AnimationPlayer &player = *_players[_currentBank];
	body.fromX = add(_x, player.state().movementX);
	body.fromY = add(_y, player.state().movementY);
	player.update(now);
	if (dropsMovement())
		player.clearMovement();
	int32 pendingX, pendingY;
	pendingPosition(pendingX, pendingY);
	body.toX = add(pendingX, player.state().movementX);
	body.toY = add(pendingY, player.state().movementY);

	const Frame *frame = player.currentFrame();
	if (!frame)
		return;
	body.box[0] = frame->hitX;
	body.box[1] = frame->hitY;
	body.box[2] = add(frame->hitX, int16(frame->hitW));
	body.box[3] = add(frame->hitY, -int32(int16(frame->hitH)));
	adjustBox(body.box);

	// Frames of types 2 and 4 carry an outline; the others have none.
	if (frame->type == 2 && !frame->pathPoints.empty()) {
		body.polygon = OverlapPolygon::fromPoints(reinterpret_cast<const int16 *>(frame->pathPoints.data()),
		                                          frame->pathPoints.size());
	} else if (frame->type == 4 && !frame->floorPoints.empty()) {
		_outline.resize(frame->floorPoints.size() * 2);
		for (uint i = 0; i < frame->floorPoints.size(); ++i) {
			_outline[i * 2] = frame->floorPoints[i].x;
			_outline[i * 2 + 1] = frame->floorPoints[i].y;
		}
		body.polygon = OverlapPolygon::fromPoints(_outline.data(), frame->floorPoints.size());
	}
}

PointerProbe::PointerProbe()
	: SceneObject(0xfffe, encodeName("PUNTERO"), Common::Array<Common::SharedPtr<Animation>>(), 0, 0, 0) {
	addFlags(kShown | kCollidable);
}

void PointerProbe::updateOverlap(uint32) {
	// The probe is a point: no box, no outline, and it jumps to where it is put.
	OverlapBody &body = _overlap;
	body = OverlapBody();
	body.x = body.fromX = body.toX = x();
	body.y = body.fromY = body.toY = y();
	body.depth = depth();
}

void Scene::objectMoved(SceneObject *object) {
	if (!(object->flags() & SceneObject::kCollidable))
		return;
	for (uint i = 0; i < _moved.size(); ++i)
		if (_moved[i] == object)
			return;
	_moved.push_back(object);
}

void Scene::queuePair(SceneObject &first, SceneObject &second) {
	OverlapResult result;
	if (!testOverlap(first.overlapBody(), second.overlapBody(), result))
		return;
	// Kept in order of contact, then of depth; a pair equal to one that is queued goes after it.
	OverlapPair pair = {&first, &second, result.overlap, result.priority};
	uint at = 0;
	while (at < _pairs.size() &&
	       !(pair.overlap < _pairs[at].overlap ||
	         (pair.overlap == _pairs[at].overlap && int16(pair.priority) < int16(_pairs[at].priority))))
		++at;
	_pairs.insert_at(at, pair);
}

void Scene::debugShapes(Common::Array<SceneDebugShape> &shapes) const {
	for (uint g = 0; g < _groups.size(); ++g) {
		const SceneView view = _camera.groupView(_groups[g].parallax);
		auto toScreen = [&](int32 worldX, int32 worldY) {
			return Common::Point(subtract(add(worldX, 320), view.x), subtract(add(240, view.y), worldY));
		};
		for (uint i = 0; i < _groups[g].objects.size(); ++i) {
			const SceneObject &object = *_groups[g].objects[i];
			const uint32 flags = object.flags();
			if (!(flags & SceneObject::kCollidable) || !(flags & SceneObject::kActive))
				continue;
			const OverlapBody &body = object.overlapBody();
			SceneDebugShape shape;
			shape.object = &object;
			const Common::Point topLeft = toScreen(add(body.fromX, body.box[0]), add(body.fromY, body.box[1]));
			const Common::Point bottomRight = toScreen(add(body.fromX, body.box[2]), add(body.fromY, body.box[3]));
			shape.box = Common::Rect(topLeft.x, topLeft.y, bottomRight.x + 1, bottomRight.y + 1);
			for (uint p = 0; p < body.polygon.count; ++p)
				shape.outline.push_back(toScreen(add(body.x, body.polygon.points[p * 2]),
				                                 add(body.y, body.polygon.points[p * 2 + 1])));
			shapes.push_back(shape);
		}
	}
}

void Scene::resolveOverlaps(uint32 now) {
	_pairs.clear();

	// What the moved objects and their neighbours are made of this frame.
	Common::Array<SceneObject *> prepared;
	auto prepare = [&](SceneObject *object) {
		for (uint i = 0; i < prepared.size(); ++i)
			if (prepared[i] == object)
				return;
		prepared.push_back(object);
		object->updateOverlap(now);
	};
	for (uint i = 0; i < _moved.size(); ++i)
		prepare(_moved[i]);
	for (uint i = 0; i < _moved.size(); ++i) {
		const Common::Array<Common::SharedPtr<SceneObject>> &list = _groups[_moved[i]->group()].objects;
		for (uint j = 0; j < list.size(); ++j) {
			const uint32 flags = list[j]->flags();
			if ((flags & SceneObject::kCollidable) && (flags & SceneObject::kActive))
				prepare(list[j].get());
		}
	}

	for (uint i = 0; i < _moved.size(); ++i) {
		SceneObject &object = *_moved[i];
		if (!(object.flags() & SceneObject::kCollidable))
			continue;
		const Common::Array<Common::SharedPtr<SceneObject>> &list = _groups[object.group()].objects;
		for (uint j = 0; j < list.size(); ++j) {
			SceneObject &other = *list[j];
			const uint32 flags = other.flags();
			if (&other == &object || !(flags & SceneObject::kCollidable) || !(flags & SceneObject::kActive))
				continue;
			if (object.inFront(other))
				queuePair(object, other);
			else if (other.inFront(object))
				queuePair(other, object);
		}
	}

	// Delivering a pair can move or remove objects, so work from a copy.
	Common::Array<OverlapPair> pairs = _pairs;
	_pairs.clear();
	for (uint i = 0; i < pairs.size(); ++i)
		pairs[i].first->onOverlap(*pairs[i].second, pairs[i].overlap);

	for (uint i = 0; i < _moved.size(); ++i)
		_moved[i]->commitPosition();
	_moved.clear();
}

void Scene::appendSprites(uint32 now, Common::RandomSource &random, Common::Array<SceneSprite> &sprites) {
	_camera.update(now);
	for (uint i = 0; i < _groups.size(); ++i) {
		const Group &group = _groups[i];
		SceneView view = _camera.groupView(group.parallax);
		// for each object in the parallax group
		for (uint j = 0; j < group.objects.size(); ++j) {
			SceneObject &object = *group.objects[j];
			if ((object.flags() & (SceneObject::kActive | SceneObject::kShown)) !=
			    (SceneObject::kActive | SceneObject::kShown))
				continue;
			// prepare frame
			object.prepareFrame(now, random);
			SceneSprite sprite;
			if (object.backSprite(view, sprite))
				sprites.push_back(sprite);
			if (object.sprite(view, sprite))
				sprites.push_back(sprite);
		}
	}
}

} // End of namespace Tesoro
