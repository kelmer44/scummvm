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
static const int kOffice = 0, kHarbour = 1, kTavern = 2, kCabin = 4, kStore = 5, kKitchen = 6;

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
	// The screen fades out, and Jim comes out of the other door 300 ms later.
	game.transferJim(*destination, exit->approachX(), exit->approachY(), _arrivalDirection, true, target);
	game.setPointerText(Common::String());
	return true;
}

void DoorObject::arrivedThrough(PointerHost &host) {
	bindHost(host);
	arrival(host);
}

void DoorObject::arrival(PointerHost &host) {
	if (_class != 3)
		return;
	// The door of the captain's cabin: the first time Jim comes in (phase 0) the cabin's scene plays out, with
	// the floor and the path of the room set to what the plate Jim holds decides.
	if (host.jimState().phase != 0)
		return;
	SceneObject *plate = host.interfaceScene()->find("IPLATO");
	if (!plate || !plate->variables())
		error("Tesoro: The cabin needs the plate.");
	SceneObject *path = scene()->find("PATH");
	SceneObject *floor = const_cast<FloorObject *>(scene()->floor());
	if (!floor || !path)
		error("Tesoro: The cabin has no floor and path.");
	floor->play(0, host.now(), 0, 0);
	path->play(0, host.now(), 1, 1);
	if (plate->variables()[0] == 2) {
		host.jimState().phase = 1;
		variables()[0] = 6;
	}
	if (plate->isActive()) {
		if (variables()[0] < 4)
			variables()[0] = 4;
	} else {
		variables()[0] = host.randomBelow(4);
		floor->play(0, host.now(), 2, 2);
		path->play(0, host.now(), 3, 3);
	}
	host.startObjectScript("FANCAMAROTE", "main", variables(), *scene());
}

