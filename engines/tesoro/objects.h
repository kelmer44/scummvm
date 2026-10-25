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

#ifndef TESORO_OBJECTS_H
#define TESORO_OBJECTS_H

#include "tesoro/scene.h"

namespace Tesoro {

// Kind 6: doors. Using one takes Jim through it to the door it names in another scene.
class DoorObject : public InteractiveObject {
public:
	DoorObject(const SceneObjectRecord &record, const SceneGroup &group, const SceneData &scene, uint32 now, uint doorClass);

	// A right click on a door puts Jim at it and uses it at once.
	int onRightClick(PointerHost &host, int frame) override;
	// Scripts use a door by running its default verb, which works only if the verb that the door
	// was last clicked with was the default one.
	int useFromScript(PointerHost &host);

protected:
	int runVerb(int verb) override;

private:
	// What the default verb does for the class of door.
	int useHandler();
	// Takes Jim through the door, if the verb that the door was clicked with is the default one.
	bool use(int verb);
	// The door that the right click and the use of a door that can be locked work on.
	int rightClick(PointerHost &host, int frame);

	uint _class;
	uint _destinationLevel = 0;     // the scene that the door leads to
	uint64 _destinationName = 0;    // and the door in it where Jim comes out
	int _arrivalDirection = 1;      // the way Jim faces there
};

// Kind 12: objects whose verbs start scripts of the game. What each does depends on its class (the
// second byte of its record), which is shared by the objects of that class in every scene.
class ScriptedObject : public InteractiveObject {
public:
	ScriptedObject(const SceneObjectRecord &record, const SceneGroup &group, const SceneData &scene, uint32 now, uint objectClass);

protected:
	int runVerb(int verb) override;

private:
	// Whether the captain of the office has his back to the room.
	bool captainLooksAway(PointerHost &host) const;
	void startScript(const char *block, const char *label, uint8 *variables);

	uint _class;
};

// Kind 13: the items of the inventory (all in the interface scene). An item that is active is
// owned. Activating one by a script, or clicking it in the inventory, puts it in Jim's hand, where it
// is the pointer; deactivating it takes it away.
class ItemObject : public InteractiveObject {
public:
	ItemObject(const SceneObjectRecord &record, const SceneGroup &group, const SceneData &scene, uint32 now);
	// Scripts reach the items without any click, so they need the game from the start.
	void setGame(PointerHost &game) { _game = &game; }
	// The number of the item; its cursor frame is this plus 7.
	int index() const { return _index; }

	void setActive(bool active) override;
	int onLeftClick(PointerHost &host, int frame) override;

private:
	// What Jim says when this item meets the thing named, if it has a line for it.
	const TextEntry *useLine(PointerHost &host, uint64 target) const;

	PointerHost *_game = nullptr;
	int _index = 0;
	Common::Array<uint64> _useNames;
	Common::Array<Common::String> _useLines;
};

// Kind 15: the inventory page. It covers the page and takes every click, so that none goes to the room.
class InventoryPageObject : public InteractiveObject {
public:
	InventoryPageObject(const SceneObjectRecord &record, const SceneGroup &group, const SceneData &scene, uint32 now);
	bool hitTest(PointerHost &host) override { return true; }
	int onHover(PointerHost &host) override { return 1; }
	void onUnhover(PointerHost &host) override {}
	int onLeftClick(PointerHost &host, int frame) override { return 1; }
};

} // namespace Tesoro

#endif
