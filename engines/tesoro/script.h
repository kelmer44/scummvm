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

#ifndef TESORO_SCRIPT_H
#define TESORO_SCRIPT_H

#include "common/array.h"
#include "common/ptr.h"
#include "common/str.h"
#include "tesoro/resources.h"

namespace Tesoro {

class Archive;
struct ScriptBlock;

// One line of speech text. Entries are looked up by the 64-bit encoding of their id.
struct TextEntry {
	uint64 id = 0;
	Common::String name; // the id as written; also names the speech WAV
	Common::String text; // raw bytes in the game's code page
};

class TextTable {
public:
	void load(const Bytes &data);
	const TextEntry *find(uint64 id) const;

private:
	Common::Array<TextEntry> _entries; // sorted by id
};

// A compiled script instruction. Only the fields of its type are meaningful.
struct ScriptOp {
	enum Type {
		kReturn = 0,
		kSpeech = 1,
		kSleep = 2,
		kBreak = 3,
		kJump = 4,
		kMenu = 5,
		kMenuItem = 6,
		kIf = 7,
		kRun = 8,
		kMessage = 9,
		kEnd = 10,
		kAssign = 11,
		kNative = 12,
		kMovie = 13,
		kSound = 14,
		kMusic = 15
	};
	// The numbering follows the original's sendmessage sub-operations.
	enum MessageKind { kPlay = 0, kWalk = 1, kLook = 2, kActivate = 3, kShow = 4 };
	enum PlayMode { kStop = 0, kPlayOnce = 1, kLoop = 2, kPlayAndWait = 3 };
	enum Direction { kUp = 1, kDown = 2, kLeft = 3, kRight = 4 };
	enum Scope { kCurrentScene, kInventory, kLevel };

	Type type = kEnd;
	uint line = 0; // source line, for diagnostics

	// kSpeech: speaker 0 is the player character, otherwise a 1-based participant.
	const TextEntry *text = nullptr;
	int speaker = 0;
	int option = 0;  // which of the speaker's talking banks shows the speech

	// kSleep: milliseconds. kMovie: movie number. kMusic: CD track.
	// kNative: ordinal of the DO in its block. kAssign and kIf: the comparison value.
	int value = 0;

	// kJump and kIf: index of the target op. kAssign and kIf: variable index.
	uint target = 0;
	uint8 var = 0;

	// kMenuItem: index of the next item of the same menu, or -1 for the last one.
	// The item is listed when variable var (if var != 0xff) equals 1, or equals 0 if invert.
	int next = -1;
	bool invert = false;

	// kSound: name of the WAV.
	Common::String name;

	// kRun: the block to start (null for the special FANMTRONO target) and its label.
	const ScriptBlock *block = nullptr;
	uint64 blockId = 0;
	uint64 labelId = 0;

	// kMessage
	uint64 object = 0; // encoded name of the target object
	Scope scope = kCurrentScene;
	int level = 0; // kLevel
	MessageKind kind = kPlay;
	bool on = false;            // kActivate, kShow
	uint8 bank = 0;             // kPlay
	int8 first = 0;             // kPlay (signed in the original)
	PlayMode mode = kPlayOnce;  // kPlay
	int16 x = 0, y = 0;         // kWalk
	Direction direction = kUp;  // kLook
};

struct ScriptLabel {
	uint64 id;
	uint index;
};

struct ScriptBlock {
	Common::String name;
	Common::String file;
	uint64 id = 0;
	uint nativeType = 0; // selects the class of the block (and its DO handler)
	Common::Array<uint64> participants;
	Common::Array<ScriptOp> ops;
	Common::Array<ScriptLabel> labels;

	// Index of the first op at a label, or -1.
	int findLabel(uint64 id) const;
};

// All the scripts and speech texts of the game, compiled and linked.
class ScriptLibrary {
public:
	void load(Archive &archive);

	void addTextTable(const Bytes &data);
	void addScripts(const Common::String &file, const Bytes &data);
	// Resolves jump, if and run targets. Call once, after all the scripts are added.
	void link();

	const ScriptBlock *find(uint64 id) const;
	const ScriptBlock *find(const Common::String &name) const { return find(encodeName(name)); }
	const TextEntry *findText(uint64 id) const;
	const Common::Array<Common::SharedPtr<ScriptBlock>> &blocks() const { return _blocks; }

private:
	Common::Array<TextTable> _tables;
	Common::Array<Common::SharedPtr<ScriptBlock>> _blocks;
	bool _linked = false;
};

} // End of namespace Tesoro

#endif
