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

#include "tesoro/renderer.h"

namespace Tesoro {

SceneRenderer::SceneRenderer() {
	_depth.resize(640 * 480);
}

void SceneRenderer::draw(Graphics::Surface &surface, const Common::Array<SceneSprite> &sprites) {
	assert(surface.w == 640 && surface.h == 480 && surface.format.bytesPerPixel == 4);
	surface.fillRect(Common::Rect(640, 480), 0xff000000); // paint surface black
	Common::fill(_depth.begin(), _depth.end(), 0x10000U); // initialize depth buffer with maximum depth value
	for (uint i = 0; i < sprites.size(); ++i) {
		const SceneSprite &sprite = sprites[i];
		const Image &image = *sprite.image;
		if (image.kind == 2)
			error("Tesoro: Shadow composition is not implemented.");
		int left = MAX<int32>(0, sprite.x), top = MAX<int32>(0, sprite.y);
		int right = MIN<int64>(640, int64(sprite.x) + image.w);
		int bottom = MIN<int64>(480, int64(sprite.y) + image.h);
		for (int y = top; y < bottom; ++y) {
			uint32 *row = (uint32 *)surface.getBasePtr(0, y);
			for (int x = left; x < right; ++x) {
				uint32 colour = image.pixels[(y - sprite.y) * image.w + x - sprite.x];
				uint index = y * 640 + x;
				if (colour && sprite.depth < _depth[index]) {
					row[x] = colour;
					_depth[index] = sprite.depth;
				}
			}
		}
	}
}


void SceneRenderer::drawNarration(Graphics::Surface &surface, const Animation &font,
                                 const Common::String &text, uint16 colour) {
	if (!font.isFont || font.frames.size() <= 94)
		error("Tesoro: Invalid subtitle font.");
	const Frame &space = font.frames[94];
	int margin = int16(space.hitW) + 2;
	int maxWidth = (320 - margin) * 2;
	int lineHeight = int16(space.hitH) * 3 / 2;
	if (margin <= 0 || maxWidth <= 0 || lineHeight <= 0)
		error("Tesoro: Invalid subtitle metrics.");
	auto advance = [&](byte value) {
		int glyph = int(value) - 33;
		return glyph < 0 || uint(glyph) >= font.frames.size()
			? margin : int16(font.frames[glyph].hitW) + 2;
	};
	Common::Array<Common::String> lines;
	uint start = 0, at = 0, lastSpace = 0;
	int width = 0;
	bool hasSpace = false;
	while (at < text.size()) {
		byte value = byte(text[at]);
		if (value == ' ') {
			lastSpace = at;
			hasSpace = true;
		}
		width += advance(value);
		if (width > maxWidth || at + 1 == text.size()) {
			uint end = at + 1, next = end;
			if (width > maxWidth && hasSpace) {
				end = lastSpace;
				next = lastSpace + 1;
			}
			lines.push_back(Common::String(text.c_str() + start, end - start));
			start = at = next;
			width = 0;
			hasSpace = false;
		} else {
			++at;
		}
	}
	int y = MAX<int>(lineHeight, 240 - int(lines.size()) * lineHeight / 2);
	if (y + int(lines.size()) * lineHeight > 480 - lineHeight)
		y = 480 - (int(lines.size()) + 1) * lineHeight;
	uint32 tint = 0xff000000 | ((colour >> 11) * 255 / 31 << 16)
		| (((colour >> 5) & 63) * 255 / 63 << 8) | ((colour & 31) * 255 / 31);
	for (uint line = 0; line < lines.size(); ++line, y += lineHeight) {
		int lineWidth = 0;
		for (uint i = 0; i < lines[line].size(); ++i)
			lineWidth += advance(byte(lines[line][i]));
		int x = 320 - lineWidth / 2;
		if (x < margin)
			x = margin;
		else if (320 + lineWidth / 2 > 640 - margin)
			x = 640 - margin - lineWidth;
		for (uint i = 0; i < lines[line].size(); ++i) {
			int glyph = int(byte(lines[line][i])) - 33;
			if (glyph < 0 || uint(glyph) >= font.frames.size())
				glyph = 94;
			const Frame &frame = font.frames[glyph];
			if (frame.image < 0 || uint(frame.image) >= font.images.size())
				error("Tesoro: Missing subtitle glyph.");
			const Image &image = *font.images[frame.image];
			if (glyph != 94) {
				int glyphY = y + space.y - frame.y;
				for (uint py = 0; py < image.h; ++py) {
					int targetY = glyphY + py;
					if (targetY < 0 || targetY >= surface.h)
						continue;
					uint32 *row = (uint32 *)surface.getBasePtr(0, targetY);
					for (uint px = 0; px < image.w; ++px) {
						uint32 pixel = image.pixels[py * image.w + px];
						int targetX = x + px;
						if (pixel && targetX >= 0 && targetX < surface.w)
							row[targetX] = (pixel & 0xffffff) ? tint : 0xff000000;
					}
				}
			}
			x += image.w + 2;
		}
	}
}

} // End of namespace Tesoro
