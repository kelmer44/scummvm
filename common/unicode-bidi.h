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

#ifndef COMMON_STRING_BIDI_H
#define COMMON_STRING_BIDI_H

#include "common/str.h"
#include "common/ustr.h"
#include "common/language.h"


// SCUMMVM_FRIBIDI_PAR_ON: automatically check the text's direction
// SCUMMVM_FRIBIDI_PAR_LTR, SCUMMVM_FRIBIDI_PAR_RTL: enforce LTR or RTL direction
// if not USE_FRIBIDI, these defines values don't matter
#ifdef USE_FRIBIDI
#define SCUMMVM_FRIBIDI_PAR_ON			FRIBIDI_PAR_ON
#define SCUMMVM_FRIBIDI_PAR_LTR			FRIBIDI_PAR_LTR
#define SCUMMVM_FRIBIDI_PAR_RTL			FRIBIDI_PAR_RTL
#else
#define SCUMMVM_FRIBIDI_PAR_ON			0
#define SCUMMVM_FRIBIDI_PAR_LTR			0
#define SCUMMVM_FRIBIDI_PAR_RTL			0
#endif


namespace Common {

class UnicodeBiDiText {
private:
	uint32 *_log_to_vis_index; // from fribidi conversion
	uint32 *_vis_to_log_index; // from fribidi conversion
	void initWithU32String(const Common::U32String &str);
	Common::String bidiByLine(Common::String line, va_list args);
public:
	const Common::U32String logical; // original string, ordered logically
	Common::U32String visual; // from fribidi conversion, ordered visually
	uint32 _pbase_dir;

	UnicodeBiDiText(const Common::U32String &str);
	UnicodeBiDiText(const Common::String &str, const Common::CodePage page);
	UnicodeBiDiText(const Common::String &str, const Common::CodePage page, uint32 *pbase_dir);
	~UnicodeBiDiText();

	/**
	 * Implicit conversion to U32String to get the visual representation.
	 */
	operator const U32String &() const { return visual; }

	uint32 getVisualPosition(uint32 logicalPos) const;
	uint32 getLogicalPosition(uint32 visualPos) const;
	uint32 size() const { return logical.size(); }
};

/* just call the constructor for convenience */
UnicodeBiDiText convertBiDiU32String(const U32String &input);
String convertBiDiString(const String &input, const Common::Language lang);
String convertBiDiString(const String &input, const Common::CodePage page);

// calls convertBiDiString for each line in isolation
String convertBiDiStringByLines(const String &input, const Common::CodePage page);

} // End of namespace Common

#endif
