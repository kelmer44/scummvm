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

#ifndef TESORO_JIM_H
#define TESORO_JIM_H

#include "tesoro/scriptexec.h"

namespace Tesoro {

// Main character. He is not part of any scene file: one object lives
// for the whole game and is placed into the room that is being played.
class JimObject : public SceneObject {
public:
	// The animation files of his banks, in the order that the constructor expects them.
	static uint bankCount();
	static const char *bankName(uint index);

	explicit JimObject(const Common::Array<Common::SharedPtr<Animation>> &banks);

	// Stands Jim at a point of the scene's floor. The engine adds him to a group of the
	// scene first.
	void place(Scene &scene, int32 x, int32 y, uint32 now);
	void leave();

	// Changes jim's direction
	void look(ScriptOp::Direction direction, uint32 now);
	void turn(int direction, uint32 now) override { look(ScriptOp::Direction(direction), now); }

	// Walks in a straight line to a point. The waiter, if any, is resumed on arrival or immediately if the room has no floor.
	void startWalk(int32 x, int32 y, Script *waiter, uint32 now);

	bool isWalking() const { return _walking; }

	// Jim's size on the floor where he stands, in 256ths.
	int scale() const { return _scale; }

	// Moves Jim a step along his walk. Call it every frame while he walks.
	void update(uint32 now);

	// Makes the position set by the last update the one that is drawn.
	void commitPosition();

	bool sprite(const SceneView &view, SceneSprite &sprite) const override;
	bool backSprite(const SceneView &view, SceneSprite &sprite) const override;
	void speechAnchor(const SceneView &view, int32 &outX, int32 &outY) const override;

protected:
	void selectTalkBank(uint32 now) override;
	// While he walks, or has just arrived and not yet turned, he shows his walking bank.
	bool showsTalk() const override { return SceneObject::showsTalk() && !_walkFlag; }

private:
	int floorScale() const;

	Scene *_scene = nullptr;
	int32 _scale = 256;
	ScriptOp::Direction _direction = ScriptOp::kLeft;

	bool _walking = false;
	bool _walkFlag = false; // set when a walk begins and cleared when he turns
	Script *_waiter = nullptr;
	int32 _fixedX = 0, _fixedY = 0;
	int32 _targetX = 0, _targetY = 0;
	int32 _lastMoveX = 0, _lastMoveY = 0;

	bool _moved = false;
	int32 _pendingX = 0, _pendingY = 0;
};

} // End of namespace Tesoro

#endif
