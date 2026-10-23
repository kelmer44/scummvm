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
#include "common/rect.h"

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
        bool has(Common::String name) const;
        Common::Array<Common::String> names() const;
        uint count() const { return _entries.size(); }

    private:
        Common::File _dat;
        Common::HashMap<Common::String, Entry> _entries;
};

// A run of pixels in one row of an image: `skip` empty pixels after the previous run, then
// `count` drawn pixels. `value` depends on the kind of the image: for literal images (kind 1)
// it is the index in Image::literals of the run's first RGB565 pixel, for solid images
// (kind 3) the RGB565 colour of the whole run, and for shadow images (kind 2) the intensity.
struct ImageSpan {
    uint16 skip, count;
    uint32 value;
};

// Images are kept as the file stores them: rows of spans. Nothing is expanded until it is drawn.
struct Image {
    uint16 w,h, kind;
    Common::Array<ImageSpan> spans;
    Common::Array<uint32> rowStart; // row y owns spans[rowStart[y] .. rowStart[y + 1])
    Common::Array<uint16> literals; // the RGB565 pixels of kind 1 images, in span order

    // The RGB565 colour of the k-th pixel of a span (kinds 1 and 3).
    uint16 pixel(const ImageSpan &span, uint k) const { return kind == 1 ? literals[span.value + k] : uint16(span.value); }
};

// Converts an RGB565 colour to the engine's screen format (opaque, RGBA8888).
inline uint32 rgb565ToArgb(uint16 rgb) {
    return 0xff000000 | (((rgb >> 11) * 255 / 31) << 16) | ((((rgb >> 5) & 63) * 255 / 63) << 8) | ((rgb & 31) * 255 / 31);
}

// A corner of a floor's walkable outline, with the scale of a character standing there
// (256 is full size). Frames of type 4 hold them.
struct FloorPoint {
    int16 x, y;
    uint16 scale;
};

// A point of a path object's route. Frames of type 2 hold them
struct PathPoint {
    int16 x, y;
};

struct Frame {
    int16 x, y, dx, dy, image;
    int16 hitX, hitY;
    uint16 hitW, hitH;
    uint16 duration, type;
    Common::Array<FloorPoint> floorPoints; // type 4
    Common::Array<PathPoint> pathPoints;   // type 2
};

struct Animation {
    bool isFont = false;
    bool interpolateMovement = false;
    Common::String name;
    Common::Array<Frame> frames;
    Common::Array<Common::SharedPtr<Image>> images;
};

struct SceneObjectRecord {
    Common::String name;
    int32 x, y;
    int16 depth;
    byte field10;
    uint32 kind;
    Bytes parameters;
    Bytes bankIndices;
};

struct SceneGroup {
    Bytes header;
    uint16 depthBase, depthSpan;
    int32 parallax;
    Common::Array<SceneObjectRecord> objects;
};

struct SceneData {
    Common::Array<Common::SharedPtr<Animation>> banks;
    Common::Array<SceneGroup> groups;
};

uint64 encodeName(const Common::String &name);
SceneData readScene(const Bytes &data);

struct MenuButton {
    Common::String name;
    Common::Rect bounds;
    uint action;
};

struct Menu {
    Common::SharedPtr<Image> background;
    Common::SharedPtr<Image> censorship;
    Common::Array<MenuButton> buttons;
};

Menu readMenu(const Bytes &data);
Common::SharedPtr<Image> readBackground(const Bytes &data);
Animation readAnimation(const Bytes &data);
} // End of namespace Tesoro

#endif // TESORO_RESOURCES_H
