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

#ifndef TESORO_RENDERER_H
#define TESORO_RENDERER_H

#include "tesoro/scene.h"
#include "graphics/surface.h"

namespace Tesoro {

// Draws an image as it is, at a position, with no depth test. Pixels the image does not
// store are left alone. E.g. menus
void drawImage(Graphics::Surface &surface, const Image &image, int x, int y);

class SceneRenderer {
public:
	SceneRenderer();
	void draw(Graphics::Surface &surface, const Common::Array<SceneSprite> &sprites);

	// The table that shadows darken the picture with (TABLA1.TBL): 32 levels of 512 RGB565
	// colours each, indexed by three bits of each channel of the colour underneath.
	void setShadowTable(const Bytes &table);

	// Text that the whole screen is the stage for, centred on the screen.
	void drawNarration(Graphics::Surface &surface, const Animation &font, const Common::String &text, uint16 colour);

	// Lays text out in lines no wider than maxWidth, centres the block on (centreX, y) and
	// keeps it on the screen. A lineHeight of 0 is one and a half times the font's height.
	void drawText(Graphics::Surface &surface, const Animation &font, const Common::String &text, uint16 colour, int centreX, int y, int maxWidth, int lineHeight = 0);

private:
	// Darkens whatever is behind a shadow image.
	void drawShadow(Graphics::Surface &surface, const SceneSprite &sprite);
	void drawScaled(Graphics::Surface &surface, const SceneSprite &sprite);

	Common::Array<uint32> _depth;
	Common::Array<uint16> _shadowTable;
	Common::Array<bool> _shadowed; // the pixels that a shadow has already claimed
};

} // namespace Tesoro
#endif
