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

#include "base/plugins.h"
#include "common/config-manager.h"
#include "common/file.h"
#include "common/md5.h"
#include "common/str-array.h"
#include "common/translation.h"
#include "common/util.h"
#include "darkseed2/detection.h"
#include "darkseed2/detection_tables.h"

const DebugChannelDef DarkSeed2MetaEngineDetection::debugFlagList[] = {
	{ DarkSeed2::kDebugResources,    "Resources",    "Resource handling debug level" },
	{ DarkSeed2::kDebugGraphics,     "Graphics",     "Graphics debug level" },
	{ DarkSeed2::kDebugMusic,        "Music",        "Music debug level" },
	{ DarkSeed2::kDebugSound,        "Sound",        "Sound debug level" },
	{ DarkSeed2::kDebugTalk,         "Talk",         "Talk debug level" },
	{ DarkSeed2::kDebugMovie,        "Movie",        "Movie debug level" },
	{ DarkSeed2::kDebugScript,       "Script",       "Script debug level" },
	{ DarkSeed2::kDebugRooms,        "Rooms",        "Rooms debug level" },
	{ DarkSeed2::kDebugObjects,      "Objects",      "Objects debug level" },
	{ DarkSeed2::kDebugConversation, "Conversation", "Conversation debug level" },
	{ DarkSeed2::kDebugOpcodes,      "Opcodes",      "Script functions debug level" },
	{ DarkSeed2::kDebugRoomConf,     "RoomConf",     "Room config debug level" },
	{ DarkSeed2::kDebugGameflow,     "Gameflow",     "Gameflow debug level" },
	DEBUG_CHANNEL_END
};

DarkSeed2MetaEngineDetection::DarkSeed2MetaEngineDetection() : AdvancedMetaEngineDetection(
	DarkSeed2::gameDescriptions, DarkSeed2::darkseed2Games) {
}

REGISTER_PLUGIN_STATIC(DARKSEED2_DETECTION, PLUGIN_TYPE_ENGINE_DETECTION, DarkSeed2MetaEngineDetection);
