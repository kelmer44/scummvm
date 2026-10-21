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

#include "tesoro/resources.h"

namespace Tesoro {

void Archive::open() {
	Common::File tre;
	if (!tre.open("TESORO.TRE") || !_dat.open("TESORO.DAT")) {
		error("Tesoro: Failed to open TESORO.TRE or TESORO.DAT");
		return;
	}

	if (tre.size() % 28)
		error("Tesoro: TESORO.TRE Invalid TRE Size.");

	while (tre.pos() < tre.size()) {
		Entry e;
		e.offset = tre.readUint32LE();
		e.size = tre.readUint32LE();
		byte flags = tre.readByte();
		char name[20];
		tre.read(name, 19);
		// if this flag is set then they're not files
		if (!(flags & 4))
			continue;
		// verify the calculated position and size dont exceed the dat size.
		if (!name[0] || uint64(e.offset) + 4 + e.size > uint64(_dat.size()))
			error("Tesoro: Bad resource bounds.");
		_dat.seek(e.offset);
		if (_dat.readUint32LE() != e.size)
			error("Tesoro: Resource size mismatch.");
		Common::String n = Common::String(name);
		n.toUppercase();
		if (_entries.contains(n))
			error("Tesoro: Duplicate resource name.");
		_entries[n] = e;
	}
}

Bytes Archive::read(Common::String name) {
	Common::SeekableReadStream *in = stream(name);
	Bytes b;
	b.resize(in->size());

	if (in->read(b.data(), b.size()) != b.size())
		error("Tesoro: Truncated resource.");

	return b;
}

Common::SeekableReadStream *Archive::stream(Common::String name) {
	name.toUppercase();
	if (!_entries.contains(name))
		error("Tesoro: Resource %s not found.", name.c_str());

	const Entry &e = _entries[name];
	_dat.seek(e.offset + 4);

	return _dat.readStream(e.size);
}

class Reader {
	const Bytes &_b;

public:
	uint32 curPos;
	Reader(const Bytes &b) : _b(b), curPos(0) {}
	void check(uint32 p, uint32 n) const {
		if (p + n > _b.size())
			error("Tesoro: Out of bounds read.");
	}
	uint16 u16(uint32 p) const {
		check(p, 2);
		return READ_LE_UINT16(_b.data() + p);
	}
	uint32 u32(uint32 p) const {
		check(p, 4);
		return READ_LE_UINT32(_b.data() + p);
	}
	Common::String str(uint32 p, uint32 n) const {
		check(p, n);
		uint32 end = p;
		while (end < p + n && _b[end])
			++end;
		return Common::String((const char *)_b.data() + p, end - p);
	}
	Bytes bytes(uint32 p, uint32 n) const {
		check(p, n);
		Bytes out;
		out.resize(n);
		if (n)
			memcpy(out.data(), _b.data() + p, n);
		return out;
	}
	Animation bank();
};

Animation Reader::bank() {
	check(curPos, 28);
	// check header is "ANM"
	if ((u32(curPos) & 0xffffff) != 0x4d4e41 || (u32(curPos) >> 24) > 1)
		error("Tesoro: Invalid ANM header.");

	uint frameCount = u16(curPos + 4);
	uint imageCount = u16(curPos + 8);
	if (frameCount > 10000 || imageCount > 10000)
		error("Tesoro: Invalid ANM counts.");
	Animation bank;
	bank.name = str(curPos + 12, 12);
	curPos += 28; // advance past the header
	// Read frame description

	for (uint i = 0; i < frameCount; ++i) {
		check(curPos, 24);
		Frame f;
		f.x = u16(curPos);
		f.y = u16(curPos + 2);
		f.dx = u16(curPos + 4);
		f.dy = u16(curPos + 6);
		f.duration = u16(curPos + 8);
		f.image = u16(curPos + 10);
		f.type = u16(curPos + 20);
		uint extra = u16(curPos + 22);
		check(curPos + 24, extra);
		curPos += 24 + extra;
		bank.frames.push_back(f);
	}

	for (uint i = 0; i < imageCount; ++i) {
		uint32 size = u32(curPos), base = curPos + 4;
		check(base, size);
		if (size < 16)
			error("Tesoro: short image header");
		Common::SharedPtr<Image> im(new Image);
		im->w = u16(base + 8);
		im->h = u16(base + 10);
		im->kind = u16(base + 12);
		uint32 pixelStart = u32(base);
		if (!im->w || !im->h || im->w > 4096 || im->h > 4096 || im->kind < 1 || im->kind > 3 || pixelStart < 16 + im->h * 8U || pixelStart > size)
			error("Tesoro: bad image dimensions/table");
		im->pixels.resize(im->w * im->h);
		Common::fill(im->pixels.begin(), im->pixels.end(), 0);
		for (uint y = 0; y < im->h; ++y) {
			uint32 offset = u32(base + 16 + y * 8);
			uint count = u16(base + 20 + y * 8), x = 0;
			if (offset < 16 + im->h * 8U || uint64(offset) + count * 8 > pixelStart)
				error("Tesoro: bad span table");
			for (uint s = 0; s < count; ++s) {
				uint32 span = base + offset + s * 8, value = u32(span);
				uint skip = u16(span + 4), len = u16(span + 6);
				x += skip;
				if (x + len > im->w || (im->kind == 1 && (value < pixelStart || uint64(value) + len * 2 > size)))
					error("Tesoro: bad image span");
				for (uint k = 0; k < len; ++k) {
					uint32 c;
					if (im->kind == 2)
						c = 0x80000000 | ((value & 31) * 255 / 31); // Shadow marker: renderer multiplies destination.
					else {
						uint16 rgb = im->kind == 1 ? u16(base + value + k * 2) : value;
						c = 0xff000000 | (((rgb >> 11) * 255 / 31) << 16) | ((((rgb >> 5) & 63) * 255 / 63) << 8) | ((rgb & 31) * 255 / 31);
					}
					im->pixels[y * im->w + x + k] = c;
				}
				x += len;
			}
		}
		bank.images.push_back(im);
		curPos = base + size;
	}
	for (uint i = 0; i < bank.frames.size(); ++i)
		if (bank.frames[i].image < -1 || bank.frames[i].image >= (int)imageCount)
			error("Tesoro: bad frame image reference");
	return bank;
}

Animation readAnimation(const Bytes &data) {
	Reader r(data);
	Animation b = r.bank();
	if (r.curPos != data.size())
		error("Tesoro: ANM trailing bytes");
	return b;
}

Common::SharedPtr<Image> readBackground(const Bytes &data) {
	Reader r(data);

	if (r.u32(0) != 0x014c564c || r.u32(8) == 0)
		error("Tesoro: invalid LVL");

	r.curPos = 16; // Skip the LVL header.
	Animation bank = r.bank();

	// This first-bank shortcut is intended for PUERTO.LVL.
	if (bank.name != "FONDO" || bank.frames.empty() || bank.frames[0].image < 0)
		error("Tesoro: expected a background bank");

	return bank.images[bank.frames[0].image];
}

} // End of namespace Tesoro
