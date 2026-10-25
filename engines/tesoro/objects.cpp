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

} // namespace Tesoro
