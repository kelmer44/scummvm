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

#include "tesoro/console.h"
#include "tesoro/tesoro.h"

namespace Tesoro {

Console::Console() : GUI::Debugger() {
	registerCmd("test",   WRAP_METHOD(Console, Cmd_test));
	registerCmd("hotspots", WRAP_METHOD(Console, Cmd_hotspots));
}

Console::~Console() {
}

// Draws the hit boxes and outlines of the objects over the scene: hotspots [on|off]
bool Console::Cmd_hotspots(int argc, const char **argv) {
	bool show = !g_engine->showHotspots();
	if (argc > 1) {
		const Common::String arg(argv[1]);
		if (arg == "on")
			show = true;
		else if (arg == "off")
			show = false;
		else {
			debugPrintf("Usage: hotspots [on|off]\n");
			return true;
		}
	}
	g_engine->setShowHotspots(show);
	debugPrintf("Hotspot layer %s\n", show ? "on" : "off");
	return true;
}

bool Console::Cmd_test(int argc, const char **argv) {
	debugPrintf("Test\n");
	return true;
}

} // End of namespace Tesoro
