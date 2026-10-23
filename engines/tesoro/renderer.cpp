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

void drawImage(Graphics::Surface &surface, const Image &image, int x, int y) {
	if (image.kind == 2)
		error("Tesoro: Shadow images cannot be drawn on their own.");
	for (uint row = 0; row < image.h; ++row) {
		const int targetY = y + int(row);
		if (targetY < 0 || targetY >= surface.h)
			continue;
		uint32 *dest = (uint32 *)surface.getBasePtr(0, targetY);
		int targetX = x;
		for (uint s = image.rowStart[row]; s < image.rowStart[row + 1]; ++s) {
			const ImageSpan &span = image.spans[s];
			targetX += span.skip;
			for (uint k = 0; k < span.count; ++k, ++targetX)
				if (targetX >= 0 && targetX < surface.w)
					dest[targetX] = rgb565ToArgb(image.pixel(span, k));
		}
	}
}

SceneRenderer::SceneRenderer() {
	_depth.resize(640 * 480);
}

void SceneRenderer::draw(Graphics::Surface &surface, const Common::Array<SceneSprite> &sprites) {
	assert(surface.w == 640 && surface.h == 480 && surface.format.bytesPerPixel == 4);
	surface.fillRect(Common::Rect(640, 480), 0xff000000); // paint surface black
	Common::fill(_depth.begin(), _depth.end(), 0x10000U); // initialize depth buffer with maximum depth value
	for (uint i = 0; i < sprites.size(); ++i) {
		const SceneSprite &sprite = sprites[i];
		if (sprite.scale)
			continue; // scaled sprites are composed after the others
		const Image &image = *sprite.image;
		if (image.kind == 2)
			error("Tesoro: Shadow composition is not implemented.");
		const int top = MAX<int32>(0, sprite.y);
		const int bottom = MIN<int64>(480, int64(sprite.y) + image.h);
		for (int y = top; y < bottom; ++y) {
			uint32 *row = (uint32 *)surface.getBasePtr(0, y);
			const uint imageRow = y - sprite.y;
			int x = sprite.x;
			// Each run of drawn pixels is expanded as it is drawn.
			for (uint s = image.rowStart[imageRow]; s < image.rowStart[imageRow + 1]; ++s) {
				const ImageSpan &span = image.spans[s];
				x += span.skip;
				for (uint k = 0; k < span.count; ++k, ++x) {
					if (x < 0 || x >= 640)
						continue;
					const uint index = y * 640 + x;
					if (sprite.depth < _depth[index]) {
						row[x] = rgb565ToArgb(image.pixel(span, k));
						_depth[index] = sprite.depth;
					}
				}
			}
		}
	}
	// Scaled sprites are drawn from the farthest to the nearest.
	Common::Array<const SceneSprite *> scaled;
	for (uint i = 0; i < sprites.size(); ++i) {
		if (!sprites[i].scale)
			continue;
		uint at = scaled.size();
		while (at > 0 && scaled[at - 1]->depth < sprites[i].depth)
			--at;
		scaled.insert_at(at, &sprites[i]);
	}
	for (uint i = 0; i < scaled.size(); ++i)
		drawScaled(surface, *scaled[i]);
}

