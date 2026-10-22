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
};

class SceneryObject {
public:
	enum Flags { kActive = 2, kShown = 4, kHasAnimation = 16 };
	SceneryObject(const SceneObjectRecord &record, const SceneGroup &group,
	              const SceneData &scene, uint32 now);
	void setActive(bool active);
	void show(bool shown);
	bool play(uint bank, uint32 now, uint16 first = 0, int last = -1,
	          bool loop = false, bool cacheDuration = false);
	void prepareFrame(uint32 now);
	bool sprite(const SceneView &view, SceneSprite &sprite) const;
	const AnimationState &animationState() const { return _players[_selected]->state(); }
	uint32 flags() const { return _flags; }
	uint16 depth() const { return _depth; }
	int32 x() const { return _x; }
	const SceneBounds &hitBounds() const { return _hitBounds; }

private:
	SceneryObject(const SceneryObject &) = delete;
	SceneryObject &operator=(const SceneryObject &) = delete;

	Common::Array<Common::SharedPtr<Animation>> _banks;
	Common::Array<Common::SharedPtr<AnimationPlayer>> _players;
	uint _selected = 0;
	uint32 _flags = kActive | kHasAnimation;
	int32 _x, _y;
	uint16 _depth;
	SceneBounds _hitBounds;
};

// The scenery portion of a scene; actor and interactive-object scheduling is separate.
class SceneScenery {
public:
	SceneScenery(const SceneData &scene, uint32 now);
	void appendSprites(uint32 now, const SceneCamera &camera, Common::Array<SceneSprite> &sprites);

private:
	struct Group {
		int32 parallax;
		Common::Array<Common::SharedPtr<SceneryObject>> objects;
	};
	Common::Array<Group> _groups;
};

} // namespace Tesoro

#endif
