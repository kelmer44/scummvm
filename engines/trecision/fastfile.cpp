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

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "common/scummsys.h"
#include "common/str.h"
#include "common/substream.h"
#include "common/memstream.h"
#include "trecision/trecision.h"
#include "trecision/fastfile.h"
#include "trecision/video.h"

namespace Trecision {

/* -----------------19/01/98 17.12-------------------
 * Compare
 *
 * bsearch comparison routine
 * --------------------------------------------------*/
int Compare(const void *p1, const void *p2) {
	SFileEntry *p1c = (SFileEntry *)p1, *p2c = (SFileEntry*)p2;
	return (scumm_stricmp((p1c)->name, (p2c)->name));
}

FastFile::FastFile() : Common::Archive(), _stream(nullptr), _compBuffer(nullptr), _compStream(nullptr) {
}

FastFile::~FastFile() {
	close();
}

const FastFile::FileEntry *FastFile::getEntry(const Common::String &name) const {
	FileEntry key;
	strncpy(key.name, name.c_str(), ARRAYSIZE(key.name));

	FileEntry *entry = (FileEntry *)bsearch(&key, &_fileEntries[0], _fileEntries.size(), sizeof(FileEntry), Compare);
	return entry;
}

bool FastFile::open(const Common::String &name) {
	close();

	_stream = SearchMan.createReadStreamForMember(name);
	if (!_stream)
		return false;

	int numFiles = _stream->readUint32LE();
	_fileEntries.resize(numFiles);
	for (int i = 0; i < numFiles; i++) {
		FileEntry *entry = &_fileEntries[i];
		_stream->read(entry->name, ARRAYSIZE(entry->name));
		entry->offset = _stream->readUint32LE();
	}

	return true;
}

void FastFile::close() {
	delete _stream;
	_stream = nullptr;
	delete _compStream;
	_compStream = nullptr;
	_fileEntries.clear();
}

bool FastFile::hasFile(const Common::String &name) const {
	const FileEntry *entry = getEntry(name);
	return entry != nullptr;
}

int FastFile::listMembers(Common::ArchiveMemberList &list) const {
	list.clear();
	for (Common::Array<FileEntry>::const_iterator i = _fileEntries.begin(); i != _fileEntries.end(); i++)
		list.push_back(getMember(i->name));

	return list.size();
}

const Common::ArchiveMemberPtr FastFile::getMember(const Common::String &name) const {
	return Common::ArchiveMemberPtr(new Common::GenericArchiveMember(name, this));
}

Common::SeekableReadStream *FastFile::createReadStreamForMember(const Common::String &name) const {
	if (!_stream)
		return nullptr;

	const FileEntry *entry = getEntry(name);
	if (entry) {
		uint32 size = (entry + 1)->offset - entry->offset;
		return new Common::SeekableSubReadStream(_stream, entry->offset, entry->offset + size);
	}

	return nullptr;
}

void FastFile::decompress(const unsigned char *src, uint32 srcSize, unsigned char *dst, uint32 decompSize) {
	uint16 *sw = (uint16 *)(src + srcSize);
	uint8 *d = dst;
	uint32 bytesWritten = 0;
	const uint8 *s = src;
	unsigned short ctrl = 0, ctrl_cnt = 1;

	while (s < (const uint8 *)sw) {
		if (!--ctrl_cnt) {
			ctrl = READ_LE_UINT16(--sw);
			ctrl_cnt = 16;
		} else {
			ctrl <<= 1;
		}

		if (ctrl & 0x8000) {
			uint16 foo = READ_LE_UINT16(--sw);
			const uint8 *cs = d - (foo >> 4);

			uint16 num = 16 - (foo & 0xF);

			for (uint16 i = 0; i < num; ++i) {
				*d++ = *cs++;
				bytesWritten++;
				assert(bytesWritten <= decompSize);
			}

			*d++ = *cs++;
			*d++ = *cs;
			bytesWritten += 2;
			assert(bytesWritten <= decompSize);
		} else {
			*d++ = *s++;
			bytesWritten++;
			assert(bytesWritten <= decompSize);
		}
	}
}

#define FAST_COOKIE 0xFA57F00D
Common::SeekableReadStream *FastFile::createReadStreamForCompressedMember(const Common::String &name) {
	Common::SeekableReadStream *ff = createReadStreamForMember(name);
	if (ff == nullptr)
		error("createReadStreamForCompressedMember - File not found %s", name.c_str());

	int32 dataSize = ff->size() - 8;

	uint32 signature = ff->readUint32LE();
	if (signature != FAST_COOKIE)
		error("createReadStreamForCompressedMember - %s has a bad signature and can't be loaded", name.c_str());

	int32 decompSize = ff->readSint32LE();

	uint8 *ibuf = new uint8[dataSize];
	int32 realSize = MAX(dataSize, decompSize) + 8 + 100;	// add extra padding for the decompressor

	delete _compStream;
	_compBuffer = new uint8[realSize];

	ff->read(ibuf, dataSize);
	delete ff;

	if (dataSize < decompSize)
		decompress(ibuf, dataSize, _compBuffer, realSize);
	else
		memcpy(_compBuffer, ibuf, dataSize);

	delete[] ibuf;

	_compStream = new Common::MemoryReadStream(_compBuffer, realSize, DisposeAfterUse::YES);
	return _compStream;
}
} // End of namespace Trecision
