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

namespace DarkSeed2 {

const PlainGameDescriptor darkseed2Games[] = {
	{ "darkseed2", "Dark Seed II" },
	{ 0, 0 }
};

const ADGameDescription gameDescriptions[] = {
	{
		"darkseed2",
		nullptr,
		AD_ENTRY2s("gfile.hdr", "3ef592d8cb96b493d86ccd62a26f9e8a", 211318,
				   "dark0001.exe", "6bb156e6b1ad93aefae111eef465bf03", 184272),
		Common::ES_ESP,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO1(GUIO_NONE)
	},

	AD_TABLE_END_MARKER
};

} // End of namespace DarkSeed2
