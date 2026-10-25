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

#ifndef TESORO_OVERLAP_H
#define TESORO_OVERLAP_H

#include "common/array.h"
#include "common/scummsys.h"

namespace Tesoro {

/**
 * The outline that a frame carries for hit testing, as the original keeps it: a closed
 * polygon of points (pairs of 16 bit coordinates), whose first run of points does not go back
 * in x. `split` is the index of the last point of that run. A polygon of one point is a point.
 * Reads outside of the points give 0.
 */
struct OverlapPolygon {
	uint16 count = 0;
	uint16 split = 0;
	const int16 *points = nullptr;

	// Finds the split of an outline of `count` points (x, y pairs).
	static OverlapPolygon fromPoints(const int16 *points, uint16 count);
};

/**
 * Sweeps the box a, moving by (dx, dy), against the box b. A box is left, top, right, bottom
 * with top above bottom. Returns -1 if they never touch, 0 if they already overlap, else how far
 * along the movement they first touch, in 65536ths.
 */
int32 sweptBoxOverlap(const int32 a[4], const int32 b[4], int32 dx, int32 dy);

// Whether a point is inside the polygon or on its edge.
bool pointInPolygon(const OverlapPolygon &polygon, int32 x, int32 y);

// Whether two outlines cross or touch.
bool polygonsOverlap(const OverlapPolygon &a, const OverlapPolygon &b);

/**
 * Cuts the polygon down to the x range [clipLeft, clipRight] into `out`, whose points must have
 * room for count + 16 points. `left` and `right` are the x range of the polygon's own box.
 */
void clipPolygon(const OverlapPolygon &polygon, int32 left, int32 right, Common::Array<int16> &points,
                 OverlapPolygon &out, int32 clipLeft, int32 clipRight);

// What the overlap test needs to know about an object.
struct OverlapBody {
	int32 box[4] = {};          // left, top, right, bottom of the hit box
	int32 fromX = 0, fromY = 0; // where it was before the last move
	int32 toX = 0, toY = 0;     // where it was moved to
	int32 x = 0, y = 0;         // the position it is drawn at
	OverlapPolygon polygon;     // outline of the current frame, if any
	uint16 depth = 0;
	bool isJim = false;
};

struct OverlapResult {
	int32 overlap = 0;   // how far along the relative movement they touch, in 65536ths
	uint16 priority = 0; // the depth that the pair is ordered by
};

/**
 * Tests a pair of objects. Returns false when they do not touch or the sweep finds no contact,
 * else fills in the result that the scene queues for the pair.
 */
bool testOverlap(const OverlapBody &a, const OverlapBody &b, OverlapResult &result);

} // End of namespace Tesoro

#endif
