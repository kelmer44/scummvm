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

#ifndef __SPLASH_SCREEN_H__
#define __SPLASH_SCREEN_H__

#include "graphics/surface.h"

namespace QDEngine {

class SplashScreen {
public:
	SplashScreen() : splash_hwnd_(NULL), bitmap_handle_(NULL), start_time_(0) { }
	~SplashScreen() {
		destroy();
	}

	bool create(int bitmap_resid);
	bool create(const char *bitmap_file);

	bool set_mask(int mask_resid);
	bool set_mask(const char *mask_file);

	bool destroy();

	void show();
	void hide();

	void wait(int time);

private:

	int start_time_;

	Graphics::Surface *splash_hwnd_;
	void *bitmap_handle_;

	bool create_window();
	void apply_mask(void *mask_handle);
};

} // namespace QDEngine

#endif // __SPLASH_SCREEN_H__
