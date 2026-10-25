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

#ifndef TESORO_INTERFACE_H
#define TESORO_INTERFACE_H

#include "tesoro/scene.h"

namespace Tesoro {

/**
 * The objects of INTERFACE.LVL that make up the verb bar. The bar sits above the top edge of the
 * screen; touching the top edge slides it in, the icon under the pointer lights up, and clicking
 * it changes the verb the pointer shows.
 */

// Kind 2 (ICONOS): a list of the objects that are its children, in order.
class IconGroupObject : public SceneObject {
public:
	IconGroupObject(const SceneObjectRecord &record, const SceneGroup &group, const SceneData &scene);
	const Common::Array<uint64> &children() const { return _children; }

private:
	Common::Array<uint64> _children;
};

// Kind 10 (CURSOR): the pointer that is drawn. Its frame is the verb: 0 is the arrow, 1 to 4 are
// the verbs of the bar, and the frames after those show the inventory items.
class CursorObject : public SceneObject {
public:
	CursorObject(const SceneObjectRecord &record, const SceneGroup &group, const SceneData &scene, uint32 now);
	bool sprite(const SceneView &view, SceneSprite &sprite) const override;
	void setVerb(int verb, uint32 now);
	int verb() const { return _verb; }
	// The picture of the pointer's current frame.
	const Image *image() const { return selectedPlayer().currentImage(); }

private:
	int _verb = 0;
};

// Kind 11 (CONTROL_ICO): a strip along the top edge of the screen. While the pointer is in it the
// bar is out; its frame 1 is a taller strip that holds the pointer while it is on the bar.
class ControlIconObject : public InteractiveObject {
public:
	ControlIconObject(const SceneObjectRecord &record, const SceneGroup &group, const SceneData &scene, uint32 now);
	// Finds the objects it works with in the scene that it belongs to.
	void bind(Scene &scene);

	bool hitTest(PointerHost &host) override;
	int onHover(PointerHost &host) override;
	void onUnhover(PointerHost &host) override;
	int onLeftClick(PointerHost &host, int frame) override;

private:
	SceneObject *_cursor = nullptr, *_square = nullptr;
	Common::Array<SceneObject *> _icons;
	SceneObject *_lit = nullptr;   // the icon that is lit
	int _hovered = -1;             // the index of the icon under the pointer
};

} // namespace Tesoro

#endif