// Draws an image at a scale by picking source pixels in 16.16 fixed point: the rows by the
// vertical step, and each run of pixels of a row from where it starts and ends when scaled.
void SceneRenderer::drawScaled(Graphics::Surface &surface, const SceneSprite &sprite) {
	const Image &image = *sprite.image;
	if (image.kind != 1)
		error("Tesoro: Only literal images can be scaled so far.");
	const uint32 scaleX = uint32(sprite.scale) << 8, scaleY = scaleX;
	const int32 x = sprite.x;
	int32 top = sprite.y;
	int32 bottom = (int32((image.h - 1) * scaleY) >> 16) + top;
	const int32 right = (int32((image.w - 1) * scaleX) >> 16) + x;
	const uint32 rowStep = 0xffffffffU / scaleY;
	int32 rowPosition = 0;
	if (top < 0) {
		rowPosition = int32(int64(rowStep) * -int64(top));
		top = 0;
	}
	if (bottom > 479)
		bottom = 479;
	if (x >= 640 || right < 0 || top >= 480 || bottom < 0)
		return;

	const uint32 inverse = 0x1000000U / (scaleX >> 8);
	const int32 rows = bottom - top + 1;
	for (int32 r = 0; r < rows; ++r) {
		const int32 sourceRow = rowPosition >> 16;
		rowPosition += rowStep;
		if (sourceRow < 0 || sourceRow >= image.h)
			continue;
		uint32 *row = (uint32 *)surface.getBasePtr(0, top + r);
		int32 consumed = 0;
		for (uint s = image.rowStart[sourceRow]; s < image.rowStart[sourceRow + 1]; ++s) {
			const ImageSpan &span = image.spans[s];
			const int32 sourceStart = consumed + span.skip;
			const uint32 startFixed = uint32(sourceStart) * scaleX;
			consumed = sourceStart + span.count;
			int32 first = (int32(startFixed) >> 16) + x;
			const int32 last = ((consumed * int32(scaleX) - 0x7fff) >> 16) + x;
			// The first pixel starts part way into a source pixel.
			uint32 position = -((startFixed >> 8 & 0xff) * (inverse >> 8));
			if (position != 0) {
				++first;
				position += inverse;
			}
			for (int32 dx = first; dx <= last; ++dx, position += inverse) {
				if (dx < 0 || dx >= 640)
					continue;
				const uint32 offset = MIN<uint32>(position >> 16, span.count - 1);
				const uint index = (top + r) * 640 + dx;
				if (sprite.depth < _depth[index]) {
					row[dx] = rgb565ToArgb(image.pixel(span, offset));
					_depth[index] = sprite.depth;
				}
			}
		}
	}
}


void SceneRenderer::drawNarration(Graphics::Surface &surface, const Animation &font,
                                 const Common::String &text, uint16 colour) {
	drawText(surface, font, text, colour, 320, 240, 640);
}

void SceneRenderer::drawText(Graphics::Surface &surface, const Animation &font, const Common::String &text,
                             uint16 colour, int centreX, int y, int maxWidth, int lineHeight) {
	if (!font.isFont || font.frames.size() <= 94)
		error("Tesoro: Invalid font.");
	const Frame &space = font.frames[94];
	const int margin = int16(space.hitW) + 2;
	if (lineHeight == 0)
		lineHeight = int16(space.hitH) * 3 / 2;
	// A width that the screen cannot hold is the width of the screen between its margins.
	const int screenWidth = (320 - margin) * 2;
	if (maxWidth > screenWidth) {
		centreX = 320;
		maxWidth = screenWidth;
	}
	if (margin <= 0 || screenWidth <= 0 || lineHeight <= 0)
		error("Tesoro: Invalid font metrics.");
	// Characters that the font has no glyph for, and spaces, are as wide as a space.
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
	// The block is centred on y, then kept between the first and the last line of the screen.
	const int count = lines.size();
	const int top = y - (count * lineHeight) / 2;
	if (lineHeight > top)
		y = lineHeight;
	else if (480 - lineHeight < count * lineHeight + top)
		y = 480 - (count + 1) * lineHeight;
	else
		y = top;
	const uint32 tint = rgb565ToArgb(colour);
	for (uint line = 0; line < lines.size(); ++line, y += lineHeight) {
		int lineWidth = 0;
		for (uint i = 0; i < lines[line].size(); ++i)
			lineWidth += advance(byte(lines[line][i]));
		int x = centreX - lineWidth / 2;
		if (x < margin)
			x = margin;
		else if (640 - margin < centreX + lineWidth / 2)
			x = 640 - margin - lineWidth;
		for (uint i = 0; i < lines[line].size(); ++i) {
			int glyph = int(byte(lines[line][i])) - 33;
			if (glyph < 0 || uint(glyph) >= font.frames.size())
				glyph = 94;
			const Frame &frame = font.frames[glyph];
			if (frame.image < 0 || uint(frame.image) >= font.images.size())
				error("Tesoro: Missing glyph.");
			const Image &image = *font.images[frame.image];
			if (glyph != 94) {
				int glyphY = y + space.y - frame.y;
				for (uint py = 0; py < image.h; ++py) {
					int targetY = glyphY + py;
					if (targetY < 0 || targetY >= surface.h)
						continue;
					uint32 *row = (uint32 *)surface.getBasePtr(0, targetY);
					int targetX = x;
					for (uint s = image.rowStart[py]; s < image.rowStart[py + 1]; ++s) {
						const ImageSpan &span = image.spans[s];
						targetX += span.skip;
						for (uint k = 0; k < span.count; ++k, ++targetX)
							if (targetX >= 0 && targetX < surface.w)
								row[targetX] = image.pixel(span, k) ? tint : 0xff000000;
					}
				}
			}
			x += image.w + 2;
		}
	}
}

} // End of namespace Tesoro
