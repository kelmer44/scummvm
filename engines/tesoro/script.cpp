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

#include "common/textconsole.h"

#include "tesoro/script.h"

namespace Tesoro {
namespace {

const uint kTokenSize = 1024;

bool isSpace(int c) {
	return c == ' ' || (c >= 9 && c <= 13);
}

int digitValue(int c) {
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return 99;
}

// The scripts and text tables are read with the C library's scanf conversions, and
// they depend on its details: a scanset does not skip white space, a literal that
// fails to match is left unread, and a line is only recognised by its first letters.
// This reader offers the conversions those files use, so the original format strings
// can be applied as they are.
class TextReader {
public:
	explicit TextReader(const Bytes &data) : _data(data) {}

	int get() {
		if (_pos >= _data.size())
			return -1;
		int c = _data[_pos++];
		if (c == '\n')
			++_line;
		return c;
	}

	void unget(int c) {
		if (c < 0)
			return;
		--_pos;
		if (c == '\n')
			--_line;
	}

	uint line() const { return _line; }
	uint remaining() const { return _data.size() - _pos; }

	// Returns the number of conversions stored. Stops at the first mismatch.
	int scan(const char *format, ...) {
		va_list args;
		va_start(args, format);
		int count = scanArgs(format, args);
		va_end(args);
		return count;
	}

	void skipSpace() {
		int c;
		while (isSpace(c = get())) {
		}
		unget(c);
	}

	void skipLine() {
		int c;
		do {
			c = get();
		} while (c != '\n' && c != -1);
	}

private:
	int scanArgs(const char *format, va_list args) {
		int count = 0;
		const char *f = format;
		while (*f) {
			if (isSpace((byte)*f)) {
				while (isSpace((byte)*f))
					++f;
				skipSpace();
			} else if (*f != '%') {
				int c = get();
				if (c != (byte)*f) {
					unget(c);
					break;
				}
				++f;
			} else if (f[1] == 'i') {
				f += 2;
				if (!scanInt(*va_arg(args, int *)))
					break;
				++count;
			} else if (f[1] == 's') {
				f += 2;
				skipSpace();
				if (!scanToken(va_arg(args, char *), nullptr))
					break;
				++count;
			} else if (f[1] == '[') {
				bool set[256];
				f = parseSet(f + 2, set);
				if (!scanToken(va_arg(args, char *), set))
					break;
				++count;
			} else {
				error("Tesoro: Unsupported scan conversion in \"%s\".", format);
			}
		}
		return count;
	}

	static const char *parseSet(const char *f, bool *set) {
		const bool negate = *f == '^';
		if (negate)
			++f;
		bool listed[256] = {};
		bool first = true;
		while (*f && (*f != ']' || first)) {
			const byte from = (byte)*f;
			if (f[1] == '-' && f[2] && f[2] != ']') {
				const byte to = (byte)f[2];
				for (uint c = from; c <= to; ++c)
					listed[c] = true;
				f += 3;
			} else {
				listed[from] = true;
				++f;
			}
			first = false;
		}
		for (uint c = 0; c < 256; ++c)
			set[c] = listed[c] != negate;
		return *f ? f + 1 : f;
	}

	// A word when there is no set, otherwise a run of characters in the set.
	bool scanToken(char *out, const bool *set) {
		uint length = 0;
		for (;;) {
			int c = get();
			if (c < 0 || (set ? !set[c] : isSpace(c))) {
				unget(c);
				break;
			}
			if (length + 1 >= kTokenSize)
				error("Tesoro: Script token too long.");
			out[length++] = (char)c;
		}
		out[length] = 0;
		return length != 0;
	}

	bool scanInt(int &out) {
		skipSpace();
		int c = get();
		bool negative = false;
		if (c == '-' || c == '+') {
			negative = c == '-';
			c = get();
		}
		int base = 10;
		if (c == '0') {
			int next = get();
			if (next == 'x' || next == 'X') {
				base = 16;
				c = get();
			} else {
				unget(next);
				base = 8;
			}
		}
		uint32 value = 0;
		uint digits = 0;
		while (digitValue(c) < base) {
			value = value * base + digitValue(c);
			++digits;
			c = get();
		}
		unget(c);
		if (!digits)
			return false;
		out = negative ? -int(value) : int(value);
		return true;
	}

