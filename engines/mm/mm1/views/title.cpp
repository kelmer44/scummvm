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

#include "mm/mm1/views/title.h"
#include "mm/mm1/gfx/gfx.h"
#include "mm/mm1/gfx/screen_decoder.h"
#include "mm/mm1/mm1.h"

namespace MM {
namespace MM1 {
namespace Views {

Title::Title() : UIElement("Title", g_engine) {
	Gfx::ScreenDecoder decoder;
	decoder._indexes[0] = 0;
	decoder._indexes[1] = 2;
	decoder._indexes[2] = 4;
	decoder._indexes[3] = 15;

	if (decoder.loadFile("screen0")) {
		_surface.copyFrom(decoder.getSurface());
	} else {
		error("Could not load screen0");
	}
}

void Title::draw() {
	getSurface().blitFrom(_surface);
}

} // namespace Views
} // namespace MM1
} // namespace MM
