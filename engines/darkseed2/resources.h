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

#ifndef DARKSEED2_RESOURCES_H
#define DARKSEED2_RESOURCES_H

#include "common/file.h"
#include "common/util.h"
#include "common/str.h"
#include "common/hashmap.h"

#include "darkseed2/versionformats.h"

namespace DarkSeed2 {


class Archive;

typedef Common::HashMap<Common::String, Archive *, Common::IgnoreCase_Hash, Common::IgnoreCase_EqualTo> ResourceMap;

/** An archive file. */
class Archive {
public:
	Archive();
	virtual ~Archive() {}

	/** Open the file, returns success */
	virtual bool open(const Common::String &fileName, Archive *parentArchive = 0) = 0;

	/** Index the resources in this Archive */
	virtual void index(ResourceMap &map) = 0;

	/** Get the resource stream, returns 0 upon failure */
	virtual Common::SeekableReadStream *getStream(const Common::String &fileName) = 0;

	/** Has the archive already been indexed? */
	bool isIndexed() const { return _isIndexed; }

	/** Get the file name of the archive */
	Common::String getFileName() const { return _fileName; }

	/** Clear uncompressed data */
	virtual void clearUncompressedData() {}

protected:
	bool _isIndexed;
	Common::String _fileName;
};

class GlueArchive : public Archive {
public:
	GlueArchive();
	~GlueArchive();

	bool open(const Common::String &fileName, Archive *parentArchive = 0);
	void index(ResourceMap &map);
	Common::SeekableReadStream *getStream(const Common::String &fileName);
	void clearUncompressedData();

private:
	struct ResourceEntry {
		Common::String fileName;
		uint32 offset;
		uint32 size;
	};

	Common::Array<ResourceEntry> _resources;

	Common::SeekableReadStream *_file;

	/** Uncompress a glue file. */
	void uncompressGlue();
	/** Uncompress a compress glue file chunk. */
	uint32 uncompressGlueChunk(byte *outBuf, const byte *inBuf, int n) const;

	/** Simple heuristic to guess whether a glue file is compressed. */
	bool isCompressed();
};


/** The resource manager. */
class Resources {
public:
	Resources();
	~Resources();

	/** Index all resources, based on a resource index file. */
	bool index(const char *fileName);

	/** Clear all resource information. */
	void clear();

	/** Does a specific resource exist? */
	bool hasResource(const Common::String &resource);

	/** Get a specific resource. */
	Common::SeekableReadStream *getResource(const Common::String &resource);

	/** Set the specific game version. */
	void setGameVersion(GameVersion gameVersion, Common::Language language);

	/** Get the information class about which formats the game uses. */
	const VersionFormats &getVersionFormats();

	static Common::String addExtension(const Common::String &name, const Common::String &extension);

	/** List all resources and glue files (for debugging) */
	uint32 getArchiveCount() const { return _archives.size(); }
	uint32 getResourceCount() const { return _resources.size(); }
private:
	VersionFormats _versionFormats;

	/** All indexed archives. */
	Common::Array<Archive *> _archives;
	/** All indexed resources. */
	ResourceMap _resources;

	/** Read the index file's header. */
	bool readIndexHeader(Common::File &indexFile, uint16 &resCount);
	/** Read the glue file section of the index file. */
	bool readIndexGlues(Common::File &indexFile);
	/** Read the resources section of the index file. */
	bool readIndexResources(Common::File &indexFile, uint16 resCount);
};

} // End of namespace DarkSeed2

#endif // DARKSEED2_RESOURCES_H