	const Bytes &_data;
	uint _pos = 0;
	uint _line = 1;
};

int scanString(const char *text, const char *format, int *out) {
	Bytes data;
	data.resize(strlen(text));
	if (!data.empty())
		memcpy(data.data(), text, data.size());
	TextReader reader(data);
	return reader.scan(format, out);
}

class BlockParser {
public:
	BlockParser(TextReader &in, const Common::String &file, const ScriptLibrary &library)
		: _in(in), _file(file), _library(library) {}

	Common::SharedPtr<ScriptBlock> parse();

private:
	void fail(const char *what) const {
		error("Tesoro: %s:%u (%s): %s", _file.c_str(), _in.line(), _block->name.c_str(), what);
	}
	void expect(int count, int expected, const char *what) const {
		if (count != expected)
			fail(what);
	}

	void parseSpeech(ScriptOp &op);
	void parseMenu(ScriptOp &op);
	void parsePlay(ScriptOp &op);
	void parseRun(ScriptOp &op);
	void parseSleepOrMessage(ScriptOp &op);
	void parseMessage(ScriptOp &op);

	TextReader &_in;
	const Common::String &_file;
	const ScriptLibrary &_library;
	ScriptBlock *_block = nullptr;
	char _token[kTokenSize];
};

void BlockParser::parseSpeech(ScriptOp &op) {
	op.type = ScriptOp::kSpeech;
	expect(_in.scan("%[^,]s", _token), 1, "speech without a text id");
	op.text = _library.findText(encodeName(_token));
	if (!op.text)
		fail("speech text not found");
	expect(_in.scan(", %i", &op.speaker), 1, "speech without a speaker");
	int c = _in.get();
	if (c == ',')
		expect(_in.scan("%i", &op.option), 1, "bad speech option");
	else
		_in.unget(c);
}

void BlockParser::parseMenu(ScriptOp &op) {
	// "menu" or "menuitemN"; only the length of the first word tells them apart.
	_token[0] = 0;
	_in.scan("%s", _token);
	_in.get();
	if (strlen(_token) < 4) {
		op.type = ScriptOp::kMenu;
		return;
	}
	op.type = ScriptOp::kMenuItem;
	expect(_in.scan("%s", _token), 1, "menu item without next/stop");
	op.next = _token[0] == 'n' ? 1 : 0;
	int c = _in.get();
	if (c == '\n')
		_in.unget(c);
	// When there is no condition this read fails and the previous word stays in the buffer.
	_in.scan("%[^'\n']s", _token);
	op.var = 0xff;
	if (_token[0] == '!' || _token[0] == 'v') {
		op.invert = _token[0] == '!';
		int var = 0;
		if (scanString(_token + (op.invert ? 2 : 1), "%i", &var) != 1)
			fail("bad menu item condition");
		op.var = (uint8)var;
	}
}

void BlockParser::parsePlay(ScriptOp &op) {
	_token[0] = 0;
	_in.scan("lay %s", _token);
	_in.get();
	int value = 0;
	switch (_token[0]) {
	case 'a':
		op.type = ScriptOp::kMovie;
		expect(_in.scan("%i", &value), 1, "play avi without a number");
		op.value = value;
		break;
	case 'c':
		op.type = ScriptOp::kMusic;
		expect(_in.scan("%i", &value), 1, "play cd without a track");
		op.value = value;
		break;
	case 's':
		op.type = ScriptOp::kSound;
		_in.get();
		expect(_in.scan("%[^']s", _token), 1, "play sound without a name");
		op.name = _token;
		break;
	default:
		fail("unknown play command");
	}
}

void BlockParser::parseRun(ScriptOp &op) {
	int c = _in.get();
	if (c == 'e') {
		op.type = ScriptOp::kReturn;
		return;
	}
	if (c != 'u')
		fail("unknown command");
	op.type = ScriptOp::kRun;
	expect(_in.scan("n '%[^']s'", _token), 1, "run without a script name");
	op.blockId = encodeName(_token);
	expect(_in.scan("' %[0-\xa1]s", _token), 1, "run without a label");
	op.labelId = encodeName(_token);
}

void BlockParser::parseSleepOrMessage(ScriptOp &op) {
	int c = _in.get();
	if (c == 'l') {
		op.type = ScriptOp::kSleep;
		expect(_in.scan("eep %i", &op.value), 1, "sleep without a duration");
	} else if (c == 'e') {
		parseMessage(op);
	} else {
		fail("unknown command");
	}
}

void BlockParser::parseMessage(ScriptOp &op) {
	op.type = ScriptOp::kMessage;
	_in.scan("ndmessage ");
	int c = _in.get();
	if (c == '\'') {
		expect(_in.scan("%[^']s", _token), 1, "sendmessage without an object");
		_in.scan("' ");
		op.object = encodeName(_token);
		int next = _in.get();
		if (next == 'i') {
			// "in <scope>" runs the message in another scene.
			expect(_in.scan("n %s", _token), 1, "sendmessage without a scope");
			if (Common::String(_token) == "INVENTARIO") {
				op.scope = ScriptOp::kInventory;
			} else {
				expect(scanString(_token, "LEVEL%i", &op.level), 1, "bad sendmessage scope");
				op.scope = ScriptOp::kLevel;
			}
			_in.get();
		} else {
			_in.unget(next);
		}
	} else {
		// A digit selects a participant of the block, counting from 1.
		const int slot = c - '1';
		if (slot < 0 || uint(slot) >= _block->participants.size())
			fail("sendmessage to an unknown participant");
		op.object = _block->participants[slot];
		_in.get();
	}

	const int kind = _in.get();
	switch (kind) {
	case 'a':
		_in.scan("ctivate o");
		op.kind = ScriptOp::kActivate;
		op.on = _in.get() == 'n';
		break;
	case 's':
		_in.scan("how o");
		op.kind = ScriptOp::kShow;
		op.on = _in.get() == 'n';
		break;
	case 'i': {
		int x = 0, y = 0;
		expect(_in.scan("ra %i %i", &x, &y), 2, "bad walk message");
		op.kind = ScriptOp::kWalk;
		op.x = (int16)x;
		op.y = (int16)y;
		break;
	}
	case 'm': {
		expect(_in.scan("irar %s", _token), 1, "look message without a direction");
		const Common::String direction(_token);
		op.kind = ScriptOp::kLook;
		if (direction == "ARRIBA")
			op.direction = ScriptOp::kUp;
		else if (direction == "ABAJO")
			op.direction = ScriptOp::kDown;
		else if (direction == "IZQUIERDA")
			op.direction = ScriptOp::kLeft;
		else if (direction == "DERECHA")
			op.direction = ScriptOp::kRight;
		else
			fail("unknown look direction");
		break;
	}
	case 'p': {
		int bank = 0, first = 0;
		expect(_in.scan("lay %i %i %s", &bank, &first, _token), 3, "bad play message");
		op.kind = ScriptOp::kPlay;
		op.bank = (uint8)bank;
		op.first = (int8)first;
		const Common::String mode(_token);
		if (mode == "play")
			op.mode = ScriptOp::kPlayOnce;
		else if (mode == "stop")
			op.mode = ScriptOp::kStop;
		else if (mode == "loop")
			op.mode = ScriptOp::kLoop;
		else if (mode == "wait")
			op.mode = ScriptOp::kPlayAndWait;
		else
			fail("unknown play mode");
		break;
	}
	default:
		fail("unknown message");
	}
}

Common::SharedPtr<ScriptBlock> BlockParser::parse() {
	Common::SharedPtr<ScriptBlock> block(new ScriptBlock);
	_block = block.get();
	block->file = _file;

	// The caller has consumed the '.' that starts the header.
	char name[kTokenSize];
	if (_in.scan("%s", name) != 1)
		error("Tesoro: %s:%u: script block without a name.", _file.c_str(), _in.line());
	block->name = name;
	block->id = encodeName(name);

	int type = 0;
	if (_in.get() == ' ')
		expect(_in.scan("%i\n", &type), 1, "bad script type");
	block->nativeType = type;

	int participants = 0;
	expect(_in.scan("%i\n", &participants), 1, "bad participant count");
	if (participants < 0 || participants > 100)
		fail("bad participant count");
	for (int i = 0; i < participants; ++i) {
		expect(_in.scan("%s\n", name), 1, "missing participant name");
		block->participants.push_back(encodeName(name));
	}

	int ordinal = 0;
	for (;;) {
		const int c = _in.get();
		ScriptOp op;
		op.line = _in.line();
		bool emit = true;
		switch (c) {
		case ' ':
			parseSpeech(op);
			break;
		case '.':
			op.type = ScriptOp::kEnd;
			break;
		case 'D':
			op.type = ScriptOp::kNative;
			op.value = ordinal++;
			break;
		case '_': {
			expect(_in.scan("%[0-\xa1]s", _token), 1, "label without a name");
			ScriptLabel label = {encodeName(_token), block->ops.size()};
			block->labels.push_back(label);
			emit = false;
			break;
		}
		case 'b':
			op.type = ScriptOp::kBreak;
			break;
		case 'i': {
			int var = 0, value = 0;
			expect(_in.scan("f v%i == %i jmp %[0-\xa1]s", &var, &value, _token), 3, "bad if");
			op.type = ScriptOp::kIf;
			op.var = (uint8)var;
			op.value = (uint8)value;
			op.labelId = encodeName(_token);
			break;
		}
		case 'j':
			expect(_in.scan("mp %s", _token), 1, "jmp without a label");
			op.type = ScriptOp::kJump;
			op.labelId = encodeName(_token);
			break;
		case 'm':
			parseMenu(op);
			break;
		case 'p':
			parsePlay(op);
			break;
		case 'r':
			parseRun(op);
			break;
		case 's':
			parseSleepOrMessage(op);
			break;
		case 'v': {
			int var = 0, value = 0;
			expect(_in.scan("%i = %i", &var, &value), 2, "bad assignment");
			op.type = ScriptOp::kAssign;
			op.var = (uint8)var;
			op.value = (uint8)value;
			break;
		}
		default:
			fail("unknown command");
		}
		if (emit)
			block->ops.push_back(op);
		_in.skipLine();
		if (c == '.') {
			// The block is followed by one separator line.
			_in.skipLine();
			break;
		}
	}
	return block;
}

} // namespace

void TextTable::load(const Bytes &data) {
	TextReader in(data);
	char id[kTokenSize], text[kTokenSize];
	while (in.remaining() >= 8) {
		if (in.scan(" %[^,]s", id) != 1 || in.scan(", \"%[^\"]s", text) != 1)
			error("Tesoro: Malformed speech text near line %u.", in.line());
		in.get();
		in.get();
		TextEntry entry;
		entry.id = encodeName(id);
		entry.name = id;
		entry.text = text;
		_entries.push_back(entry);
	}

	// Descending selection sort by (low, high), which is the order the lookup expects.
	for (uint i = 0; i + 1 < _entries.size(); ++i) {
		uint best = i;
		for (uint j = i + 1; j < _entries.size(); ++j) {
			const uint32 bestLow = uint32(_entries[best].id), bestHigh = uint32(_entries[best].id >> 32);
			const uint32 low = uint32(_entries[j].id), high = uint32(_entries[j].id >> 32);
			if (bestLow < low || (low == bestLow && bestHigh < high))
				best = j;
		}
		TextEntry tmp = _entries[i];
		_entries[i] = _entries[best];
		_entries[best] = tmp;
	}
}

const TextEntry *TextTable::find(uint64 id) const {
	const int32 low = int32(uint32(id)), high = int32(uint32(id >> 32));
	int first = 0, last = int(_entries.size()) - 1;
	while (first <= last) {
		const int middle = (first + last) >> 1;
		const TextEntry &entry = _entries[middle];
		int32 a = low, b = int32(uint32(entry.id));
		if (a == b) {
			a = high;
			b = int32(uint32(entry.id >> 32));
		}
		const int32 difference = int32(uint32(a) - uint32(b));
		if (difference == 0)
			return &entry;
		if (difference < 0)
			first = middle + 1;
		else
			last = middle - 1;
	}
	return nullptr;
}

int ScriptBlock::findLabel(uint64 labelId) const {
	for (uint i = 0; i < labels.size(); ++i)
		if (labels[i].id == labelId)
			return int(labels[i].index);
	return -1;
}

void ScriptLibrary::addTextTable(const Bytes &data) {
	_tables.push_back(TextTable());
	_tables.back().load(data);
}

void ScriptLibrary::addScripts(const Common::String &file, const Bytes &data) {
	TextReader in(data);
	while (in.get() == '.') {
		BlockParser parser(in, file, *this);
		Common::SharedPtr<ScriptBlock> block = parser.parse();
		if (find(block->id))
			warning("Tesoro: %s: script block %s repeats the name of an earlier one.", file.c_str(), block->name.c_str());
		_blocks.push_back(block);
		if (in.remaining() < 8)
			break;
	}
}

const ScriptBlock *ScriptLibrary::find(uint64 id) const {
	for (uint i = 0; i < _blocks.size(); ++i)
		if (_blocks[i]->id == id)
			return _blocks[i].get();
	return nullptr;
}

const TextEntry *ScriptLibrary::findText(uint64 id) const {
	for (uint i = 0; i < _tables.size(); ++i)
		if (const TextEntry *entry = _tables[i].find(id))
			return entry;
	return nullptr;
}

void ScriptLibrary::link() {
	if (_linked)
		return;
	_linked = true;
	const uint64 throne = encodeName("FANMTRONO");
	for (uint b = 0; b < _blocks.size(); ++b) {
		ScriptBlock &block = *_blocks[b];
		int previousItem = -1;
		for (uint i = 0; i < block.ops.size(); ++i) {
			ScriptOp &op = block.ops[i];
			switch (op.type) {
			case ScriptOp::kJump:
			case ScriptOp::kIf: {
				const int target = block.findLabel(op.labelId);
				// The original sends a jump to a missing label to the first op of the block.
				if (target < 0)
					warning("Tesoro: %s:%u (%s): jump to an unknown label.", block.file.c_str(), op.line, block.name.c_str());
				op.target = target < 0 ? 0 : target;
				break;
			}
			case ScriptOp::kMenu:
				previousItem = -1;
				break;
			case ScriptOp::kMenuItem:
				// "next" items are chained to the item that follows them in the block.
				if (previousItem > 0)
					block.ops[previousItem].next = int(i);
				previousItem = int(i);
				if (op.next == 0)
					previousItem = -1;
				op.next = -1;
				break;
			case ScriptOp::kRun:
				if (op.blockId != throne) {
					op.block = find(op.blockId);
					if (!op.block)
						error("Tesoro: %s:%u (%s): run of an unknown script.", block.file.c_str(), op.line, block.name.c_str());
				}
				break;
			default:
				break;
			}
		}
	}
}

void ScriptLibrary::load(Archive &archive) {
	static const char *const kTexts[] = {"OBJETOS.TXT", "ANIMS.TXT", "CONVERS.TXT", "ICONS.TXT"};
	static const char *const kScripts[] = {"OBJETOS.DLG", "ANIMS.DLG", "CONVERS.DLG", "ICONS.DLG"};
	for (uint i = 0; i < ARRAYSIZE(kTexts); ++i)
		addTextTable(archive.read(kTexts[i]));
	for (uint i = 0; i < ARRAYSIZE(kScripts); ++i)
		addScripts(kScripts[i], archive.read(kScripts[i]));
	link();
}

} // End of namespace Tesoro
