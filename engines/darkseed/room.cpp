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

#include "room.h"
#include "darkseed.h"

const static int roomDescriptionTextTbl[] = {
	0, 138, 165, 165,
	181, 243, 254, 292,
	369, 397, 420, 429,
	438, 447, 463, 473,
	502, 520, 546, 564,
	564, 564, 564, 0,
	569, 574, 574, 574,
	563, 0, 748, 739,
	738, 723, 704, 612,
	619, 0, 758, 0,
	770, 777, 779, 790,
	778, 801, 809, 0,
	810, 811, 814, 821,
	0, 828, 810, 810,
	850, 860, 867, 874,
	886, 651, 682, 701,
	702, 701, 898, 899,
	903};

Darkseed::Room::Room(int roomNumber) : _roomNumber(roomNumber) {
	room1.resize(8);
	walkableLocationsMap.resize(16);
	room3.resize(30);

	if(!load()) {
		error("Failed to load room %d", roomNumber);
	}
}

bool Darkseed::Room::load() {
	Common::String filenameBase = getRoomFilenameBase(_roomNumber);
	Common::String romFilename;
	Common::File file;
	romFilename = g_engine->getRoomFilePath(Common::String::format("%s.rom", filenameBase.c_str(), _roomNumber));
	if(!file.open(romFilename)) {
		return false;
	}

	Common::String nspFilename = stripSpaces(file.readString());
	file.seek(0xd);
	Common::String picFilename = stripSpaces(file.readString());
	file.seek(0x1a);
	Common::String palFilename = stripSpaces(file.readString());
	file.seek(0x27);

	for (int i = 0; i < 8; i++) {
		room1[i].x = file.readUint16BE();
		room1[i].y = file.readUint16BE();
		if (room1[i].y > 233) {
			room1[i].y = 233;
		}
		room1[i].width = file.readUint16BE();
		room1[i].height = file.readUint16BE();
		room1[i].roomNumber = file.readUint16BE();
		room1[i].direction = file.readByte();
	}

	for (int i = 0; i < 16; i++) {
		file.read(walkableLocationsMap[i].strip, 40);
	}

	for (int i = 0; i < 30; i++) {
		room3[i].type = file.readUint16BE();
		room3[i].objNum = file.readUint16BE();
		room3[i].xOffset = file.readUint16BE();
		room3[i].yOffset = file.readUint16BE();
		room3[i].width = file.readUint16BE();
		room3[i].height = file.readUint16BE();
		room3[i].depth = file.readByte();
		room3[i].spriteNum = file.readByte();

		if (room3[i].spriteNum >= 0x29 && room3[i].type != 0 && room3[i].type != 1000) {
			room3[i].height = 0x14;
			room3[i].width = 0x14;
			room3[i].type = 0;
		}

		if (room3[i].objNum == 0 && room3[i].type == 1) {
			if (connectors.size() == 0xc) {
				error("Too many connectors in this room, max of %d", 0xc);
			}
			RoomConnector connector;
			connector.x = room3[i].xOffset;
			connector.y = room3[i].yOffset;

			if (connector.x > 565) {
				connector.x = 565;
			}
			if (connector.x < 75) {
				connector.x = 75;
			}
			if (connector.y > 235) {
				connector.y = 235;
			}
			if (connector.y < 45) {
				connector.y = 45;
			}

			debug("Room Connector: %d %d", connector.x, connector.y);
			connectors.push_back(connector);
			room3[i].type = 0xff;
		}
	}

	file.close();

	if(!pic.load(picFilename)) {
		return false;
	}

	_pal.load(g_engine->getPictureFilePath(Common::String::format("%s.pal", filenameBase.c_str())));

	return true;
}

Common::String Darkseed::Room::stripSpaces(Common::String source) {
	Common::String out;
	const char *src = source.c_str();
	for (int i = 0; i < source.size(); i++) {
		if (src[i] != ' ') {
			out += src[i];
		}
	}
	return out;
}

void Darkseed::Room::draw() {
	pic.draw(0x45, 0x28);

	// print walkable area map.
//	for (int y = 0x28; y < pic.getHeight() + 0x28; y++) {
//		for (int x = 0x45; x < pic.getWidth() + 0x45; x++) {
//			if (canWalkAtLocation(x, y)) {
//				g_engine->_screen->drawLine(x, y, x, y, 14);
//			}
//		}
//	}
//
//	for (int i = 0; i < connectors.size(); i++) {
//		g_engine->_baseSprites.getSpriteAt(0).draw(connectors[i].x, connectors[i].y);
//	}
}

