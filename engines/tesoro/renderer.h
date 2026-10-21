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

class SceneRenderer {
public:
	SceneRenderer();
	void draw(Graphics::Surface &surface, const Common::Array<SceneSprite> &sprites);
	void drawNarration(Graphics::Surface &surface, const Animation &font, const Common::String &text, uint16 colour);

private:
	Common::Array<uint32> _depth;
};

} // namespace Tesoro
#endif
