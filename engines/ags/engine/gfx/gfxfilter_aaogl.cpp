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

#include "ags/shared/core/platform.h"

#if AGS_PLATFORM_OS_WINDOWS || AGS_PLATFORM_OS_ANDROID || AGS_PLATFORM_OS_IOS || AGS_PLATFORM_OS_LINUX

#include "ags/shared/gfx/gfxfilter_aaogl.h"
#include "ags/shared/ogl_headers.h"

namespace AGS3 {
namespace AGS {
namespace Engine {
namespace OGL {

const GfxFilterInfo AAOGLGfxFilter::FilterInfo = GfxFilterInfo("Linear", "Linear interpolation");

bool AAOGLGfxFilter::UseLinearFiltering() const {
	return true;
}

void AAOGLGfxFilter::SetFilteringForStandardSprite() {
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

const GfxFilterInfo &AAOGLGfxFilter::GetInfo() const {
	return FilterInfo;
}

} // namespace OGL
} // namespace Engine
} // namespace AGS
} // namespace AGS3

#endif // only on Windows, Android and iOS