int Darkseed::Room::checkCursorAndMoveableObjects() {
	ActionMode actionMode = g_engine->_actionMode;
	const Sprite &cursorSprite = (actionMode == LookAction)
									 ? g_engine->_cursor.getSpriteForType(ExclamationMark)
									 : g_engine->_cursor.getSprite();
	bool hasObject = false;
	int objNum = -1;
	for (int i = 0; i < room3.size(); i++) {
		if ((room3[i].type == 1 || room3[i].type == 3)
			&& room3[i].xOffset <= cursorSprite.width + g_engine->_cursor.getX()
			&& g_engine->_cursor.getX() <= room3[i].width + room3[i].xOffset
			&& room3[i].yOffset <= cursorSprite.height + g_engine->_cursor.getY()
			&& g_engine->_cursor.getY() <= room3[i].height + room3[i].yOffset
		) {
			if (room3[i].objNum == 25) {
				if (g_engine->_objects.getVar(80) < 3) {
					hasObject = false;
				} else {
					hasObject = true;
				}
			}

			if (room3[i].objNum == 14 && g_engine->_cursor.getY() > 40 && g_engine->_objects.getVar(86) == 0) {
				hasObject = false;
			}

			if (hasObject) {
				objNum = i;
			}
		}
	}
	return hasObject ? objNum : -1;
}

int Darkseed::Room::checkCursorAndStaticObjects(int x, int y) {
	ActionMode actionMode = g_engine->_actionMode;
	const Sprite &cursorSprite = (actionMode == LookAction)
									 ? g_engine->_cursor.getSpriteForType(ExclamationMark)
									 : g_engine->_cursor.getSprite();
	bool hasObject = false;
	for (int i = 0; i < room3.size(); i++) {
		if (room3[i].type == 0
			&& room3[i].xOffset <= cursorSprite.width + g_engine->_cursor.getX()
			&& g_engine->_cursor.getX() <= room3[i].width + room3[i].xOffset
			&& room3[i].yOffset <= cursorSprite.height + g_engine->_cursor.getY()
			&& g_engine->_cursor.getY() <= room3[i].height + room3[i].yOffset
		) {
			if (actionMode != PointerAction && room3[i].objNum >= 5) {
				hasObject = true;
			}

			if (actionMode == PointerAction && room3[i].objNum < 6) {
				hasObject = true;
			}

			if (room3[i].objNum == 59 || room3[i].objNum == 78) {
				if (g_engine->_objects.getVar(34) == 1) {
					hasObject = true;
				} else {
					hasObject = false;
				}
			}

			if (room3[i].objNum == 0x19 && hasObject) {
				if (g_engine->_objects.getVar(80) < 2) {
					hasObject = false;
				} else {
					hasObject = true;
				}
			}

			if (room3[i].objNum == 0x74 && hasObject && (int)actionMode != 0x13) {
				hasObject = false;
			}

			if (hasObject) {
				return i;
			}
		}
	}
	return -1;
}

void Darkseed::Room::update() {
	if (g_engine->_actionMode == HandAction) {
		int moveableObj = checkCursorAndMoveableObjects();
		if (moveableObj == -1) {
			int objectUnderCursor = checkCursorAndStaticObjects(g_engine->_cursor.getX(), g_engine->_cursor.getY());
			if (objectUnderCursor == -1) {
				// TODO CheckCursorAndMovedObjects();
				g_engine->_cursor.setCursorType(Hand);
			} else {
				g_engine->_cursor.setCursorType(HandPointing);
			}
		} else {
			g_engine->_cursor.setCursorType(HandPointing);
		}
	} else if (g_engine->_actionMode == LookAction) {

	} else if (g_engine->_actionMode == PointerAction) {
		int objectUnderCursor = checkCursorAndStaticObjects(g_engine->_cursor.getX(), g_engine->_cursor.getY());
		g_engine->_cursor.setCursorType(objectUnderCursor != -1 ? ConnectorEntrance : Pointer);
	}
}

bool Darkseed::Room::exitRoom() {
	return false;
}
int Darkseed::Room::getExitRoomNumberAtPoint(int x, int y) {
	int obj = checkCursorAndStaticObjects(x, y);
	for (int i = 0; i < room1.size(); i++) {
		if (
			room1[i].roomNumber != 0xff
			&& room3[obj].xOffset <= room1[i].x
			&& room1[i].x <= room3[obj].width + room3[obj].xOffset
			&& room3[obj].yOffset <= room1[i].y
			&& room1[i].y <= room3[obj].yOffset + room3[obj].height
			) {
			return room1[i].roomNumber;
		}
	}
	return -1;
}
Common::String Darkseed::Room::getRoomFilenameBase(int roomNumber) {
	if (roomNumber == 20 || roomNumber == 22) {
		return "room19";
	}
	return Common::String::format("room%d", roomNumber);
}

bool Darkseed::Room::canWalkAtLocation(int x, int y) {
	if (x < 69 || x >= 570 || y < 40 || y >= 239) {
		return false;
	}

	int t = (x - 69) / 5;

	return (walkableLocationsMap[t / 8].strip[(y - 40) / 5] >> (7 - (t % 8) & 0x1f) & 1);
}

