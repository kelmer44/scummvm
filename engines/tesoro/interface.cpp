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

#include "tesoro/interface.h"

namespace Tesoro {

// The bar is this far above the screen when it is out of sight, and slides down by 85 pixels.
static const int32 kBarSlide = 0x55;
static const int32 kTopOfScreen = 0x1e0;
// An icon is under the pointer when their x differ by less than this.
static const int32 kIconReach = 0x23;

IconGroupObject::IconGroupObject(const SceneObjectRecord &record, const SceneGroup &group, const SceneData &scene)
	: SceneObject(record, group, scene) {
	const Bytes &p = record.parameters;
	if (p.size() < 4)
		error("Tesoro: Invalid icon group %s.", record.name.c_str());
	// A count at 2, then the 12-byte names of the children from 0x10 on.
	const uint count = p[2] | (p[3] << 8);
	if (p.size() < 0x10 + 12 * count)
		error("Tesoro: Invalid icon group %s.", record.name.c_str());
	for (uint i = 0; i < count; ++i) {
		Common::String name;
		for (uint k = 0; k < 12 && p[0x10 + 12 * i + k]; ++k)
			name += char(p[0x10 + 12 * i + k]);
		_children.push_back(encodeName(name));
	}
}

CursorObject::CursorObject(const SceneObjectRecord &record, const SceneGroup &group, const SceneData &scene, uint32 now)
	: SceneObject(record, group, scene) {
	if (record.bankIndices.empty())
		error("Tesoro: The pointer has no animation.");
	// Only grouped objects are drawn and updated with the scene.
	addFlags(kGrouped);
	show(true);
	setVerb(0, now);
}

void CursorObject::setVerb(int verb, uint32 now) {
	_verb = verb;
	if (!play(0, now, verb, verb))
		error("Tesoro: The pointer has no frame %d.", verb);
}

bool CursorObject::sprite(const SceneView &view, SceneSprite &sprite) const {
	return frameSprite(view, sprite);
}

ControlIconObject::ControlIconObject(const SceneObjectRecord &record, const SceneGroup &group, const SceneData &scene, uint32 now)
	: InteractiveObject(record, group, scene, now, 0) {
}

void ControlIconObject::bind(Scene &scene) {
	_cursor = scene.find("CURSOR");
	_square = scene.find("CUADRADO");
	const SceneObject *icons = scene.find("ICONOS");
	if (!_cursor || !_square || !icons || icons->kind() != 2)
		error("Tesoro: The interface has no verb bar.");
	const IconGroupObject *group = static_cast<const IconGroupObject *>(icons);
	_icons.clear();
	for (uint i = 0; i < group->children().size(); ++i) {
		SceneObject *icon = scene.find(group->children()[i]);
		if (!icon)
			error("Tesoro: The verb bar has a missing icon.");
		_icons.push_back(icon);
	}
	if (_icons.empty())
		error("Tesoro: The verb bar has no icons.");
}

void ControlIconObject::shiftX(int32 dx) {
	setPosition(x() + dx, y());
	_square->setPosition(_square->x() + dx, _square->y());
	for (uint i = 0; i < _icons.size(); ++i)
		_icons[i]->setPosition(_icons[i]->x() + dx, _icons[i]->y());
}

bool ControlIconObject::hitTest(PointerHost &host) {
	// Touching the top edge: the strip grows to hold the pointer while it is on the bar, the icons
	// and the shade behind them slide in, and nothing is lit yet.
	host.setPointerText(Common::String());
	const uint32 now = host.now();
	play(0, now, 1, 1);
	for (uint i = 0; i < _icons.size(); ++i)
		_icons[i]->play(0, now, 0, 1);
	_square->play(0, now, 0, 1);
	_lit = nullptr;
	onHover(host);
	return true;
}

int ControlIconObject::onHover(PointerHost &host) {
	// Nothing is decided while the shade is still sliding.
	if (_square->isPlaying())
		return 1;
	const uint32 now = host.now();
	// Once the slide has ended the icons stay where it took them.
	if (_icons[0]->y() > kTopOfScreen) {
		for (uint i = 0; i < _icons.size(); ++i) {
			_icons[i]->setPosition(_icons[i]->x(), _icons[i]->y() - kBarSlide);
			_icons[i]->play(1, now, 0, 0);
		}
	}
	// The icon under the pointer (the last one in reach) is the one that is lit; the one that was
	// lit plays its animation backwards from where it had got to.
	_hovered = -1;
	SceneObject *under = nullptr;
	for (uint i = 0; i < _icons.size(); ++i) {
		if (ABS(_icons[i]->x() - _cursor->x()) < kIconReach) {
			_hovered = i;
			under = _icons[i];
		}
	}
	if (under != _lit) {
		if (under)
			under->play(1, now, 0, 3);
		if (_lit)
			_lit->play(2, now, 3 - _lit->animationState().frame, 3);
		_lit = under;
	}
	return 1;
}

void ControlIconObject::onUnhover(PointerHost &host) {
	// The pointer left the bar: the icons go back above the screen after a moment, and the strip
	// shrinks to the edge again.
	const uint32 now = host.now();
	if (_icons[0]->y() < kTopOfScreen) {
		for (uint i = 0; i < _icons.size(); ++i)
			_icons[i]->setPosition(_icons[i]->x(), _icons[i]->y() + kBarSlide);
	}
	for (uint i = 0; i < _icons.size(); ++i)
		_icons[i]->play(0, now, 2, 3);
	_square->play(0, now, 2, 3);
	play(0, now, 0, 0);
}

int ControlIconObject::onLeftClick(PointerHost &host, int frame) {
	// Choosing an icon changes the pointer's verb to its place on the bar; an item in the hand goes back.
	if (_hovered >= 0) {
		host.holdItem(nullptr, 0);
		host.setVerb(_hovered);
	}
	return 1;
}

} // namespace Tesoro
