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

#ifndef TESORO_RESOURCES_H
#define TESORO_RESOURCES_H

#include "common/array.h"
#include "common/file.h"

// Resource definitions for the Tesoro engine
namespace Tesoro {

struct Entry {
	uint32 offset, size;
};

typedef Common::Array<byte> Bytes;
class Archive {
    public:
        void open();
        Bytes read(Common::String name);
        Common::SeekableReadStream *stream(Common::String name);
        Common::Array<Common::String> names() const;
        uint count() const { return _entries.size(); }

    private:
        Common::File _dat;
        Common::HashMap<Common::String, Entry> _entries;
};

struct Image {
    uint16 w,h, kind;
    Common::Array<uint32> pixels;
};

struct Frame {
    uint16 x, y, dx, dy, image;
    uint16 duration, type;
};

struct Animation {
    Common::String name;
    Common::Array<Frame> frames;
    Common::Array<Common::SharedPtr<Image>> images;

};

Common::SharedPtr<Image> readBackground(const Bytes &data);
Animation readAnimation(const Bytes &data);
} // End of namespace Tesoro

#endif // TESORO_RESOURCES_H
