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

#include "m4/core/rooms.h"
#include "m4/core/errors.h"
#include "m4/adv_r/adv_been.h"
#include "m4/adv_r/adv_file.h"
#include "m4/adv_r/adv_interface.h"
#include "m4/adv_r/adv_walk.h"
#include "m4/adv_r/db_env.h"
#include "m4/fileio/extensions.h"
#include "m4/graphics/krn_pal.h"
#include "m4/gui/gui_buffer.h"
#include "m4/gui/gui_vmng.h"
#include "m4/vars.h"
#include "m4/m4.h"

namespace M4 {

constexpr int kScaleEditor = 1;

HotSpotRec *Section::walker_spotter(int32 x, int32 y) {
	warning("TODO: walker_spotter");
	return nullptr;
}

void Sections::global_section_constructor() {
	uint sectionNum = _G(game).new_section;
	assert(sectionNum >= 1 && sectionNum <= 9);

	_activeSection = &_sections[sectionNum - 1];
}

void Sections::section_room_constructor() {
	_activeRoom = (*_activeSection)[_G(game).new_room];
}

void Sections::m4SceneLoad() {
	_G(between_rooms) = true;
	cameraShiftAmount = 0;
	cameraShift_vert_Amount = 0;
	_G(art_base_override) = nullptr;
	_G(use_alternate_attribute_file) = true;
	shut_down_digi_tracks_between_rooms = true;
	camera_pan_step = 10;
	_G(camera_reacts_to_player) = true;

	_G(kernel).force_restart = false;
	player_set_defaults();
	player_set_commands_allowed(false);		// Also sets "Wait" cursor

	// -------------------- SECTION CONSTRUCTOR and ROOM PRELOAD ------------------

	section_room_constructor();
	_G(kernel).supress_fadeup = false;

	room_preload();

	// -------------------- ROOM LOAD ------------------

	_GI().cancel_sentence();
	gr_pal_clear_range(_G(master_palette), _G(kernel).first_fade, 255);

	term_message("Calling kernel_load_room");

	_G(kernel).going = kernel_load_room(_G(kernel).minPalEntry, _G(kernel).maxPalEntry,
		&_G(currentSceneDef), &_G(screenCodeBuff), &_G(game_bgBuff));
	if (!_G(kernel).going)
		error_show(FL, 'IMP!');	// this should never ever happen

	get_ipl();

	// Must reset event handler because loading a room re-initalizes gameBuff
	gui_buffer_set_event_handler(_G(gameDrawBuff), intr_EventHandler);

	if (_G(player).walker_in_this_scene)
		get_walker();

	_G(kernel).trigger_mode = KT_DAEMON;
	_G(kernel).call_daemon_every_loop = false;
	_G(kernel).fade_up_time = 30;

	//-------------------- GLOBAL ROOM INIT and ROOM INIT ------------------

	player_set_commands_allowed(false);
	_G(set_commands_allowed_since_last_checked) = false;
	_G(between_rooms) = false;

	room_init();
	_G(player).walker_trigger = -1;



	if (_G(game).previous_room == KERNEL_RESTORING_GAME) {
		if (_G(player).walker_in_this_scene) {
			// If restoring game, restore player position and facing
			player_demand_location(_G(player_info).x, _G(player_info).y);
			player_demand_facing(_G(player_info).facing);
		}

		// Restore camera position
		MoveScreenAbs(_G(game_buff_ptr), _G(player_info).camera_x, _G(player_info).camera_y);
	}

	_visited_room = player_been_here(_G(game).room_id);

	term_message("calling room_init_code");
	g_engine->room_init();

	if (_G(game).previous_room == KERNEL_RESTORING_GAME) {
		_G(game).previous_room = -1;
	}

	// Init for fade up screen
	if (!_G(kernel).supress_fadeup) {
		pal_fade_set_start(&_G(master_palette)[0], 0);	// Set fade to black instantly (0 ticks)
		pal_fade_init(&_G(master_palette)[0], _G(kernel).first_fade, 255, 100,
			_G(kernel).fade_up_time, 32765);      // 30 ticks
	}

	if (!_G(set_commands_allowed_since_last_checked))
		player_set_commands_allowed(true);

	//-------------------- PLAY ROOM ------------------

	term_message("Off to the races -- %d", timer_read_60());
}

void Sections::m4SceneRun() {
	game_control_cycle();

	if (!player_been_here(_G(game).room_id))
		player_enters_scene(_G(game).room_id);


}

void Sections::get_ipl() {
	if (_G(inverse_pal))
		delete _G(inverse_pal);
	_G(inverse_pal) = NULL;

	char *name;
	Common::String filename;

	name = env_find(_G(currentSceneDef).art_base);
	if (name) {
		// Means found in database
		filename = f_extension_new(name, "ipl");

	} else {
		// Concat hag mode
		filename = Common::String::format("%s.IPL", _G(currentSceneDef).art_base);
	}

	_G(inverse_pal) = new InvPal(filename.c_str());
	if (!_G(inverse_pal))
		error_show(FL, 'OOM!', "loading ipl: %s", filename.c_str());
}

void Sections::get_walker() {
	term_message("Loading walker sprites");
	if (!_GW().walk_load_walker_and_shadow_series())
		error_show(FL, 'WLOD');
	ws_walk_init_system();
}

void Sections::game_control_cycle() {
	// TODO
}

/*------------------------------------------------------------------------*/

Room *Section::operator[](uint roomNum) {
	for (uint i = 0; i < _roomsCount; ++i) {
		if (_rooms[i]._roomNum == roomNum)
			return &_rooms[i];
	}

	error("Unknown room number - %d", roomNum);
}


} // namespace M4
