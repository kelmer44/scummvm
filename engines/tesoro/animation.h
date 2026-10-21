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

#ifndef TESORO_ANIMATION_H
#define TESORO_ANIMATION_H

#include "tesoro/resources.h"

namespace Tesoro {

struct AnimationState {
	uint32 flags = 0;
	uint32 frameStartedAt = 0;
	uint32 cycleDuration = 0;
	uint32 pausedAt = 0;
	uint16 firstFrame = 0, lastFrame = 0, frame = 0;
	int16 movementX = 0, movementY = 0;
	int16 loopX = 0, loopY = 0;
};

// One playback state per object-owned bank.
// Callers supply the game clock; presentation frequency does not advance frames.
class AnimationPlayer {
public:
	enum Flags { kPlaying = 1, kPaused = 2, kLoop = 4, kInterpolate = 8 };

	explicit AnimationPlayer(const Animation &animation);
	bool play(uint32 now, uint16 first = 0, int last = -1, bool loop = false, bool cacheDuration = false);
	void update(uint32 now);
	void pause(uint32 now);
	void resume(uint32 now);
	void clearMovement() { _state.movementX = _state.movementY = 0; }

	const AnimationState &state() const { return _state; }
	const Frame *currentFrame() const;
	const Image *currentImage() const;

private:
	const Animation &_animation;
	AnimationState _state;
};

} // namespace Tesoro
#endif
