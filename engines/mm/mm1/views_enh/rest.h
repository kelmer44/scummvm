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

#ifndef MM1_VIEWS_ENH_REST_H
#define MM1_VIEWS_ENH_REST_H

#include "mm/mm1/views_enh/yes_no.h"

namespace MM {
namespace MM1 {
namespace ViewsEnh {

class Rest : public YesNo {
private:
	enum Mode { CONFIRM, TOO_DANGEROUS };
	Mode _mode = CONFIRM;

	void tooDangerous();
	void confirmRest();
public:
	Rest();
	virtual ~Rest() {}

	bool msgFocus(const FocusMessage &msg) override;
	void draw() override;
	bool msgKeypress(const KeypressMessage &msg) override;
	bool msgAction(const ActionMessage &msg) override;
	void timeout() override;
};

} // namespace ViewsEnh
} // namespace MM1
} // namespace MM

#endif
