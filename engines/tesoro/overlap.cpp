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


#include "tesoro/overlap.h"

namespace Tesoro {

namespace {

// The original works in 32 bit integers that wrap, and its divisions never see a zero divisor
// on the data that reaches them; a zero here gives 0 instead of a crash.
int32 mul(int32 a, int32 b) {
	return int32(uint32(a) * uint32(b));
}

int32 shift16(int32 a) {
	return int32(uint32(a) << 16);
}

int32 divide(int32 a, int32 b) {
	return b ? a / b : 0;
}

// Reads a coordinate of a polygon; outside of the points it is 0.
struct Points {
	const OverlapPolygon &polygon;
	explicit Points(const OverlapPolygon &p) : polygon(p) {}
	int32 x(int i) const { return i >= 0 && i < polygon.count && polygon.points ? polygon.points[i * 2] : 0; }
	int32 y(int i) const { return i >= 0 && i < polygon.count && polygon.points ? polygon.points[i * 2 + 1] : 0; }
};

} // namespace

OverlapPolygon OverlapPolygon::fromPoints(const int16 *points, uint16 count) {
	OverlapPolygon polygon;
	polygon.count = count;
	polygon.points = points;
	if (count) {
		int split = 0;
		for (int last = count - 1; split < last && !(points[(split + 1) * 2] < points[split * 2]); ++split) {
		}
		polygon.split = split;
	}
	return polygon;
}

int32 sweptBoxOverlap(const int32 a[4], const int32 b[4], int32 dx, int32 dy) {
	int32 p1 = a[0], p2 = a[1], p3 = a[2], p4 = a[3];
	const int32 p5 = b[0], p6 = b[1], p7 = b[2], p8 = b[3];
	const int32 p9 = dx, p10 = dy;
	if (p4 <= p6 && p8 <= p2 && p1 <= p7 && p5 <= p3)
		return 0;

	int32 moved1 = p1, moved2 = p2, moved3 = p3, moved4 = p4;
	int32 stepX, stepY;
	if (p9 < 0) {
		p1 += p9;
		p3 += p9;
		p2 += p10;
		p4 += p10;
		stepX = -p9;
		stepY = -p10;
	} else {
		moved1 = p9 + p1;
		moved3 = p3 + p9;
		moved2 = p2 + p10;
		moved4 = p4 + p10;
		stepX = p9;
		stepY = p10;
	}
	if (p7 < p1 || moved3 < p5)
		return -1;

	if (p2 < moved2) {
		if (moved2 < p8)
			return -1;
		if (p6 < p4)
			return -1;
		if (p3 < p5) {
			const int32 left = mul(p5 - p3, stepY), right = mul(p6 - p4, stepX);
			if (left - right != 0 && right <= left)
				return -1;
		}
		if (p7 < moved1 && mul(p7 - p1, stepY) < mul(p8 - p2, stepX))
			return -1;
	} else {
		if (p2 < p8)
			return -1;
		if (p6 < moved4)
			return -1;
		if (p3 < p5 && mul(p5 - p3, stepY) < mul(p8 - p2, stepX))
			return -1;
		if (p7 < moved1) {
			const int32 left = mul(p7 - p1, stepY), right = mul(p6 - p4, stepX);
			if (left - right != 0 && right <= left)
				return -1;
		}
	}

	if (p9 < 0) {
		p2 = moved2;
		p3 = moved3;
		p1 = moved1;
		p4 = moved4;
	}
	int32 span;
	if (p3 < p5) {
		if (p6 < p4) {
			span = p6 - p4;
			if (mul(p10, p5 - p3) < mul(p9, span))
				return divide(mul(p5 - p3, 0x10000), p9);
			return divide(shift16(span), p10);
		}
		if (p8 <= p2)
			return divide(mul(p5 - p3, 0x10000), p9);
		span = p5 - p3;
		if (mul(p10, span) < mul(p9, p8 - p2))
			return divide(mul(p8 - p2, 0x10000), p10);
		return divide(shift16(span), p9);
	}
	if (p1 <= p7) {
		if (p6 < p4)
			return divide(mul(p6 - p4, 0x10000), p10);
		return divide(mul(p8 - p2, 0x10000), p10);
	}
	int32 across;
	if (p6 < p4) {
		across = p7 - p1;
		if (mul(p10, across) < mul(p9, p6 - p4))
			return divide(mul(p6 - p4, 0x10000), p10);
	} else {
		if (p8 <= p2)
			return divide(shift16(p7 - p1), p9);
		across = p7 - p1;
		span = p8 - p2;
		if (mul(p10, across) - mul(p9, span) != 0 && mul(p9, span) <= mul(p10, across))
			return divide(shift16(span), p10);
	}
	return divide(shift16(across), p9);
}

bool pointInPolygon(const OverlapPolygon &polygon, int32 px, int32 py) {
	const Points p(polygon);
	const int count = polygon.count;
	if (count == 0)
		return false;
	int edges = count - 1;
	if (edges == 0)
		return false;
	// The vertex before `edge`; the outline is closed, so before the first one comes the second last.
	auto previous = [&](int edge) { return edge == 0 ? count - 2 : edge - 1; };
	// Looks back from the vertex `from` for one that is not level with y.
	auto lastNotLevel = [&](int from, int32 y) {
		int i = from;
		int32 other;
		int guard = 0;
		do {
			if (i < 0)
				i = count - 2;
			other = p.y(i);
			--i;
		} while (other == y && ++guard < count * 2);
		return other;
	};

	uint8 crossings = 0;
	for (int e = 0; e < edges; ++e) {
		const int32 y1 = int16(p.y(e)), y2 = int16(p.y(e + 1));
		const int32 x1 = int16(p.x(e)), x2 = int16(p.x(e + 1));
		if (y1 < y2) {
			if (y1 <= py && py <= y2) {
				if (py == y1) {
					if (x1 < px) {
						int32 before = p.y(previous(e));
						if (before == y1)
							before = lastNotLevel(e - 2, y1);
						if (before < y1)
							++crossings;
					} else if (x1 == px) {
						return true;
					}
				} else if (py != y2) {
					const int32 x = divide((py - y1) * (x2 - x1), y2 - y1) + x1;
					if (x == px)
						return true;
					if (x < px)
						++crossings;
				}
			}
		} else if (y2 < y1) {
			if (y2 <= py && py <= y1) {
				if (py == y1) {
					if (px <= x1) {
						if (x1 == px)
							return true;
					} else {
						int32 before = p.y(previous(e));
						if (before == y1)
							before = lastNotLevel(e - 2, y1);
						if (before > y1)
							++crossings;
					}
				} else if (py != y2) {
					const int32 x = divide((py - y1) * (x2 - x1), y2 - y1) + x1;
					if (x == px)
						return true;
					if (x < px)
						++crossings;
				}
			}
		} else if (y1 == py) {
			const int32 low = MIN(x1, x2), high = MAX(x1, x2);
			if (low <= px && px <= high)
				return true;
		}
	}
	return crossings & 1;
}

bool polygonsOverlap(const OverlapPolygon &a, const OverlapPolygon &b) {
	const Points A(a), B(b);
	const int splitA = a.split;
	bool f6 = false, f7 = false, f8 = false;
	int k = 0, i = 0;

	if (splitA != 0) {
		do {
			if ((int)b.split < i)
				break;
			const int bx = B.x(i);
			if (A.x(k + 1) < bx) {
				++k;
			} else {
				const int ax = A.x(k);
				if (bx < ax) {
					++i;
				} else if (bx == ax) {
					if (A.y(k) < B.y(i)) {
						if (f6)
							return true;
						++i;
						f8 = f7 = true;
					} else {
						if (f7)
							return true;
						++i;
						f6 = true;
					}
				} else {
					const int32 lhs = (B.y(i) - A.y(k)) * (A.x(k + 1) - ax);
					const int32 rhs = (A.y(k + 1) - A.y(k)) * (bx - ax);
					if (lhs <= rhs) {
						if (f8)
							return true;
						f6 = true;
					} else {
						if (f6)
							return true;
						f8 = true;
					}
					++i;
					f7 = f8;
				}
			}
		} while (k < splitA);
	}

	const int firstBack = b.split + 1;
	if (k >= 0) {
		int j = firstBack;
		do {
			if ((int)b.count <= j)
				break;
			const int ax = A.x(k), bx = B.x(j);
			if (bx < ax) {
				--k;
			} else if (A.x(k + 1) < bx) {
				++j;
			} else if (bx == ax) {
				if (A.y(k) < B.y(j)) {
					if (f6)
						return true;
					f7 = true;
					++j;
					f8 = true;
				} else {
					if (f7)
						return true;
					++j;
					f6 = true;
				}
			} else {
				const int32 lhs = (B.y(j) - A.y(k)) * (A.x(k + 1) - ax);
				const int32 rhs = (A.y(k + 1) - A.y(k)) * (bx - ax);
				if (lhs <= rhs) {
					if (f8)
						return true;
					f6 = true;
					f7 = f8;
				} else {
					if (f6)
						return true;
					f7 = true;
				}
				++j;
				f8 = f7;
			}
		} while (k >= 0);
	}

	const int countA = a.count;
	k = countA - 1;
	if (splitA < k) {
		int j = 0;
		do {
			if ((int)b.split < j)
				break;
			const int bx = B.x(j);
			if (A.x(k - 1) < bx) {
				--k;
			} else {
				const int ax = A.x(k);
				if (bx < ax) {
					++j;
				} else if (bx == ax) {
					if (B.y(j) < A.y(k)) {
						if (f7)
							return true;
						++j;
						f6 = true;
					} else {
						if (f6)
							return true;
						++j;
						f8 = true;
						f7 = true;
					}
				} else {
					const int32 lhs = (B.y(j) - A.y(k)) * (A.x(k - 1) - ax);
					const int32 rhs = (A.y(k - 1) - A.y(k)) * (bx - ax);
					if (lhs < rhs) {
						if (f8)
							return true;
						f6 = true;
					} else {
						if (f6)
							return true;
						f8 = true;
					}
					++j;
					f7 = f8;
				}
			}
		} while (splitA < k);
	}

	if (k < countA) {
		int j = firstBack;
		while (j < (int)b.count) {
			const int ax = A.x(k), bx = B.x(j);
			if (bx < ax) {
				++k;
			} else if (A.x(k + 1) < bx) {
				++j;
			} else if (bx == ax) {
				if (B.y(j) < A.y(k)) {
					if (f7)
						return true;
					++j;
					f6 = true;
				} else {
					if (f6)
						return true;
					f7 = true;
					++j;
					f8 = true;
				}
			} else {
				const int32 lhs = (A.x(k - 1) - ax) * (B.y(j) - A.y(k));
				const int32 rhs = (A.y(k - 1) - A.y(k)) * (bx - ax);
				if (lhs < rhs) {
					if (f8)
						return true;
					f6 = true;
					f7 = f8;
				} else {
					if (f6)
						return true;
					f7 = true;
				}
				++j;
				f8 = f7;
			}
			if (countA <= k)
				return false;
		}
	}
	return false;
}

void clipPolygon(const OverlapPolygon &polygon, int32 left, int32 right, Common::Array<int16> &points,
                 OverlapPolygon &out, int32 clipLeft, int32 clipRight) {
	const Points P(polygon);
	points.resize((polygon.count + 16) * 2);
	int16 *o = points.data();
	int op = 0; // in points
	int n = 0;  // the point that `p` stands on
	auto copyPoint = [&](int index) {
		o[op * 2] = P.x(index);
		o[op * 2 + 1] = P.y(index);
		++op;
	};
	auto edgeY = [&](int index, int32 x) {
		const int32 dx = P.x(index + 1) - P.x(index);
		const int16 quotient = int16(divide(mul(x - P.x(index), P.y(index + 1) - P.y(index)), dx));
		return int16(quotient + int16(P.y(index)));
	};

	if (left < clipLeft) {
		int32 next = P.x(1);
		int i = 0;
		while (next < clipLeft && n <= polygon.count + 1) {
			++n;
			next = P.x(i + 2);
			++i;
		}
		o[0] = int16(clipLeft);
		o[1] = edgeY(n, clipLeft);
		op = 1;
	}
	if (clipRight < right) {
		int32 next = P.x(n + 1);
		while (next < clipRight && n <= polygon.count + 1) {
			copyPoint(n);
			next = P.x(n + 2);
			++n;
		}
		copyPoint(n);
		out.split = op;
		// The point where the outline leaves the range, then the one where it comes back.
		o[op * 2] = int16(clipRight);
		o[op * 2 + 1] = edgeY(n, clipRight);
		int m = n + 1;
		next = P.x(n + 2);
		while (clipRight < next && m <= polygon.count + 1) {
			next = P.x(m + 2);
			++m;
		}
		o[(op + 1) * 2] = int16(clipRight);
		o[(op + 1) * 2 + 1] = edgeY(m, clipRight);
		op += 2;
		n = m + 1;
	} else {
		for (int c = (int)polygon.split - n; c > 0; --c, ++n)
			copyPoint(n);
		out.split = op - 1;
	}

	if (clipLeft <= left) {
		for (int c = (int)polygon.count - n; c > 0; --c, ++n)
			copyPoint(n);
		out.count = op;
		out.points = points.data();
		return;
	}
	int32 next = P.x(n + 1);
	while (clipLeft < next && n <= polygon.count + 1) {
		copyPoint(n);
		next = P.x(n + 2);
		++n;
	}
	copyPoint(n);
	o[op * 2] = int16(clipLeft);
	o[op * 2 + 1] = edgeY(n, clipLeft);
	out.count = op + 1;
	out.points = points.data();
}

bool testOverlap(const OverlapBody &a, const OverlapBody &b, OverlapResult &result) {
	// How much more a moves than b, and where a's box stands relative to b's old position.
	int32 dx = ((a.toX - a.fromX) - (b.toX - b.fromX));
	int32 dy = ((a.toY - b.toY) - a.fromY) + b.fromY;
	int32 relX = a.fromX - b.fromX, relY = a.fromY - b.fromY;
	int32 box[4] = {a.box[0] + relX, a.box[1] + relY, a.box[2] + relX, a.box[3] + relY};
	int32 t = sweptBoxOverlap(box, b.box, dx, dy);
	if (t < 0)
		return false;

	if (a.polygon.count != 0 && b.polygon.count != 0) {
		if (a.polygon.count < 2 || b.polygon.count < 2) {
			if (a.polygon.count == 1) {
				if (dx == 0 && dy == 0)
					return false;
				if (pointInPolygon(b.polygon, a.fromX - b.x, a.fromY - b.y))
					t = pointInPolygon(b.polygon, a.toX - b.x, a.toY - b.y) ? -1 : 0;
				else
					t = 0;
			} else {
				if (dx == 0 && dy == 0) {
					t = 1;
				} else if (pointInPolygon(a.polygon, b.fromX - a.x, b.fromY - a.y)) {
					t = pointInPolygon(a.polygon, b.toX - a.x, b.toY - a.y) ? -1 : 0;
				} else {
					t = 0;
				}
			}
		} else {
			// Walks b's outline back along the relative movement and tests it against a's.
			int32 stepX = -dx, stepY = -dy; // local_1a4, local_1a0
			int32 fx = mul(b.fromX, 0x10000), fy = mul(b.fromY, 0x10000);
			int32 deltaX = 0, deltaY = 0, steps = 0;
			if (dx != 0 || dy != 0) {
				const int32 absY = dy > 0 ? dy : stepY;
				const int32 absX = dx > 0 ? dx : stepX;
				if (absX < absY) {
					steps = absY;
					deltaX = divide(mul(dx, -0x10000), steps);
					deltaY = ((stepY < 1) - 1 & 0x20000) - 0x10000;
					fx += 0x8000;
				} else {
					steps = absX;
					deltaX = ((stepX < 1) - 1 & 0x20000) - 0x10000;
					deltaY = divide(mul(dy, -0x10000), steps);
					fy += 0x8000;
				}
			}
			int32 skipped = int32(int64(mul(steps, t)) >> 16);
			fx += mul(skipped, deltaX);
			fy += mul(skipped, deltaY);
			int32 remaining = steps - skipped;
			Common::Array<int16> clipped;
			for (; remaining > -1; --remaining) {
				const int32 offX = a.fromX - (fx >> 16);
				const int32 offY = a.fromY - (fy >> 16);
				const int32 left = a.box[0] + offX, right = a.box[2] + offX;
				if (left <= b.box[2] && b.box[0] <= right) {
					OverlapPolygon cut;
					clipPolygon(b.polygon, b.box[0], b.box[2], clipped, cut, left, right);
					for (uint c = 0; c < cut.count; ++c) {
						clipped[c * 2] = int16(clipped[c * 2] - int16(offX));
						clipped[c * 2 + 1] = int16(clipped[c * 2 + 1] - int16(offY));
					}
					if (polygonsOverlap(a.polygon, cut) || polygonsOverlap(cut, a.polygon))
						break;
				}
				if (remaining == 0)
					return false;
				fy += deltaY;
				fx += deltaX;
			}
			if ((stepX != 0 || stepY != 0) && steps != remaining)
				t = divide(mul((steps - remaining) - 1, 0x10000), steps);
			else
				t = -1;
		}
	}

	if (t == -1)
		return false;
	result.overlap = t;
	result.priority = MAX(a.depth, b.depth);
	if (a.isJim || b.isJim)
		result.priority = 100;
	return true;
}

} // End of namespace Tesoro
