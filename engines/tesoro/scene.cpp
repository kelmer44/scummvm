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

SceneObject::SceneObject(const SceneObjectRecord &record, const SceneGroup &group, const SceneData &scene)
	: _kind(record.kind), _nameId(encodeName(record.name)), _x(record.x), _y(record.y),
	  _depth(uint16(record.depth + group.depthBase + 5)) {
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

bool SceneObject::play(uint bank, uint32 now, uint16 first, int last,
                       bool loop, bool cacheDuration) {
	if (bank >= _players.size() || !_players[bank]->play(now, first, last, loop, cacheDuration))
		return false;
	_selected = bank;
	return true;
}

void SceneObject::prepareFrame(uint32 now) {
	if (_players.empty())
		return;
	_players[_selected]->update(now);
	const Frame *frame = _players[_selected]->currentFrame();
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

InteractiveObject::InteractiveObject(const SceneObjectRecord &record, const SceneGroup &group, const SceneData &scene, uint32 now, uint parameterOffset)
	: SceneObject(record, group, scene) {
	if (record.parameters.size() <= parameterOffset)
		error("Tesoro: Invalid interactive object %s.", record.name.c_str());
	if (!record.bankIndices.empty()) {
		// shown when its first frame has an image
		show(selectedPlayer().currentImage() != nullptr);
		const uint first = record.parameters[parameterOffset];
		if (!play(0, now, first, int(first), false))
			error("Tesoro: Invalid initial animation for %s.", record.name.c_str());
	}
	addFlags(kGrouped);
}

bool InteractiveObject::sprite(const SceneView &view, SceneSprite &sprite) const {
	return frameSprite(view, sprite);
}

CharacterObject::CharacterObject(const SceneObjectRecord &record, const SceneGroup &group,
                                 const SceneData &scene, uint32 now)
	: InteractiveObject(record, group, scene, now, 4) {
	show(true);
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
	addFlags(kGrouped);
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
	case 5: // characters
		object = new CharacterObject(record, group, scene, now);
		limits = "it is drawn, but its behaviour is not implemented";
		break;
	case 6: // interactive objects
	case 9:
	case 12:
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
	const SceneObjectRecord *room = nullptr;
	for (uint i = 0; i < scene.groups.size(); ++i) {
		const SceneGroup &source = scene.groups[i];
		Group group;
		group.parallax = source.parallax;
		for (uint j = 0; j < source.objects.size(); ++j) {
			const SceneObjectRecord &record = source.objects[j];
			if (record.kind == 1)
				room = &record;
			Common::SharedPtr<SceneObject> object(createObject(record, source, scene, now));
			_objects.push_back(object);
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
	Common::Array<Common::SharedPtr<SceneObject>> &list = _groups[group].objects;
	uint at = 0;
	while (at < list.size() && list[at]->x() >= object->x())
		++at;
	list.insert_at(at, object);
}

void Scene::removeObject(SceneObject *object) {
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

void Scene::appendSprites(uint32 now, const SceneCamera &camera, Common::Array<SceneSprite> &sprites) {
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

} // End of namespace Tesoro
