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

#ifndef DARKSEED_PLAYER_H
#define DARKSEED_PLAYER_H

#include "common/rect.h"
#include "nsp.h"

namespace Darkseed {

class Player {
private:
	Nsp _cPlayerSprites;
	Nsp _gPlayerSprites;

public:
	Nsp _animations;
	int _frameIdx = 0;
	int _direction = 0;
	Common::Point _position;
	Common::Point _positionLong; // the original sometimes seems to use a long (4 byte) version of the location
	Common::Point _walkTarget;
	int16 playerSpriteWalkIndex_maybe = 0;
	int16 playerWalkFrameDeltaOffset = 0;
	int16 playerNewFacingDirection_maybe = 0;
	bool isPlayerWalking_maybe = false;
	bool _playerIsChangingDirection = false;
	bool _isAutoWalkingToBed = false;

public:
	Player();
	bool loadAnimations(const Common::String &filename);
	const Sprite &getSprite(int frameNo);
	void updateSprite();
	void draw();

	bool isAtPosition(int x, int y) const;
	bool isAtWalkTarget() const;

	void changeDirection(int16 oldDir, int16 newDir);
	void playerFaceWalkTarget();
};

} // namespace Darkseed

#endif // DARKSEED_PLAYER_H