bool Darkseed::Room::canWalkInLineToTarget(int x, int y, int targetX, int targetY) {
	int iVar2 = targetX - x;
	int iVar3 = targetY - y;

	if (iVar2 < 0 || iVar3 > 0) {
		if (iVar2 < 1 && iVar3 < 1) {
			if (-iVar2 <= -iVar3) {
				int iVar4 = 0;
				do {
					if (x == targetX) {
						return true;
					}
					iVar4 -= iVar2;
					if (-iVar4 == iVar3 || -iVar3 < iVar4) {
						iVar4 += iVar3;
						x--;
					}
					y--;
				} while (canWalkAtLocation(x, y));
			}
		} else {
			int iVar4 = 0;
			do {
				if (x == targetX) {
					return true;
				}
				iVar4 -= iVar3;
				if (-iVar4 == iVar2 || -iVar2 < iVar4) {
					iVar4 += iVar2;
					y--;
				}
				x--;
			} while (canWalkAtLocation(x, y));
		}
	} else if (-iVar3 < iVar2) {

	} else {

	}
	return false;
}

void Darkseed::Room::printRoomDescriptionText() const {
	int textId = roomDescriptionTextTbl[_roomNumber];
	if (textId != 0) {
		g_engine->_console->printTosText(textId);
	}
}

int Darkseed::Room::getObjectNumUnder6AtCursor() {
	for (int i = 0; i < room3.size(); i++) {
		Common::Rect roomRect(room3[i].xOffset, room3[i].yOffset, room3[i].xOffset + room3[i].width, room3[i].yOffset + room3[i].height);
		if (room3[i].type == 0 && room3[i].objNum < 6 && roomRect.contains(g_engine->_cursor.getPosition())) {
			selectedObjIndex = i;
			return room3[i].objNum;
		}
	}
	return 0;
}

void Darkseed::Room::getWalkTargetForObjectType_maybe(int objId) {
	for (int i = 0; i < room3.size(); i++) {
		if (room3[i].objNum == objId && room3[i].type == 4) {
			g_engine->_player->_walkTarget.x = room3[i].xOffset;
			g_engine->_player->_walkTarget.y = room3[i].yOffset;
			for (int j = 0; j < room1.size(); j++) {
				if (room1[j].roomNumber != 0xff
					&& room3[selectedObjIndex].xOffset < room1[j].x
					&& room1[j].x < room3[selectedObjIndex].xOffset + room3[selectedObjIndex].width
					&& room3[selectedObjIndex].yOffset < room1[j].y
					&& room1[j].y < room3[selectedObjIndex].yOffset + room3[selectedObjIndex].height
					) {
					if (_roomNumber != 0x3d || room1[j].roomNumber == 5 || g_engine->trunkPushCounter > 2) {
						g_engine->BoolByteEnum_2c85_9e67 = true;
					}
					g_engine->targetRoomNumber = room1[j].roomNumber;
					g_engine->targetPlayerDirection = room1[j].direction;
					break;
				}
			}
			return;
		}
	}
	g_engine->_player->_walkTarget.x = g_engine->_cursor.getX();
	g_engine->_player->_walkTarget.y = g_engine->_cursor.getY();
}

static const int room_sprite_related_2c85_41e[] = {
	 1000,          1000,          1000,          1000,
	 1000,          1000,          1000,          1000,
	 1000,          1000,           400,           750,
	  800,          1000,          1000,          1000,
	 1000,          1000,          1000,          1000,
	 1000,          1000,          1000,          1000,
	  750,           850,          1000,          1000,
	 1000,          1000,          1000,           800,
	 1000,          1000,          1000,           900,
	 1000,          1000,          1000,          1000,
	 1000,          1000,          1000,           830,
	 1000,           750,           550,           500,
	  650,          1000,           950,          1000,
	  500,           750,           700,           800,
	  800,          1000,          1000,          1000,
	 1000,          1000,          1000,           245,
	  750,           800,           500,           700,
	  800
};

static const uint8 room_sprite_related_2c85_4303[] = {
	 13,            13,            25,            25,
	28,            15,            22,            18,
	18,            13,            15,            15,
	35,            18,            40,            45,
	25,            22,            20,            10,
	10,            10,            10,            10,
	40,            20,            50,            30,
	25,            10,            10,            35,
	55,            35,            10,            45,
	15,            20,            13,            20,
	20,            15,            25,            30,
	20,            20,            30,            40,
	40,            60,            20,            15,
	5,            20,            10,            35,
	40,            15,            45,            10,
	34,            20,            25,             5,
	15,            25,            10,            10,
	15
};

void Darkseed::Room::calculateScaledSpriteDimensions(int width, int height, int curYPosition) {
	int local_6 = (g_engine->sprite_y_scaling_threshold_maybe - 2) - curYPosition;
	if (local_6 <= 0) {
		local_6 = 0;
	}
	g_engine->scaledWalkSpeed_maybe = room_sprite_related_2c85_41e[_roomNumber] - ((room_sprite_related_2c85_4303[_roomNumber] * local_6) / 5);
	g_engine->scaledSpriteWidth = (width * g_engine->scaledWalkSpeed_maybe) / 1000;
	g_engine->scaledSpriteHeight = (height * g_engine->scaledWalkSpeed_maybe) / 1000;
}
