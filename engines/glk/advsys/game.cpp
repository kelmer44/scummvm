/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "glk/advsys/game.h"
#include "glk/advsys/definitions.h"
#include "common/memstream.h"

namespace Glk {
namespace AdvSys {

void Decrypter::decrypt(byte *data, size_t size) {
	for (; --size; ++data)
		*data = ~(*data + 30);
}

/*--------------------------------------------------------------------------*/

#define HEADER_SIZE 62

bool Header::init(Common::ReadStream &s) {
	_valid = false;
	byte data[HEADER_SIZE];

	// Read in the data
	if (s.read(data, HEADER_SIZE) != HEADER_SIZE)
		return false;
	decrypt(data, HEADER_SIZE);
	Common::MemoryReadStream ms(data, HEADER_SIZE, DisposeAfterUse::NO);

	// Validate the header
	_valid = !strncmp((const char*)data + 2, "ADVSYS", 6);
	if (!_valid)
		return false;
	
	_size = ms.readUint16LE();
	ms.skip(6);
	_headerVersion = ms.readUint16LE();
	_name = Common::String((const char *)data + 10, (const char *)data + 28);
	ms.skip(18);
	_version = ms.readUint16LE();
	_wordTableOffset = ms.readUint16LE();
	_wordTypeTableOffset = ms.readUint16LE();
	_objectTableOffset = ms.readUint16LE();
	_actionTableOffset = ms.readUint16LE();
	_variableTableOffset = ms.readUint16LE();
	_dataSpaceOffset = ms.readUint16LE();
	_codeSpaceOffset = ms.readUint16LE();
	_dataBlockOffset = ms.readUint16LE();
	_messageBlockOffset = ms.readUint16LE();
	_initCodeOffset = ms.readUint16LE();
	_updateCodeOffset = ms.readUint16LE();
	_beforeOffset = ms.readUint16LE();
	_afterOffset = ms.readUint16LE();
	_errorHandlerOffset = ms.readUint16LE();
	_saveAreaOffset = ms.readUint16LE();
	_saveSize = ms.readUint16LE();

	return true;
}

/*--------------------------------------------------------------------------*/

#define MAX_VERSION 102
#define WORD_SIZE 6

/**
 * Property flags
 */
enum PropertyFlag {
	P_CLASS = 0x8000
};

/**
 * Link fields
 */
enum LinkField {
	L_DATA = 0,
	L_NEXT = 2,
	L_SIZE = 4
};

bool Game::init(Common::SeekableReadStream &s) {
	// Load the header
	s.seek(0);
	if (!Header::init(s))
		return false;
	
	if (_headerVersion < 101 || _headerVersion > MAX_VERSION)
		error("Wrong version number");

	// Load the needed resident game data and decrypt it
	_residentOffset = _dataBlockOffset * 512;
	s.seek(_residentOffset);

	_data.resize(_size);
	if (!s.read(&_data[0], _size))
		return false;
	decrypt(&_data[0], _size);

	_residentBase = &_data[0];
	_wordTable = &_data[_wordTableOffset];
	_wordTypeTable = &_data[_wordTypeTableOffset];
	_objectTable = &_data[_objectTableOffset];
	_actionTable = &_data[_actionTableOffset];
	_variableTable = &_data[_variableTableOffset];
	_saveArea = &_data[_saveAreaOffset];
	_dataSpace = &_data[_dataSpaceOffset];
	_codeSpace = &_data[_codeSpaceOffset];

	_wordCount = READ_LE_UINT16(_wordTable);
	_objectCount = READ_LE_UINT16(_objectTable);
	_actionCount = READ_LE_UINT16(_actionTable);
	_variableCount = READ_LE_UINT16(_variableTable);

	setVariable(V_OCOUNT, _objectCount);

	return true;
}

void Game::restart(Common::SeekableReadStream& s) {
	s.seek(_residentOffset + _saveAreaOffset);
	s.read(_saveArea, _saveSize);
	decrypt(_saveArea, _saveSize);

	setVariable(V_OCOUNT, _objectCount);
}

void Game::saveGameData(Common::WriteStream& ws) {
	ws.write(_saveArea, _saveSize);
}

void Game::loadGameData(Common::ReadStream& rs) {
	rs.read(_saveArea, _saveSize);
}

int Game::findWord(const Common::String &word) const {
	// Limit the word to the maximum allowable size
	Common::String w(word.c_str(), word.c_str() + WORD_SIZE);

	// Iterate over the dictionary for the word
	for (int idx = 1; idx <= _wordCount; ++idx) {
		int wordOffset = READ_LE_UINT16(_wordTable + idx * 2);
		if (w == (const char*)_residentBase + wordOffset + 2)
			return READ_LE_UINT16(_residentBase + wordOffset);
	}

	return NIL;
}

bool Game::match(int obj, int noun, int* adjectives) {
	if (!hasNoun(obj, noun))
		return false;

	for (int* adjPtr = adjectives; *adjPtr; ++adjPtr) {
		if (!hasAdjective(obj, *adjPtr))
			return false;
	}

	return true;
}

int Game::checkVerb(int* verbs) {
	// Iterate through the actions
	for (int idx = 1; idx <= _actionCount; ++idx) {
		if (hasVerb(idx, verbs))
			return idx;
	}

	return NIL;
}

int Game::findAction(int* verbs, int preposition, int flag) {
	// Iterate through the actions
	for (int idx = 1; idx <= _actionCount; ++idx) {
		if ((preposition && !hasPreposition(idx, preposition)) || !hasVerb(idx, verbs))
			continue;

		int mask = ~getActionByte(idx, A_MASK);
		if ((flag & mask) == (getActionByte(idx, A_FLAG) & mask))
			return idx;
	}

	return NIL;
}

int Game::getObjectProperty(int obj, int prop) {
	int field;

	for (; obj; obj = getObjectField(obj, O_CLASS)) {
		if ((field = findProperty(obj, prop)) != 0)
			return getObjectField(obj, field);
	}

	return NIL;
}

void Game::setObjectProperty(int obj, int prop, int val) {
	int field;

	for (; obj; obj = getObjectField(obj, O_CLASS)) {
		if ((field = findProperty(obj, prop)) != 0)
			return setObjectField(obj, field, val);
	}
}

int Game::getObjectLocation(int obj) const {
	if (obj < 1 || obj > _objectCount)
		error("Invalid object number %d", obj);

	return READ_LE_UINT16(_objectTable + obj * 2);
}

int Game::getActionLocation(int action) const {
	if (action < 1 || action >= _actionCount)
		error("Invalid action number %d", action);

	return READ_LE_UINT16(_actionTable + action * 2);
}

int Game::getVariable(int variableNum) {
	assert(variableNum < _variableCount);
	return READ_LE_UINT16(_variableTable + variableNum * 2);
}

void Game::setVariable(int variableNum, int value) {
	assert(variableNum < _variableCount);
	WRITE_LE_UINT16(_variableTable + variableNum * 2, value);
}

int Game::findProperty(int obj, int prop) const {
	int nProp = getObjectField(obj, O_NPROPERTIES);

	for (int idx = 0, p = 0; idx < nProp; ++idx, p += 4) {
		if ((getObjectField(obj, O_PROPERTIES + p) & ~P_CLASS) == prop)
			return O_PROPERTIES + p + 2;
	}

	return NIL;
}

bool Game::hasNoun(int obj, int noun) const {
	for (; obj; obj = getObjectField(obj, O_CLASS)) {
		if (inList(getObjectField(obj, O_NOUNS), noun))
			return true;
	}

	return false;
}

bool Game::hasAdjective(int obj, int adjective) const {
	for (; obj; obj = getObjectField(obj, O_CLASS)) {
		if (inList(getObjectField(obj, O_ADJECTIVES), adjective))
			return true;
	}

	return false;
}

bool Game::hasVerb(int act, int* verbs) const {
	// Get the list of verbs
	int link = getActionField(act, A_VERBS);

	// Look for the verb
	for (; link; link = readWord(link + L_NEXT)) {
		int* verb = verbs;
		int word = readWord(link + L_DATA);

		for (; *verb && word; link = readWord(link + L_NEXT)) {
			if (*verb != readWord(word + L_DATA))
				break;

			++verb;
		}

		if (!*verb && !word)
			return true;
	}

	return true;
}

bool Game::inList(int link, int word) const {
	for (; link; link = readWord(link + L_NEXT)) {
		if (word == readWord(link + L_DATA))
			return true;
	}

	return false;
}

} // End of namespace AdvSys
} // End of namespace Glk
