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

#include "tesoro/animation.h"

namespace Tesoro {
namespace {

// Wrap explicitly without signed overflow or out-of-range signed casts.
int32 signed32(uint32 bits) {
	return bits <= 0x7fffffffU ? int32(bits) : -1 - int32(~bits);
}

int16 signed16(uint32 bits) {
	uint16 low = uint16(bits);
	return int16(low <= 0x7fff ? int32(low) : int32(low) - 0x10000);
}

} // namespace

AnimationPlayer::AnimationPlayer(const Animation &animation) : _animation(animation) {
	_state.flags = animation.interpolateMovement ? kInterpolate : 0;
}

bool AnimationPlayer::play(uint32 now, uint16 first, int last, bool loop, bool cacheDuration) {
	if (_animation.isFont)
		return false;
	if (_animation.frames.empty()) {
		_state.flags = 0;
		_state.firstFrame = _state.lastFrame = _state.frame = 0;
		_state.cycleDuration = 0;
	} else {
		if (first >= _animation.frames.size() || last < -1)
			return false;
		uint end = first;
		if (last == -1) {
			// A missing image on the next frame delimits
			// the sequence. Looping includes that endpoint for displacement.
			while (end + 1 < _animation.frames.size()) {
				if (_animation.frames[end + 1].image < 0) {
					if (loop)
						++end;
					break;
				}
				++end;
			}
		} else {
			if (uint(last) >= _animation.frames.size() || uint(last) < first)
				return false;
			end = uint(last);
		}

		uint32 duration = 0, liveDuration = 0;
		for (uint i = first; i <= end; ++i) {
			uint16 frameDuration = _animation.frames[i].duration;
			duration += frameDuration;
			if (i < end) {
				liveDuration += frameDuration;
				// A zero-duration first frame cannot interpolate at its start time.
				if (i == first && _animation.interpolateMovement && !frameDuration)
					return false;
			}
		}
		if (loop && first != end && !liveDuration)
			return false; // Otherwise a zero-time loop could never yield.

		_state.flags = (_state.flags & ~uint32(7)) | (loop ? kLoop : 0) | kPlaying;
		_state.firstFrame = first;
		_state.lastFrame = uint16(end);
		_state.frame = first;
		_state.cycleDuration = cacheDuration ? duration : 0;
		if (first == end)
			_state.flags &= ~uint32(kPlaying);
	}
	_state.frameStartedAt = now;
	_state.loopX = _state.loopY = 0;
	return true;
}

void AnimationPlayer::update(uint32 now) {
	if (_animation.frames.empty())
		return;

	// Fold time only when the optional duration cache is set.
	uint32 effectiveNow = now;
	if ((_state.flags & kPlaying) && !(_state.flags & kPaused)) {
		if (_state.cycleDuration)
			effectiveNow -= ((now - _state.frameStartedAt) / _state.cycleDuration) * _state.cycleDuration;
		while (signed32(effectiveNow - _state.frameStartedAt) > _animation.frames[_state.frame].duration) {
			const Frame &frame = _animation.frames[_state.frame];
			if ((_state.flags & kLoop) && uint(_state.frame) + 1 == _state.lastFrame) {
				_state.frameStartedAt += frame.duration;
				_state.frame = _state.firstFrame;
				const Frame &first = _animation.frames[_state.firstFrame];
				const Frame &last = _animation.frames[_state.lastFrame];
				_state.loopX = signed16(uint32(_state.loopX) + uint32(int32(last.dx) - first.dx));
				_state.loopY = signed16(uint32(_state.loopY) + uint32(int32(last.dy) - first.dy));
			} else {
				if (_state.frame == _state.lastFrame) {
					_state.flags &= ~uint32(kPlaying);
					break;
				}
				_state.frameStartedAt += frame.duration;
				++_state.frame;
			}
		}
	}

	const Frame &frame = _animation.frames[_state.frame];
	if (_state.frame != _state.lastFrame && (_state.flags & kPlaying)) {
		int32 x = 0, y = 0;
		if (_state.flags & kInterpolate) {
			const Frame &next = _animation.frames[_state.frame + 1];
			uint32 elapsed = effectiveNow - _state.frameStartedAt;
			// Wrap the product to 32 bits and divide toward zero.
			x = signed32(uint32(int32(next.dx) - frame.dx) * elapsed) / frame.duration;
			y = signed32(uint32(int32(next.dy) - frame.dy) * elapsed) / frame.duration;
		}
		_state.movementX = signed16(uint32(x) + uint32(_state.loopX) + uint32(frame.dx));
		_state.movementY = signed16(uint32(y) + uint32(_state.loopY) + uint32(frame.dy));
	} else {
		// Endpoint and stopped frames use their movement without loop offsets.
		_state.movementX = frame.dx;
		_state.movementY = frame.dy;
	}
}

void AnimationPlayer::pause(uint32 now) {
	_state.flags = (_state.flags & ~uint32(kPlaying)) | kPaused;
	_state.pausedAt = now;
}

void AnimationPlayer::resume(uint32 now) {
	_state.frameStartedAt += now - _state.pausedAt;
	_state.pausedAt = 0;
	_state.flags = (_state.flags & ~uint32(kPaused)) | kPlaying;
}

const Frame *AnimationPlayer::currentFrame() const {
	return _animation.frames.empty() ? nullptr : &_animation.frames[_state.frame];
}

const Image *AnimationPlayer::currentImage() const {
	const Frame *frame = currentFrame();
	return frame && frame->image >= 0 && uint(frame->image) < _animation.images.size()
		? _animation.images[frame->image].get() : nullptr;
}

} // namespace Tesoro
