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

#ifndef TESORO_SCENE_H
#define TESORO_SCENE_H

#include "tesoro/animation.h"
#include "tesoro/resources.h"

namespace Tesoro {

// World coordinates have positive Y upwards.
struct SceneBounds {
	int32 left = 0, top = 0, right = 0, bottom = 0;
};

struct SceneView {
	int32 x = 0, y = 0;
};

class SceneCamera {
public:
	void setBounds(const SceneBounds &bounds) { _bounds = bounds; }
	bool setPosition(int32 x, int32 y);
	SceneView groupView(int32 parallax) const;
	const SceneView &position() const { return _position; }

private:
	SceneBounds _bounds;
	SceneView _position;
};

struct RoomEntry {
	uint floorGroup = 0, floorObject = 0;
	int32 x = 0, y = 0;
	byte musicCue = 0;
	SceneBounds bounds;
};

// Resolve the floor origin for default entry; scene transitions can supply another position.
bool findRoomEntry(const SceneData &scene, RoomEntry &entry);

struct SceneSprite {
	const Image *image = nullptr;
	int32 x = 0, y = 0;
	uint16 depth = 0;
	// 0 draws the image as it is; otherwise its scale in 256ths (256 is full size).
	int32 scale = 0;
};

// Base of every object a scene owns, whatever its kind.
class SceneObject {
public:
	enum Flags { kActive = 2, kShown = 4, kHasAnimation = 16, kGrouped = 0x100 };
	virtual ~SceneObject() {}

	void setActive(bool active);
	void show(bool shown);
	bool play(uint bank, uint32 now, uint16 first = 0, int last = -1, bool loop = false, bool cacheDuration = false);

	// Advances the selected bank and refreshes the hit bounds.
	virtual void prepareFrame(uint32 now);
	// Fills in the sprite this object submits for the frame; false when it draws nothing.
	virtual bool sprite(const SceneView &view, SceneSprite &sprite) const;

	const AnimationState &animationState() const { assert(!_players.empty()); return _players[_selected]->state(); }
	uint32 flags() const { return _flags; }
	uint32 kind() const { return _kind; }
	uint64 nameId() const { return _nameId; }
	uint16 depth() const { return _depth; }
	int32 x() const { return _x; }
	int32 y() const { return _y; }
	const SceneBounds &hitBounds() const { return _hitBounds; }
	// True while the selected bank is still playing an animation.
	bool isPlaying() const { return !_players.empty() && (_players[_selected]->state().flags & AnimationPlayer::kPlaying); }

protected:
	SceneObject(const SceneObjectRecord &record, const SceneGroup &group, const SceneData &scene);

	void addFlags(uint32 flags) { _flags |= flags; }
	// Where the current frame is drawn, shared by every object that shows its animation.
	bool frameSprite(const SceneView &view, SceneSprite &sprite) const;
	const AnimationPlayer &selectedPlayer() const { assert(!_players.empty()); return *_players[_selected]; }

private:
	SceneObject(const SceneObject &) = delete;
	SceneObject &operator=(const SceneObject &) = delete;

	Common::Array<Common::SharedPtr<Animation>> _banks;
	Common::Array<Common::SharedPtr<AnimationPlayer>> _players;
	uint _selected = 0;
	uint32 _kind;
	uint64 _nameId;
	uint32 _flags = kActive | kHasAnimation;
	int32 _x, _y;
	uint16 _depth;
	SceneBounds _hitBounds;
};

// Kind 3: backgrounds and ambient animation.
class PropObject : public SceneObject {
public:
	PropObject(const SceneObjectRecord &record, const SceneGroup &group, const SceneData &scene, uint32 now);
	bool sprite(const SceneView &view, SceneSprite &sprite) const override;
};

// The object messages a script's sendmessage can carry (script op 9).
struct SceneMessage {
	enum Kind { kActivate, kShow, kPlay };
	// How a play message ends: stop on `first`, run to the end of the sequence, loop,
	// or run to the end while the sender waits for it.
	enum PlayMode { kStop, kPlayOnce, kLoop, kPlayAndWait };

	Kind kind = kActivate;
	bool on = false;             // kActivate, kShow
	uint bank = 0;               // kPlay
	uint16 first = 0;            // kPlay
	PlayMode mode = kPlayOnce;   // kPlay
};

// Kinds 6, 9, 12, 13, 17 and 20: objects the player can interact with.
class InteractiveObject : public SceneObject {
public:
	InteractiveObject(const SceneObjectRecord &record, const SceneGroup &group, const SceneData &scene, uint32 now, uint parameterOffset = 0);
	bool sprite(const SceneView &view, SceneSprite &sprite) const override;
};

// Kind 5: the characters of the game.
class CharacterObject : public InteractiveObject {
public:
	CharacterObject(const SceneObjectRecord &record, const SceneGroup &group, const SceneData &scene, uint32 now);
};

// Kind 7: the walkable ground of a room. It is not drawn; its outline gives the scale of
// the characters that stand on it.
class FloorObject : public SceneObject {
public:
	FloorObject(const SceneObjectRecord &record, const SceneGroup &group,
	            const SceneData &scene, uint32 now);
	// The scale (256 is full size) of a character at a point of the room.
	int scaleAt(int32 x, int32 y) const;
};

// A loaded scene owns every object in its LVL. Kinds without a SceneObject subclass
// yet are built as inert placeholders (with a warning) so nothing is silently dropped.
class Scene {
public:
	Scene(const SceneData &scene, uint32 now);
	void appendSprites(uint32 now, const SceneCamera &camera, Common::Array<SceneSprite> &sprites);

	// Finds an object by its LVL name (compared by the original's 64-bit encoding).
	// If several objects share a name, the first in LVL order is returned.
	SceneObject *find(const Common::String &name) const { return find(encodeName(name)); }
	SceneObject *find(uint64 id) const;

	// Delivers a message to the named object. Returns the object when the sender has to
	// wait for its animation to finish (kPlayAndWait), otherwise nullptr.
	SceneObject *send(uint32 now, const Common::String &target, const SceneMessage &message);
	SceneObject *send(uint32 now, uint64 target, const SceneMessage &message);

	// The floor that the room names, if it has one.
	const FloorObject *floor() const { return _floor; }

private:
	struct Group {
		int32 parallax;
		Common::Array<Common::SharedPtr<SceneObject>> objects;
	};
	Common::Array<Group> _groups;
	Common::Array<Common::SharedPtr<SceneObject>> _objects;
	uint32 _warnedKinds = 0;
	const FloorObject *_floor = nullptr;

	SceneObject *deliver(uint32 now, SceneObject *object, const char *name, const SceneMessage &message);
	SceneObject *createObject(const SceneObjectRecord &record, const SceneGroup &group,  const SceneData &scene, uint32 now);
};

} // namespace Tesoro

#endif
