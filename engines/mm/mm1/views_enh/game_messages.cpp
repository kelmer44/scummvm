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

#include "common/util.h"
#include "mm/utils/strings.h"
#include "mm/mm1/views_enh/game_messages.h"
#include "mm/mm1/globals.h"

namespace MM {
namespace MM1 {
namespace ViewsEnh {

GameMessages::GameMessages(UIElement *owner) :
		ScrollText("GameMessages", owner) {
}

void GameMessages::draw() {
	// Only draw non-focused messages for a single turn
	if (_show || g_events->focusedView() == this) {
		ScrollText::draw();
		_show = false;
	}
}

bool GameMessages::msgInfo(const InfoMessage &msg) {
	if (msg._ynCallback || msg._keyCallback) {
		// Do a first draw to show 3d view at new position
		g_events->redraw();
		g_events->drawElements();

		addView(this);
	}

	setBounds(Common::Rect(0, 18 * 8, 234, 200));

	_show = true;
	_ynCallback = msg._ynCallback;
	_keyCallback = msg._keyCallback;

	// Process the lines
	clear();
	for (auto line : msg._lines) {
		Common::String str = capitalize(line._text);
		str = searchAndReplace(str, "\n", " ");
		addText(str, line.y, 0, ALIGN_LEFT, line.x * 8);
	}

	redraw();
	return true;
}

bool GameMessages::msgKeypress(const KeypressMessage &msg) {
	if (g_events->focusedView() == this) {
		if (_keyCallback) {
			_keyCallback(msg);
		} else if (msg.keycode == Common::KEYCODE_n) {
			close();
		} else if (msg.keycode == Common::KEYCODE_y) {
			close();
			_ynCallback();
		}

		return true;
	}

	return false;
}

} // namespace ViewsEnh
} // namespace MM1
} // namespace MM
