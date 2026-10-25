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

#include "tesoro/objects.h"

namespace Tesoro {

// The index of the office and of the harbour in the game's scene table.
static const int kOffice = 0, kHarbour = 1, kTavern = 2;

// How many variables each class of door owns.
static uint doorVariables(uint doorClass) {
	static const byte sizes[11] = {0, 1, 1, 2, 1, 1, 0, 1, 1, 0, 1};
	return doorClass < ARRAYSIZE(sizes) ? sizes[doorClass] : 0;
}

DoorObject::DoorObject(const SceneObjectRecord &record, const SceneGroup &group, const SceneData &scene, uint32 now, uint doorClass)
	: InteractiveObject(record, group, scene, now, 0, doorVariables(doorClass)), _class(doorClass) {
	const Bytes &p = record.parameters;
	if (p.size() < 0x75)
		error("Tesoro: Invalid door %s.", record.name.c_str());
	_destinationLevel = p[0x67];
	Common::String name;
	for (uint i = 0; i < 12 && p[0x68 + i]; ++i)
		name += char(p[0x68 + i]);
	_destinationName = encodeName(name);
	_arrivalDirection = p[0x74] + 1;
}

bool DoorObject::use(int verb) {
	if (verb != 0)
		return false;
	PointerHost &game = *host();
	Scene *destination = game.levelScene(_destinationLevel);
	SceneObject *target = destination->find(_destinationName);
	if (!target || target->kind() != 6)
		error("Tesoro: The door %s leads nowhere.", name().c_str());
	const DoorObject *exit = static_cast<const DoorObject *>(target);
	game.turnJim(approachDirection());
	game.transferJim(*destination, exit->approachX(), exit->approachY(), _arrivalDirection);
	game.setPointerText(Common::String());
	return true;
}

int DoorObject::useHandler() {
	switch (_class) {
	case 2:
		// The door of the harbour to the tavern: the first time, the tavern's scene plays out.
		if (!use(clickedVerb()))
			return 0;
		if (!variables()[0]) {
			variables()[0] = 1;
			host()->startObjectScript("FANTABERNA", "main", nullptr, *host()->levelScene(kTavern));
		}
		return 3;
	case 8:
		// The door of the office has a lock, which shows the door when it is on.
		if (variables()[0])
			return 1;
		break;
	default: {
		static uint warned = 0;
		if (_class != 0 && !(warned & (1u << _class))) {
			warned |= 1u << _class;
			warning("Tesoro: What the doors of class %u do besides leading somewhere is not ported (%s).", _class, name().c_str());
		}
		break;
	}
	}
	return use(clickedVerb()) ? 3 : 0;
}

int DoorObject::runVerb(int verb) {
	if (verb == 0)
		return useHandler();
	if (_class == 8 && verb == 4) {
		// The open verb toggles the lock of the office door.
		variables()[0] = !variables()[0];
		show(variables()[0] != 0);
		return 1;
	}
	return InteractiveObject::runVerb(verb);
}

int DoorObject::useFromScript(PointerHost &host) {
	bindHost(host);
	return useHandler();
}

int DoorObject::onRightClick(PointerHost &host, int frame) {
	if (_class == 8 && !variables()[0])
		return 1;
	// Only in the first scenes of the game, and only with the default verb: Jim is put at the door
	// and the door is used as if he had walked there.
	if (host.jimLevel() < 0x17 && frame == 0) {
		bindHost(host);
		host.transferJim(*scene(), approachX(), approachY(), approachDirection());
		arrive();
		return 1;
	}
	return 0;
}

// How many variables each class of object owns.
static uint objectVariables(uint objectClass) {
	static const byte sizes[12] = {1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0};
	return objectClass < ARRAYSIZE(sizes) ? sizes[objectClass] : 0;
}

ScriptedObject::ScriptedObject(const SceneObjectRecord &record, const SceneGroup &group, const SceneData &scene, uint32 now, uint objectClass)
	: InteractiveObject(record, group, scene, now, 0, objectVariables(objectClass)), _class(objectClass) {
}

bool ScriptedObject::captainLooksAway(PointerHost &host) const {
	const SceneObject *captain = host.levelScene(kOffice)->find("CAPITAN");
	if (!captain)
		error("Tesoro: The office has no captain.");
	// His third bank shows him with his back turned.
	return captain->currentBank() == 2;
}

void ScriptedObject::startScript(const char *block, const char *label, uint8 *variables) {
	host()->startObjectScript(block, label, variables, *scene());
}

int ScriptedObject::runVerb(int verb) {
	PointerHost &game = *host();
	const bool captainAway = _class <= 11 ? captainLooksAway(game) : false;
	SceneObject *captain = game.levelScene(kOffice)->find("CAPITAN");
	switch (_class) {
	case 0: // the stool
		if (verb == 3 || verb == 6) {
			// Its script wants to know whether the vase stands on it.
			const SceneObject *vase = game.levelScene(kOffice)->find("JARRON");
			variables()[0] = vase && vase->isActive();
			startScript("FTABURETE", verb == 3 ? "coger" : "mover", variables());
			return 1;
		}
		break;
	case 1: // the fish
		if (verb == 1) {
			startScript("FPEZESP", "mirar", nullptr);
			return 1;
		}
		break;
	case 2: // the letter
		if (verb == 1) {
			startScript("FCARTA", "mirar", captain->variables());
			return 1;
		}
		break;
	case 3: // the drawers
		if (verb == 4) {
			if (captainAway)
				startScript("FCAJON", "abrir", variables());
			else
				startScript("FNOCOGER", "coger", nullptr);
			return 1;
		}
		break;
	case 4: // the plaque
		if (verb == 1) {
			startScript("FPLACA", "mirar", variables());
			return 1;
		}
		break;
	case 5: // the ballpoint
		if (verb == 1) {
			startScript("FBOLI", "mirar", nullptr);
			return 1;
		}
		if (verb == 3) {
			startScript("FBOLI", "coger", captain->variables());
			return 1;
		}
		if (verb == 4) {
			startScript("FBOLI", "abrir", nullptr);
			return 1;
		}
		break;
	case 6: // the vase
	case 10: { // the pen
		if (verb != 3)
			break;
		if (!captainAway) {
			// With the captain looking Jim cannot take it, and it stops being something to take.
			startScript("FNOCOGER", "coger", nullptr);
			setActive(false);
			return 1;
		}
		SceneObject *item = game.interfaceScene()->find(_class == 6 ? "IJARRON" : "IPLUMA");
		if (!item)
			error("Tesoro: The inventory has no item for %s.", name().c_str());
		item->setActive(true);
		setActive(false);
		return 1;
	}
	case 11: // the door of the office to the next room
		if (verb == 0) {
			startScript("FPUERTARARA", captainAway ? "ir1" : "ir2", nullptr);
			return 1;
		}
		break;
	default: {
		static uint warned = 0;
		if (!(warned & (1u << (_class & 31)))) {
			warned |= 1u << (_class & 31);
			warning("Tesoro: What objects of class %u do for a verb is not ported (%s).", _class, name().c_str());
		}
		break;
	}
	}
	return InteractiveObject::runVerb(verb);
}

ItemObject::ItemObject(const SceneObjectRecord &record, const SceneGroup &group, const SceneData &scene, uint32 now)
	: InteractiveObject(record, group, scene, now, 0, 0) {
	const Bytes &p = record.parameters;
	if (p.size() < 0x64)
		error("Tesoro: Invalid inventory item %s.", record.name.c_str());
	_index = p[1];
	auto string = [&](uint at) {
		Common::String text;
		for (uint i = at; i < at + 12 && i < p.size() && p[i]; ++i)
			text += char(p[i]);
		return text;
	};
	// Its name in the dialogue tables is at 2; when it has none the lines are read from 2 as well,
	// not from 0xE.
	setTextName(string(2));
	const uint linesAt = p[2] ? 0xe : 2;
	static const uint kVerbOfText[6] = {0, 1, 3, 4, 5, 2};
	for (uint k = 0; k < 6; ++k)
		setVerbText(kVerbOfText[k], string(linesAt + 12 * k));
	// What it says to the other things it can meet: 24-byte entries from 0x7C (the name of the thing, then a line).
	const uint count = p[0x62] | (p[0x63] << 8);
	if (p.size() < 0x7c + 0x18 * count)
		error("Tesoro: Invalid inventory item %s.", record.name.c_str());
	for (uint i = 0; i < count; ++i) {
		_useNames.push_back(encodeName(string(0x7c + 0x18 * i)));
		_useLines.push_back(string(0x7c + 0x18 * i + 12));
	}
	// An item is owned once it is activated.
	SceneObject::setActive(false);
}

void ItemObject::setActive(bool active) {
	if (!_game)
		error("Tesoro: The item %s is used before the game is known.", name().c_str());
	if (!active) {
		if (_game->heldItem() == this) {
			_game->holdItem(nullptr, 0);
			show(true);
		}
	} else {
		// With the inventory open, the item it replaces goes back and this one is in the hand.
		if (_game->inventoryOpen()) {
			if (SceneObject *held = _game->heldItem())
				held->show(true);
			show(false);
		}
		_game->holdItem(this, _index + 7);
	}
	SceneObject::setActive(active);
}

const TextEntry *ItemObject::useLine(PointerHost &host, uint64 target) const {
	for (uint i = 0; i < _useNames.size(); ++i) {
		if (_useNames[i] != target)
			continue;
		if (_useLines[i].empty())
			return nullptr;
		const TextEntry *entry = host.findText(_useLines[i]);
		if (!entry)
			error("Tesoro: %s has no text %s.", name().c_str(), _useLines[i].c_str());
		return entry;
	}
	return nullptr;
}

int ItemObject::onLeftClick(PointerHost &host, int frame) {
	bindHost(host);
	setClickedVerb(frame);
	// An item with a line of its own for the verb says it.
	if (frame < 7 && answerVerb(frame, "") != 0)
		return 3;
	if (frame <= 6) {
		// Any other verb takes the item in the hand (verb 0 included).
		setActive(true);
		return 3;
	}
	// An item in the hand meets this one: either has a line for the other.
	SceneObject *held = host.heldItem();
	if (!held || held->kind() != 13)
		error("Tesoro: An item is used on %s without one in the hand.", name().c_str());
	const ItemObject *other = static_cast<const ItemObject *>(held);
	const TextEntry *line = useLine(host, other->nameId());
	if (!line)
		line = other->useLine(host, nameId());
	if (!line) {
		// Nothing is known to happen between them (what some pairs do besides talk is not ported).
		line = host.findText("MODELO2");
		if (!line)
			error("Tesoro: The refusal of an item is missing.");
		host.jimSays(*line);
		return 3;
	}
	host.jimSays(*line);
	host.closeInventory();
	return 3;
}

InventoryPageObject::InventoryPageObject(const SceneObjectRecord &record, const SceneGroup &group, const SceneData &scene, uint32 now)
	: InteractiveObject(record, group, scene, now, 0, 0) {
}

} // namespace Tesoro
