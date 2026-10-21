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

namespace Tesoro {
namespace {

int32 add(int32 a, int32 b) {
	return int32(uint32(a) + uint32(b));
}

int32 subtract(int32 a, int32 b) {
	return int32(uint32(a) - uint32(b));
}

} // namespace

bool SceneCamera::setPosition(int32 x, int32 y) {
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

SceneView SceneCamera::groupView(int32 parallax) const {
	if (parallax <= 0)
		error("Tesoro: Invalid scene parallax divisor.");
	SceneView view;
	view.x = int32(uint32(_position.x) * 300) / parallax;
	view.y = int32(uint32(_position.y) * 300) / parallax;
	return view;
}

bool findRoomEntry(const SceneData &scene, RoomEntry &entry) {
	const SceneObjectRecord *room = nullptr;
	for (uint i = 0; i < scene.groups.size(); ++i)
		for (uint j = 0; j < scene.groups[i].objects.size(); ++j)
			if (scene.groups[i].objects[j].kind == 1)
				room = &scene.groups[i].objects[j];
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

SceneObject::SceneObject(const SceneObjectRecord &record, const SceneGroup &group,
                           const SceneData &scene, uint32 now)
	: _kind(record.kind), _nameId(encodeName(record.name)), _x(record.x), _y(record.y), _depth(uint16(record.depth + group.depthBase + 5)) {
	if ((record.kind != 3 && record.kind != 5 && record.kind != 6 && record.kind != 9) ||
	    record.parameters.size() < (record.kind == 5 ? 6U : 2U) || record.bankIndices.empty())
		error("Tesoro: Invalid drawable object %s.", record.name.c_str());
	for (uint i = 0; i < record.bankIndices.size(); ++i) {
		uint index = record.bankIndices[i];
		if (index >= scene.banks.size() || scene.banks[index]->isFont)
			error("Tesoro: Invalid drawable bank for %s.", record.name.c_str());
		_banks.push_back(scene.banks[index]);
		_players.push_back(Common::SharedPtr<AnimationPlayer>(new AnimationPlayer(*_banks.back())));
	}
	if (record.kind == 3) {
		show(!_banks[0]->images.empty());
		uint first = record.parameters[0], mode = record.parameters[1];
		if (mode <= 3) {
			if (mode == 0)
				show(false);
			if (!play(0, now, first, mode <= 1 ? int(first) : -1, mode == 3))
				error("Tesoro: Invalid initial animation for %s.", record.name.c_str());
			if (mode <= 1 && first < _banks[0]->frames.size()) {
				_banks[0]->frames[first].dx = 0;
				_banks[0]->frames[first].dy = 0;
			}
		}
	} else {
		_flags |= 0x508;
		show(!_banks[0]->frames.empty() && _banks[0]->frames[0].image >= 0);
		uint first = record.parameters[record.kind == 5 ? 4 : 0];
		if (!play(0, now, first, first))
			error("Tesoro: Invalid initial frame for %s.", record.name.c_str());
		if (record.kind == 5) {
			show(true);
			uint subtype = record.parameters[5];
			if (subtype == 1 || subtype == 2 || subtype == 4 || subtype == 6)
				if (!play(0, now, 0, -1, true))
					error("Tesoro: Invalid character loop for %s.", record.name.c_str());
		}
	}
}

void SceneObject::setActive(bool active) {
	_flags = (_flags & ~uint32(kActive)) | (active ? kActive : 0);
}

void SceneObject::show(bool shown) {
	_flags = (_flags & ~uint32(kShown)) | (shown ? kShown : 0);
}

bool SceneObject::play(uint bank, uint32 now, uint16 first, int last,
                         bool loop, bool cacheDuration) {
	if (bank >= _players.size() || !_players[bank]->play(now, first, last, loop, cacheDuration))
		return false;
	_selected = bank;
	return true;
}

void SceneObject::prepareFrame(uint32 now) {
	_players[_selected]->update(now);
	if (_kind == 5)
		_players[_selected]->clearMovement();
	const Frame *frame = _players[_selected]->currentFrame();
	if (!frame)
		return;
	_hitBounds.left = frame->hitX;
	_hitBounds.top = frame->hitY;
	_hitBounds.right = add(frame->hitX, int16(frame->hitW));
	_hitBounds.bottom = add(frame->hitY, -int32(int16(frame->hitH)));
}

bool SceneObject::sprite(const SceneView &view, SceneSprite &sprite) const {
	const AnimationPlayer &player = *_players[_selected];
	const Frame *frame = player.currentFrame();
	if ((_flags & (kActive | kShown)) != (kActive | kShown) || !frame || !player.currentImage())
		return false;
	sprite.image = player.currentImage();
	sprite.x = add(add(subtract(_x, view.x), player.state().movementX), 320 + int32(frame->x));
	sprite.y = add(add(subtract(view.y, _y), -int32(frame->y)), 240 - int32(player.state().movementY));
	sprite.depth = _depth;
	return true;
}

SceneObjects::SceneObjects(const SceneData &scene, uint32 now) {
	for (uint i = 0; i < scene.groups.size(); ++i) {
		const SceneGroup &source = scene.groups[i];
		Group group;
		group.parallax = source.parallax;
		for (uint j = 0; j < source.objects.size(); ++j) {
			const SceneObjectRecord &record = source.objects[j];
			if (record.kind != 3 && record.kind != 5 && record.kind != 6 && record.kind != 9)
				continue;
			Common::SharedPtr<SceneObject> object(new SceneObject(record, source, scene, now));
			_objects.push_back(object);
			uint at = 0;
			while (at < group.objects.size() && group.objects[at]->x() >= record.x)
				++at;
			group.objects.insert_at(at, object);
		}
		_groups.push_back(group);
	}
}

void SceneObjects::appendSprites(uint32 now, const SceneCamera &camera, Common::Array<SceneSprite> &sprites) {
	for (uint i = 0; i < _groups.size(); ++i) {
		const Group &group = _groups[i];
		SceneView view = camera.groupView(group.parallax);
		for (uint j = 0; j < group.objects.size(); ++j) {
			SceneObject &object = *group.objects[j];
			if ((object.flags() & (SceneObject::kActive | SceneObject::kShown)) !=
			    (SceneObject::kActive | SceneObject::kShown))
				continue;
			object.prepareFrame(now);
			SceneSprite sprite;
			if (object.sprite(view, sprite))
				sprites.push_back(sprite);
		}
	}
}

SceneObject *SceneObjects::find(const Common::String &name) const {
	uint64 id = encodeName(name);
	for (uint i = 0; i < _objects.size(); ++i)
		if (_objects[i]->nameId() == id)
			return _objects[i].get();
	return nullptr;
}

} // namespace Tesoro
