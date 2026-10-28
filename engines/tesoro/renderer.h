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

// Where the choices of a dialogue menu go on the screen.
struct MenuLayout {
	// The y that each choice starts at (one more entry: the bottom of the screen) before the block
	// is moved down by `shift`.
	Common::Array<int> starts;
	int shift = 0;
	// The range of screen rows that choice `i` answers to the pointer in.
	int top(uint i) const { return starts[i] + shift; }
	int bottom(uint i) const { return starts[i + 1] + shift; }
};

class SceneRenderer {
public:
	SceneRenderer();
	// With `clear` off the sprites are drawn over what is on the surface, each pass with its own depths.
	void draw(Graphics::Surface &surface, const Common::Array<SceneSprite> &sprites, bool clear = true);

	// A dialogue menu: the choices are laid out from y = 50 in lines 18 pixels apart, each starting 30
	// below the last line of the one before, and the block is moved down so that the last line of
	// the last choice ends at 435. Drawn in white; the one under the pointer is drawn again in yellow.
	MenuLayout layoutMenu(Graphics::Surface &surface, const Animation &font, const Common::Array<Common::String> &choices);
	void drawMenu(Graphics::Surface &surface, const Animation &font, const Common::Array<Common::String> &choices,
	              const MenuLayout &layout, int hovered);

	// The table that shadows darken the picture with (TABLA1.TBL): 32 levels of 512 RGB565
	// colours each, indexed by three bits of each channel of the colour underneath.
	void setShadowTable(const Bytes &table);

	// Text centered on the screen, for Jim's narration
	void drawNarration(Graphics::Surface &surface, const Animation &font, const Common::String &text, uint16 colour);

	// Draws a block of text on the screen, handling line wrapping, centering, and vertical shifting.
	// Text centered on (centerX, y) if centered is true, or starts at y if not.
	// Shift is distance to move the block down after laying it out.
	void drawText(Graphics::Surface &surface, const Animation &font, const Common::String &text, uint16 colour, int centerX, int y, int maxWidth, int lineHeight = 0,
	              bool centered = true, int shift = 0, int *lastLineY = nullptr, bool draw = true);

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
