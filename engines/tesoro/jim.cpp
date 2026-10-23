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

#include "tesoro/jim.h"

namespace Tesoro {

// Jim's animation banks: the first two are for talking to the left and right, then standings, and
// walks up, down, left and right. The talking banks at the end are for facing the screen.
const char *const kBanks[] = {
	"JIMHABIZ.ANM", "JIMHABDER.ANM", "JIMQUIETO.ANM", "JIMARRIBA.ANM", "JIMABAJO.ANM",
	"JIMIZQUIERDA.ANM", "JIMDERECHA.ANM", "JIMHABAB.ANM", "JIMHABAR.ANM"
};
const uint kIdleBank = 2;
const uint kWalkBank = 2; // plus the direction: up is 1, down 2, left 3, right 4

// Jim is drawn behind the scene objects of the groups whose depth is below this one.
const uint16 kDepth = 35;

uint JimObject::bankCount() {
	return ARRAYSIZE(kBanks);
}

const char *JimObject::bankName(uint index) {
	return kBanks[index];
}

JimObject::JimObject(const Common::Array<Common::SharedPtr<Animation>> &banks)
	: SceneObject(0xffff, encodeName("JIM"), banks, 350, 20, kDepth) {
	if (banks.size() != ARRAYSIZE(kBanks))
		error("Tesoro: Jim needs %u animation banks.", (uint)ARRAYSIZE(kBanks));
	// His words are yellow and wrapped at 300 pixels, what the narrator says is blue, and he talks
	// in the bank that faces the way he looks: left, right, down, then up.
	TalkConfig config;
	config.colour = 0xffc0;
	config.width = 300;
	config.narrationColour = 0x1f;
	config.banks.push_back(0);
	config.banks.push_back(1);
	config.banks.push_back(7);
	config.banks.push_back(8);
	setTalkConfig(config);
	look(ScriptOp::kLeft, 0);
}

int JimObject::floorScale() const {
	const FloorObject *floor = _scene ? _scene->floor() : nullptr;
	return floor ? floor->scaleAt(x(), y()) : 256;
}

void JimObject::place(Scene &scene, int32 x, int32 y, uint32 now) {
	_scene = &scene;
	_walking = false;
	_waiter = nullptr;
	moveTo(x, y);
	_pendingX = x;
	_pendingY = y;
	_moved = false;
	_scale = floorScale();
}

void JimObject::leave() {
	if (_scene)
		_scene->removeObject(this);
	_scene = nullptr;
	_walking = false;
	_waiter = nullptr;
}

void JimObject::selectTalkBank(uint32 now) {
	const uint choice = _direction == ScriptOp::kLeft ? 0 : _direction == ScriptOp::kRight ? 1
		: _direction == ScriptOp::kDown ? 2 : 3;
	playBank(talkConfig().banks[choice], now, 1, 1);
}

bool JimObject::backSprite(const SceneView &view, SceneSprite &sprite) const {
	if (!showsTalk() || (flags() & (kActive | kShown)) != (kActive | kShown))
		return false;
	const AnimationPlayer &player = selectedPlayer();
	const Animation &bank = player.animation();
	if (bank.frames.empty() || bank.frames[0].image < 0)
		return false;
	const Frame &frame = bank.frames[0];
	sprite.image = bank.images[frame.image].get();
	sprite.scale = _scale;
	sprite.x = ((frame.x * _scale) >> 8) + x() - view.x + 320 + player.state().movementX;
	sprite.y = (view.y - ((frame.y * _scale) >> 8)) - y() - player.state().movementY + 240;
	sprite.depth = depth() + 1;
	return true;
}

void JimObject::speechAnchor(const SceneView &view, int32 &outX, int32 &outY) const {
	SceneObject::speechAnchor(view, outX, outY);
	// His words sit a fixed height above his feet, less the size of his picture on the floor.
	const Frame *frame = selectedPlayer().currentFrame();
	const int32 hitY = frame ? frame->hitY : 0;
	outY = (0xbe - ((hitY * _scale) >> 8)) - y() + view.y;
}

void JimObject::look(ScriptOp::Direction direction, uint32 now) {
	_walking = false;
	_walkFlag = false;
	_direction = direction;
	// Standing is a single frame of the idle bank for each direction.
	play(kIdleBank, now, direction - 1, direction - 1, false);
}

void JimObject::startWalk(int32 x, int32 y, Script *waiter, uint32 now) {
	_walking = false;
	if (!_scene || !_scene->floor()) {
		// Without a floor he does not walk, and the script that waits for him goes on.
		if (waiter) {
			look(_direction, now);
			waiter->actionDone();
		}
		return;
	}
	_walking = true;
	_walkFlag = true;
	_fixedX = JimObject::x() * 256;
	_fixedY = JimObject::y() * 256;
	_targetX = x;
	_targetY = y;
	_waiter = waiter;

	// He faces the way that he has farther to go.
	const int32 dx = _targetX - JimObject::x(), dy = _targetY - JimObject::y();
	if (ABS(dy) < ABS(dx))
		_direction = dx >= 0 ? ScriptOp::kRight : ScriptOp::kLeft;
	else
		_direction = dy < 0 ? ScriptOp::kDown : ScriptOp::kUp;

	const uint bank = kWalkBank + _direction;
	if (selectedBank() != bank || !isPlaying()) {
		playBank(bank, now, 1, -1, true);
		_lastMoveX = _lastMoveY = 0;
	}
}

void JimObject::update(uint32 now) {
	if (!_walking)
		return;
	const int32 dx = _targetX - (_fixedX >> 8);
	const int32 dy = _targetY - (_fixedY >> 8);
	const int32 distance = dx * dx + dy * dy;

	AnimationPlayer &player = selectedPlayer();
	player.update(now);
	int32 moveX = (player.state().movementX - _lastMoveX) * _scale;
	int32 moveY = (player.state().movementY - _lastMoveY) * _scale;
	_lastMoveX = player.state().movementX;
	_lastMoveY = player.state().movementY;
	// The animation only moves him along its own axis; the other one follows the slope of the walk.
	if (_direction < ScriptOp::kLeft) {
		if (dy != 0)
			moveX += (moveY * dx) / dy;
	} else if (dx != 0) {
		moveY += (moveX * dy) / dx;
	}
	_fixedY += moveY;
	_fixedX += moveX;

	if (distance == 0) {
		moveTo(_targetX, _targetY);
		_walking = false;
		if (_waiter) {
			// The script goes on, after he turns to face the way that he walked. His scale
			// is not refreshed on this frame.
			Script *waiter = _waiter;
			_waiter = nullptr;
			look(_direction, now);
			waiter->actionDone();
			return;
		}
		look(_direction, now);
		_scale = floorScale();
		return;
	}
	int32 x, y;
	if (distance < (moveX >> 8) * (moveX >> 8) + (moveY >> 8) * (moveY >> 8)) {
		// The step would pass the target.
		_fixedX = _targetX * 256;
		_fixedY = _targetY * 256;
		x = _targetX;
		y = _targetY;
	} else {
		x = _fixedX >> 8;
		y = _fixedY >> 8;
	}
	_pendingX = x;
	_pendingY = y;
	_moved = true;
	_scale = floorScale();
}

void JimObject::commitPosition() {
	if (!_moved)
		return;
	_moved = false;
	moveTo(_pendingX, _pendingY);
}

bool JimObject::sprite(const SceneView &view, SceneSprite &sprite) const {
	const AnimationPlayer &player = selectedPlayer();
	const Frame *frame = player.currentFrame();
	if ((flags() & (kActive | kShown)) != (kActive | kShown) || !frame || !player.currentImage())
		return false;
	sprite.image = player.currentImage();
	sprite.scale = _scale;
	sprite.x = ((frame->x * _scale) >> 8) - view.x + 320 + x();
	sprite.y = (view.y - ((frame->y * _scale) >> 8)) - y() + 240;
	sprite.depth = depth();
	return true;
}

} // End of namespace Tesoro