int DoorObject::useHandler() {
	switch (_class) {
	case 3:
	case 9:
		// In the storeroom and the cabin, while the mutiny goes on, a door leads to a script instead.
		if (host()->jimState().busy) {
			SceneObject *map = host()->interfaceScene()->find("IMAPA");
			_scratch = _class == 3 && map && map->isActive();
			host()->startObjectScript("FSALCAMAROTE", "ira", &_scratch, *scene());
			return 3;
		}
		break;
	case 4:
		// The storeroom's second door: the first time, the kitchen's scene plays out.
		if (!use(clickedVerb()))
			return 0;
		if (!variables()[0]) {
			variables()[0] = 1;
			SceneObject *dick = host()->levelScene(kKitchen)->find("DICK");
			if (!dick || !dick->variables())
				error("Tesoro: The kitchen has no Dick.");
			host()->startObjectScript("FANPATATAS", "main", dick->variables(), *host()->levelScene(kKitchen));
		}
		return 1;
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
		if (_class != 0 && _class != 3 && _class != 9 && !(warned & (1u << _class))) {
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

// How many variables each class of object owns, by kind.
static uint objectVariables(uint kind, uint objectClass) {
	static const byte kind12[12] = {1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0};
	static const byte kind17[17] = {0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0};
	static const byte kind20[31] = {1, 0, 0, 2, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 0, 0, 1};
	switch (kind) {
	case 12: return objectClass < ARRAYSIZE(kind12) ? kind12[objectClass] : 0;
	case 17: return objectClass < ARRAYSIZE(kind17) ? kind17[objectClass] : 0;
	default: return objectClass < ARRAYSIZE(kind20) ? kind20[objectClass] : 1;
	}
}

ScriptedObject::ScriptedObject(const SceneObjectRecord &record, const SceneGroup &group, const SceneData &scene, uint32 now, uint objectKind, uint objectClass)
	: InteractiveObject(record, group, scene, now, 0, objectVariables(objectKind, objectClass)), _kind(objectKind), _class(objectClass) {
}

bool ScriptedObject::owned(const char *item) const {
	const SceneObject *object = host()->interfaceScene()->find(item);
	if (!object)
		error("Tesoro: The inventory has no %s.", item);
	return object->isActive();
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
	switch (_kind) {
	case 17:
		return runKind17(verb);
	case 20:
		return runKind20(verb);
	default:
		return runKind12(verb);
	}
}

int ScriptedObject::runKind12(int verb) {
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

int ScriptedObject::runKind17(int verb) {
	PointerHost &game = *host();
	JimState &jim = game.jimState();
	switch (_class) {
	case 0: // the book on the cabin's table
		if (verb == 1 || verb == 3) {
			SceneObject *table = scene()->find("MESA");
			_scratch = table && table->variables() && table->variables()[0] > 0;
			startScript("FLIBRO", verb == 1 ? "mirar" : "coger", &_scratch);
			return 1;
		}
		break;
	case 1: // the flag
		if (verb == 1) {
			startScript("FBANDERA", "mirar", nullptr);
			return 1;
		}
		break;
	case 2: // the books, and the shut vent: what they say is chosen at random
		if (verb == 1) {
			variables()[0] = game.randomBelow(5);
			startScript("FLIBROS", "mirar", variables());
			return 1;
		}
		break;
	case 3: // the rug
		if (verb == 3) {
			variables()[0] = jim.phase == 8;
			startScript("FALFOMBRA", "coger", variables());
			return 1;
		}
		break;
	case 4: // the table
		if (verb == 1) {
			if (owned("IPAPELES") && variables()[0] < 1)
				variables()[0] = 1;
			startScript("FMESA", "mirar", variables());
			return 1;
		}
		break;
	case 5: // the open chest
		if (verb == 1) {
			startScript("FBAULOPEN", "mirar", variables());
			return 1;
		}
		break;
	case 6: // the open vent
		if (verb == 0) {
			_scratch = owned("IMAPA");
			startScript("FVENTOPEN", "mirar", &_scratch);
			return 1;
		}
		break;
	case 7: // the plums
		if (verb == 3) {
			variables()[0] = owned("ICIRUELA") || owned("ICIRUELABRT");
			startScript("FCIRUELAS", "coger", variables());
			return 1;
		}
		break;
	case 8: // the chest of the storeroom
		if (verb == 1 || verb == 3) {
			variables()[0] = jim.phase == 4;
			if (verb == 3 && jim.phase == 4) {
				jim.busy = 0;
				jim.phase = 5;
			}
			startScript("FCOFRE", verb == 1 ? "mirar" : "coger", variables());
			return 1;
		}
		break;
	case 9: // the rope of the storeroom
		if (verb == 3) {
			variables()[0] = owned("ICUERDANUDO");
			if (!variables()[0]) {
				// The other storeroom's rope goes with it.
				if (SceneObject *other = game.levelScene(19)->find("CUERDA"))
					other->setActive(false);
			}
			startScript("FCUERDA", "coger", variables());
			return 1;
		}
		break;
	case 10: // the potatoes
		if (verb == 3) {
			variables()[0] = game.jimLevel() != 20;
			startScript("FPATATAS", "coger", variables());
			return 1;
		}
		break;
	case 11: // the pot
		if (verb == 2) {
			startScript("FPUCHERO", "hablar", nullptr);
			return 1;
		}
		break;
	case 12: // the bucket
		if (verb == 3) {
			startScript("FCUBO", "coger", nullptr);
			return 1;
		}
		break;
	case 13: // the papers
		if (verb == 3) {
			variables()[0] = jim.phase == 6;
			startScript("FPAPELES", "coger", variables());
			return 1;
		}
		break;
	case 14: // the powder
		if (verb == 3) {
			SceneObject *item = game.interfaceScene()->find("IPOLVORA");
			if (!item)
				error("Tesoro: The inventory has no powder.");
			item->setActive(true);
			return 1;
		}
		break;
	case 15: // the revitalising potion
		if (verb == 3) {
			SceneObject *item = game.interfaceScene()->find("IREVITALIZA");
			if (!item)
				error("Tesoro: The inventory has no potion.");
			item->setActive(true);
			return 1;
		}
		break;
	case 16: // the chest of the cabin
		if (verb == 4) {
			if (jim.phase == 2) {
				jim.busy = 0;
				jim.phase = 3;
				startScript("FBAUL", "main", nullptr);
				return 1;
			}
			startScript("FBAUL", "main2", nullptr);
			return 0;
		}
		break;
	}
	return InteractiveObject::runVerb(verb);
}

int ScriptedObject::runKind20(int verb) {
	switch (_class) {
	case 18: // the sword
		if (verb == 3) {
			startScript("FESPADA", "coger", nullptr);
			return 1;
		}
		break;
	case 23: // the cut rope
		if (verb == 3) {
			SceneObject *item = host()->interfaceScene()->find("ISOGA");
			if (!item)
				error("Tesoro: The inventory has no rope.");
			item->setActive(true);
			setActive(false);
			return 1;
		}
		break;
	case 24: // the dead captain
		if (verb == 1) {
			startScript("FCAPIMUERTO", "mirar", variables());
			return 1;
		}
		break;
	case 25: // the suicide note
		if (verb == 1) {
			SceneObject *body = host()->levelScene(18)->find("SUICIDA");
			if (!body || !body->variables())
				error("Tesoro: The note needs the body.");
			startScript("FNOTARIP", "mirar", body->variables());
			return 1;
		}
		break;
	case 26: // the suicide
		if (verb == 1) {
			startScript("FSUICIDA", "mirar", variables());
			return 1;
		}
		break;
	default: {
		static uint warned = 0;
		if (!(warned & (1u << (_class & 31)))) {
			warned |= 1u << (_class & 31);
			warning("Tesoro: What objects of kind 20 class %u do for a verb is not ported (%s).", _class, name().c_str());
		}
		break;
	}
	}
	return InteractiveObject::runVerb(verb);
}

int ScriptedObject::itemUsed(int item) {
	// Items that something in the ship answers to; for any other item the default refusal.
	if (_kind == 17 && _class == 11 && item == 20) {
		startScript("FIPESCAO", "pucherito", nullptr);
		return 1;
	}
	if (_kind == 17 && _class == 16 && item == 21) {
		startScript("FILLAVE", "cerradura", nullptr);
		return 3;
	}
	if (_kind == 20 && _class == 19 && item == 40) {
		startScript("FCORTAR", "main", nullptr);
		return 3;
	}
	return InteractiveObject::itemUsed(item);
}

} // namespace Tesoro
