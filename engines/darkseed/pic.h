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

#ifndef DARKSEED_PIC_H
#define DARKSEED_PIC_H

#include "common/array.h"
#include "common/scummsys.h"
#include "common/file.h"

namespace Darkseed {

class Pic {
private:
	uint16 width;
	uint16 height;
	Common::Array<uint8> pixels;

	bool hasReadByte = false;
	byte currentDataByte = 0;

public:
	bool load(const Common::String &filename);
	Common::Array<uint8> &getPixels() {
		return pixels;
	}

	uint16 getWidth() const {
		return width;
	}
	uint16 getHeight() const {
		return height;
	}

	void draw();
	void draw(int xOffset, int yOffset);
private:
	bool load(Common::SeekableReadStream &readStream);

	byte readNextNibble(Common::SeekableReadStream &readStream);
};

} // namespace Darkseed

#endif // DARKSEED_PIC_H
