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


#ifndef QDENGINE_CORE_PARSER_QDSCR_PARSER_H
#define QDENGINE_CORE_PARSER_QDSCR_PARSER_H

#include "qdengine/core/parser/xml_fwd.h"

namespace QDEngine {

//! Идентификаторы тэгов скрипта.
enum qdscrTagID {
	QDSCR_ROOT = 1,

	// Common
	QDSCR_ID,
	QDSCR_X,
	QDSCR_Y,
	QDSCR_SX,					// 5
	QDSCR_SY,
	QDSCR_POS2D,
	QDSCR_POS3D,
	QDSCR_SRC_POS,
	QDSCR_DEST_POS,				// 10
	QDSCR_FILE,
	QDSCR_START_TIME,
	QDSCR_PERIOD,
	QDSCR_LENGTH,
	QDSCR_SPEED,				// 15
	QDSCR_ANIMATION_SPEED,
	QDSCR_INTERPOLATION_TIME,
	QDSCR_SCALE,
	QDSCR_NAME,
	QDSCR_FLAG,					// 20
	QDSCR_RND,
	QDSCR_MASK_SIZE,
	QDSCR_MASK_ATTRIBUTES,
	QDSCR_MASK_HEIGHTS,
	QDSCR_BOUND,				// 25
	QDSCR_TYPE,
	QDSCR_SIZE,
	QDSCR_STATE,
	QDSCR_TEXT,
	QDSCR_SHORT_TEXT,			// 30
	QDSCR_VALUE,
	QDSCR_TIME,
	QDSCR_CD,
	QDSCR_ALIGN,
	QDSCR_VALIGN,				// 35
	QDSCR_ORIENTATION,
	QDSCR_COLOR,
	QDSCR_COMMENT,

	QDSCR_SCREEN_SIZE,
	QDSCR_TEXT_SET,				// 40

	// Text
	QDSCR_TEXT_COLOR,
	QDSCR_TEXT_HOVER_COLOR,
	QDSCR_TEXT_ALIGN,

	// Fonts
	QDSCR_FONT_INFO,

	// Contours
	QDSCR_CONTOUR_RECTANGLE,	// 45
	QDSCR_CONTOUR_CIRCLE,
	QDSCR_CONTOUR_POLYGON,

	// Named Object Reference
	QDSCR_NAMED_OBJECT,
	QDSCR_NAMED_OBJECT_TYPES,

	// Grid zone
	QDSCR_GRID_ZONE,			// 50
	QDSCR_GRID_ZONE_CELLS,
	QDSCR_GRID_ZONE_HEIGHT,
	QDSCR_GRID_ZONE_CONTOUR,
	QDSCR_GRID_ZONE_STATE,
	QDSCR_GRID_ZONE_SHADOW_COLOR,	// 55
	QDSCR_GRID_ZONE_SHADOW_ALPHA,

	// Trigger Element
	QDSCR_TRIGGER_ELEMENT,
	QDSCR_TRIGGER_ELEMENT_LINK,
	QDSCR_TRIGGER_ELEMENT_PARENT_LINKS,
	QDSCR_TRIGGER_ELEMENT_CHILD_LINKS,			// 60
	QDSCR_TRIGGER_ELEMENT_LINK_AUTO_RESTART,
	QDSCR_TRIGGER_ELEMENT_LINK_OFFSETS,
	QDSCR_TRIGGER_ELEMENT_TITLE,
	QDSCR_TRIGGER_ELEMENT_CELL_NUMBER,
	QDSCR_TRIGGER_PARENT_LINK_OWNER_OFFSET,		// 65
	QDSCR_TRIGGER_PARENT_LINK_CHILD_OFFSET,
	QDSCR_TRIGGER_CHILD_LINK_OWNER_OFFSET,
	QDSCR_TRIGGER_CHILD_LINK_CHILD_OFFSET,
	QDSCR_TRIGGER_START_ELEMENT,

	// Trigger Chain
	QDSCR_TRIGGER_CHAIN,						// 70
	QDSCR_TRIGGER_CHAIN_ROOT,
	QDSCR_TRIGGER_CHAIN_WORK_AREA,
	QDSCR_TRIGGER_CHAIN_LAYOUT,

	QDSCR_TRIGGER_BOUND,

	// Sound
	QDSCR_SOUND,					// 75
	QDSCR_SOUND_VOLUME,

	// Music
	QDSCR_MUSIC_TRACK,
	QDSCR_MUSIC_TRACK_CYCLED,

	// Video
	QDSCR_VIDEO,
	QDSCR_VIDEO_POSITION,			// 80
	QDSCR_VIDEO_BACKGROUND,

	// Minigame
	QDSCR_MINIGAME,
	QDSCR_MINIGAME_DLL_NAME,
	QDSCR_MINIGAME_GAME_NAME,
	QDSCR_MINIGAME_CONFIG_FILE,		// 85
	QDSCR_MINIGAME_STATE,
	QDSCR_MINIGAME_SCORE,
	QDSCR_MINIGAME_CONFIG_PARAMETER,

	// Animation
	QDSCR_ANIMATION_FRAME,
	QDSCR_ANIMATION_INFO,			// 90
	QDSCR_ANIMATION_FILE,
	QDSCR_ANIMATION,

	// AnimationSet
	QDSCR_ANIMATION_SET,
	QDSCR_ANIMATION_SET_START_ANGLE,
	QDSCR_ANIMATION_SET_TURN,		// 95

	// Coords Animation
	QDSCR_COORDS_ANIMATION_POINT,
	QDSCR_COORDS_ANIMATION,
	QDSCR_ANIMATION_PHASE,

	// Game Object
	QDSCR_PARALLAX_OFFSET,

	// Static Object
	QDSCR_STATIC_OBJECT,			// 100

	// Animated Object
	QDSCR_ANIMATED_OBJECT,
	QDSCR_OBJECT_STATE,
	QDSCR_OBJECT_MOVEMENT_STATES,
	QDSCR_OBJECT_MOVEMENT_STATE,

	QDSCR_OBJECT_DIRECTION,			// 105
	QDSCR_OBJECT_DEFAULT_POS,
	QDSCR_OBJECT_DEFAULT_DIRECTION,

	QDSCR_OBJECT_STATE_STATIC,
	QDSCR_OBJECT_STATE_WALK,
	QDSCR_OBJECT_STATE_MASK,		// 110
	QDSCR_OBJECT_STATE_MASK_POS,
	QDSCR_OBJECT_STATE_MASK_SIZE,
	QDSCR_OBJECT_STATE_MASK_DATA,
	QDSCR_OBJECT_STATE_MASK_PARENT,
	QDSCR_OBJECT_STATE_MASK_CONTOUR,	// 115
	QDSCR_OBJECT_STATE_CENTER_OFFSET,
	QDSCR_OBJECT_STATE_DELAY,
	QDSCR_OBJECT_STATE_SOUND_DELAY,
	QDSCR_OBJECT_STATE_TEXT_DELAY,
	QDSCR_OBJECT_STATE_CURSOR_ID,		// 120
	QDSCR_OBJECT_STATE_ACCELERATION,
	QDSCR_OBJECT_STATE_RND_MOVE,

	QDSCR_OBJECT_STATE_WALK_SOUND_FREQUENCY,

	QDSCR_STATE_CENTER_OFFSETS,
	QDSCR_STATE_STATIC_CENTER_OFFSETS,	// 125
	QDSCR_STATE_START_CENTER_OFFSETS,
	QDSCR_STATE_STOP_CENTER_OFFSETS,

	// State conditions
	QDSCR_CONDITION,
	QDSCR_CONDITION_DATA_INT,
	QDSCR_CONDITION_DATA_FLOAT,			// 130
	QDSCR_CONDITION_DATA_STRING,
	QDSCR_CONDITION_INVERSE,
	QDSCR_CONDITIONS_MODE,

	QDSCR_CONDITION_GROUP,
	QDSCR_CONDITION_OBJECT,				// 135

	// Counter
	QDSCR_COUNTER,
	QDSCR_COUNTER_ELEMENT,
	QDSCR_COUNTER_INC_VALUE,
	QDSCR_COUNTER_LIMIT,
	QDSCR_COUNTER_DIGITS,				// 140
	QDSCR_COUNTER_NAME,

	// Object Scale
	QDSCR_OBJ_SCALE_INFO,

	// Mouse object
	QDSCR_MOUSE_OBJECT,
	QDSCR_MOUSE_DEFAULT_CURSORS,

	// Moving Object
	QDSCR_MOVEMENT_ANIMATION_NUMBER,	// 145
	QDSCR_MOVEMENT_ANIMATIONS,
	QDSCR_STATIC_ANIMATIONS,
	QDSCR_MOVING_OBJECT,
	QDSCR_COLLISION_RADIUS,
	QDSCR_COLLISION_DELAY,				// 150
	QDSCR_COLLISION_PATH,
	QDSCR_FOLLOW_MIN_RADIUS,
	QDSCR_FOLLOW_MAX_RADIUS,
	QDSCR_ATTACH_SHIFT,
	QDSCR_PERSONAGE_CONTROL,			// 155
	QDSCR_PERSONAGE_MOVEMENT_TYPE,
	QDSCR_ROTATION_ANGLE_PER_QUANT,

	// Inventory
	QDSCR_INVENTORY_CELL_TYPE,
	QDSCR_INVENTORY_CELL_SET,
	QDSCR_INVENTORY_CELL_SET_SIZE,		// 160
	QDSCR_INVENTORY_CELL_SET_ADDITIONAL_CELLS,
	QDSCR_INVENTORY_CELL_SET_POS,
	QDSCR_INVENTORY,

	// Camera & grid
	QDSCR_CAMERA,
	QDSCR_CAMERA_GRID_SIZE,				// 165
	QDSCR_CAMERA_GRID_HEIGHT,
	QDSCR_CAMERA_GRID_ATTRIBUTES,
	QDSCR_CAMERA_GRID_CENTER,
	QDSCR_CAMERA_CELL_SIZE,
	QDSCR_CAMERA_FOCUS,					// 170
	QDSCR_CAMERA_ANGLES,
	QDSCR_CAMERA_SCREEN_SIZE,
	QDSCR_CAMERA_SCREEN_OFFSET,
	QDSCR_CAMERA_SCREEN_CENTER,
	QDSCR_CAMERA_MODE,					// 175
	QDSCR_CAMERA_SCROLLING_SPEED,
	QDSCR_CAMERA_SCROLLING_DIST,
	QDSCR_CAMERA_SMOOTH_SWITCH,
	QDSCR_CAMERA_SCALE_POW,
	QDSCR_CAMERA_SCALE_Z_OFFSET,		// 180

	// Game end
	QDSCR_GAME_END,
	QDSCR_GAME_END_SCREEN,

	// Interface
	QDSCR_INTERFACE,

	QDSCR_INTERFACE_SCENE_REDRAW,

	QDSCR_INTERFACE_MAIN_MENU,			// 185
	QDSCR_INTERFACE_INGAME_SCREEN0,
	QDSCR_INTERFACE_INGAME_SCREEN1,
	QDSCR_INTERFACE_HIGHSCORE_NAME_SCREEN,
	QDSCR_INTERFACE_SAVE_PROMPT_SCREEN,
	QDSCR_INTERFACE_SAVE_NAME_SCREEN,	// 190

	QDSCR_INTERFACE_NEED_SAVE_SCREENSHOT,
	QDSCR_INTERFACE_NEED_SHOW_SAVE_NAME,
	QDSCR_INTERFACE_NEED_SHOW_SAVE_TIME,
	QDSCR_INTERFACE_SAVE_FONT_TYPE,
	QDSCR_INTERFACE_SAVE_FONT_COLOR,	// 195
	QDSCR_INTERFACE_SAVE_IS_AUTOSAVE,

	QDSCR_INTERFACE_SCREEN,
	QDSCR_INTERFACE_SCREEN_HIDE_TIME,
	QDSCR_INTERFACE_SCREEN_HIDE_OFFSET,
	QDSCR_INTERFACE_ELEMENT,			// 200

	QDSCR_INTERFACE_ELEMENT_STATE,
	QDSCR_INTERFACE_ELEMENT_STATE_MODE,
	QDSCR_INTERFACE_ELEMENT_POS,

	QDSCR_INTERFACE_OPTION_ID,

	QDSCR_INTERFACE_EVENT,				// 205
	QDSCR_INTERFACE_EVENT_DATA,
	QDSCR_INTERFACE_EVENT_BEFORE_ANIMATION,
	QDSCR_INTERFACE_EVENT_ACTIVATION_TYPE,

	QDSCR_INTERFACE_ACTION,
	QDSCR_INTERFACE_ACTION_DATA,		// 210

	QDSCR_INTERFACE_ANIMATION_FLAGS,
	QDSCR_INTERFACE_SLIDER_RECTANGLE,
	QDSCR_INTERFACE_SLIDER_ORIENTATION,
	QDSCR_INTERFACE_BACKGROUND_OFFSET,

	QDSCR_INTERFACE_THUMBNAIL_SIZE,		// 215
	QDSCR_INTERFACE_TEXT_SHIFT,

	QDSCR_TEXT_WINDOW_BORDER_CORNER,
	QDSCR_TEXT_WINDOW_BORDER_LINE_H,
	QDSCR_TEXT_WINDOW_BORDER_LINE_V,
	QDSCR_TEXT_WINDOW_BORDER_BACK,		// 220
	QDSCR_TEXT_WINDOW_SLIDER,
	QDSCR_TEXT_WINDOW_TYPE,
	QDSCR_TEXT_WINDOW_MAX_STRING_LENGTH,
	QDSCR_TEXT_WINDOW_INFO_TYPE,
	QDSCR_TEXT_WINDOW_PLAYER_ID,		// 225

	// Scene
	QDSCR_SCENE,
	QDSCR_SCENE_SAVE_SLOT,
	QDSCR_STARTUP_SCENE,
	QDSCR_SCENE_RESTART_MINIGAME,

	// Location
	QDSCR_LOCATION,						// 230

	QDSCR_GAME_TITLE,
	QDSCR_TEXT_DB,
	QDSCR_CD_KEY,

	// For qdScreenTextFormat
	QDSCR_SCREEN_TEXT_FORMAT,
	QDSCR_FONT_TYPE,					// 235

	QDSCR_DEFAULT_FONT,
	QDSCR_GLOBAL_DEPEND,
	QDSCR_INVERSE_DIRECTION,
	QDSCR_ALPHA,
	QDSCR_ENABLE_BACKGROUND,			// 240
	QDSCR_FADE_TIME,

	QDSCR_HALL_OF_FAME_SIZE,

	QDSCR_RESOURCE_COMPRESSION,

	QDSCR_SCREEN_TRANSFORM,

	QDSCR_MAX_KEYWORD_ID				// 245
};

const int idTagVersionAll[490] = {
	20021114, QDSCR_ROOT,

	// Common
	20021114, QDSCR_ID,
	20021114, QDSCR_X,
	20021114, QDSCR_Y,
	20021114, QDSCR_SX,					// 5
	20021114, QDSCR_SY,
	20021114, QDSCR_POS2D,
	20021114, QDSCR_POS3D,
	20021114, QDSCR_SRC_POS,
	20021114, QDSCR_DEST_POS,				// 10
	20021114, QDSCR_FILE,
	20021114, QDSCR_START_TIME,
	20021114, QDSCR_PERIOD,
	20021114, QDSCR_LENGTH,
	20021114, QDSCR_SPEED,				// 15
	20030422, QDSCR_ANIMATION_SPEED,
	20021114, QDSCR_INTERPOLATION_TIME,
	20021114, QDSCR_SCALE,
	20021114, QDSCR_NAME,
	20021114, QDSCR_FLAG,					// 20
	20021114, QDSCR_RND,
	20021114, QDSCR_MASK_SIZE,
	20021114, QDSCR_MASK_ATTRIBUTES,
	20021114, QDSCR_MASK_HEIGHTS,
	20021114, QDSCR_BOUND,				// 25
	20021114, QDSCR_TYPE,
	20021114, QDSCR_SIZE,
	20021114, QDSCR_STATE,
	20021114, QDSCR_TEXT,
	20021114, QDSCR_SHORT_TEXT,			// 30
	20021202, QDSCR_VALUE,
	20030125, QDSCR_TIME,
	20030822, QDSCR_CD,
	20030918, QDSCR_ALIGN,
	20070503, QDSCR_VALIGN,				// 35
	20030918, QDSCR_ORIENTATION,
	20031007, QDSCR_COLOR,
	20040601, QDSCR_COMMENT,

	20021114, QDSCR_SCREEN_SIZE,
	20030918, QDSCR_TEXT_SET,				// 40

	// Text
	20040123, QDSCR_TEXT_COLOR,
	20040210, QDSCR_TEXT_HOVER_COLOR,
	20040123, QDSCR_TEXT_ALIGN,

	// Fonts
	20040308, QDSCR_FONT_INFO,

	// Contours
	20021114, QDSCR_CONTOUR_RECTANGLE,	// 45
	20021114, QDSCR_CONTOUR_CIRCLE,
	20021114, QDSCR_CONTOUR_POLYGON,

	// Named Object Reference
	20021114, QDSCR_NAMED_OBJECT,
	20030430, QDSCR_NAMED_OBJECT_TYPES,

	// Grid zone
	20021114, QDSCR_GRID_ZONE,			// 50
	20021114, QDSCR_GRID_ZONE_CELLS,
	20021114, QDSCR_GRID_ZONE_HEIGHT,
	20021114, QDSCR_GRID_ZONE_CONTOUR,
	20021216, QDSCR_GRID_ZONE_STATE,
	20040315, QDSCR_GRID_ZONE_SHADOW_COLOR,	// 55
	20040315, QDSCR_GRID_ZONE_SHADOW_ALPHA,

	// Trigger Element
	20021114, QDSCR_TRIGGER_ELEMENT,
	20021114, QDSCR_TRIGGER_ELEMENT_LINK,
	20030429, QDSCR_TRIGGER_ELEMENT_PARENT_LINKS,
	20030429, QDSCR_TRIGGER_ELEMENT_CHILD_LINKS,			// 60
	20030227, QDSCR_TRIGGER_ELEMENT_LINK_AUTO_RESTART,
	20030429, QDSCR_TRIGGER_ELEMENT_LINK_OFFSETS,
	20021114, QDSCR_TRIGGER_ELEMENT_TITLE,
	20021114, QDSCR_TRIGGER_ELEMENT_CELL_NUMBER,
	20021114, QDSCR_TRIGGER_PARENT_LINK_OWNER_OFFSET,		// 65
	20021114, QDSCR_TRIGGER_PARENT_LINK_CHILD_OFFSET,
	20021114, QDSCR_TRIGGER_CHILD_LINK_OWNER_OFFSET,
	20021114, QDSCR_TRIGGER_CHILD_LINK_CHILD_OFFSET,
	20021204, QDSCR_TRIGGER_START_ELEMENT,

	// Trigger Chain
	20021114, QDSCR_TRIGGER_CHAIN,						// 70
	20021114, QDSCR_TRIGGER_CHAIN_ROOT,
	20021114, QDSCR_TRIGGER_CHAIN_WORK_AREA,
	20021114, QDSCR_TRIGGER_CHAIN_LAYOUT,

	20021114, QDSCR_TRIGGER_BOUND,

	// Sound
	20021114, QDSCR_SOUND,					// 75
	20030512, QDSCR_SOUND_VOLUME,

	// Music
	20021213, QDSCR_MUSIC_TRACK,
	20021213, QDSCR_MUSIC_TRACK_CYCLED,

	// Video
	20021114, QDSCR_VIDEO,
	20021114, QDSCR_VIDEO_POSITION,			// 80
	20021114, QDSCR_VIDEO_BACKGROUND,

	// Minigame
	20021114, QDSCR_MINIGAME,
	20021114, QDSCR_MINIGAME_DLL_NAME,
	20040601, QDSCR_MINIGAME_GAME_NAME,
	20021114, QDSCR_MINIGAME_CONFIG_FILE,		// 85
	20021114, QDSCR_MINIGAME_STATE,
	20021114, QDSCR_MINIGAME_SCORE,
	20040601, QDSCR_MINIGAME_CONFIG_PARAMETER,

	// Animation
	20021114, QDSCR_ANIMATION_FRAME,
	20021114, QDSCR_ANIMATION_INFO,			// 90
	20021114, QDSCR_ANIMATION_FILE,
	20021114, QDSCR_ANIMATION,

	// AnimationSet
	20021114, QDSCR_ANIMATION_SET,
	20030716, QDSCR_ANIMATION_SET_START_ANGLE,
	20060414, QDSCR_ANIMATION_SET_TURN,		// 95

	// Coords Animation
	20021114, QDSCR_COORDS_ANIMATION_POINT,
	20021114, QDSCR_COORDS_ANIMATION,
	20021114, QDSCR_ANIMATION_PHASE,

	// Game Object
	20021114, QDSCR_PARALLAX_OFFSET,

	// Static Object
	20021114, QDSCR_STATIC_OBJECT,			// 100

	// Animated Object
	20021114, QDSCR_ANIMATED_OBJECT,
	20021114, QDSCR_OBJECT_STATE,
	20021114, QDSCR_OBJECT_MOVEMENT_STATES,
	20021114, QDSCR_OBJECT_MOVEMENT_STATE,

	20021114, QDSCR_OBJECT_DIRECTION,			// 105
	20021114, QDSCR_OBJECT_DEFAULT_POS,
	20021114, QDSCR_OBJECT_DEFAULT_DIRECTION,

	20021114, QDSCR_OBJECT_STATE_STATIC,
	20021114, QDSCR_OBJECT_STATE_WALK,
	20021114, QDSCR_OBJECT_STATE_MASK,		// 110
	20021114, QDSCR_OBJECT_STATE_MASK_POS,
	20021114, QDSCR_OBJECT_STATE_MASK_SIZE,
	20021114, QDSCR_OBJECT_STATE_MASK_DATA,
	20021114, QDSCR_OBJECT_STATE_MASK_PARENT,
	20021114, QDSCR_OBJECT_STATE_MASK_CONTOUR,	// 115
	20021114, QDSCR_OBJECT_STATE_CENTER_OFFSET,
	20021114, QDSCR_OBJECT_STATE_DELAY,
	20030723, QDSCR_OBJECT_STATE_SOUND_DELAY,
	20040123, QDSCR_OBJECT_STATE_TEXT_DELAY,
	20030131, QDSCR_OBJECT_STATE_CURSOR_ID,		// 120
	20030716, QDSCR_OBJECT_STATE_ACCELERATION,
	20031206, QDSCR_OBJECT_STATE_RND_MOVE,

	20040519, QDSCR_OBJECT_STATE_WALK_SOUND_FREQUENCY,

	20030416, QDSCR_STATE_CENTER_OFFSETS,
	20030416, QDSCR_STATE_STATIC_CENTER_OFFSETS,	// 125
	20060623, QDSCR_STATE_START_CENTER_OFFSETS,
	20060623, QDSCR_STATE_STOP_CENTER_OFFSETS,

	// State conditions
	20021114, QDSCR_CONDITION,
	20021114, QDSCR_CONDITION_DATA_INT,
	20021114, QDSCR_CONDITION_DATA_FLOAT,			// 130
	20021114, QDSCR_CONDITION_DATA_STRING,
	20030203, QDSCR_CONDITION_INVERSE,
	20021114, QDSCR_CONDITIONS_MODE,

	20030219, QDSCR_CONDITION_GROUP,
	20030226, QDSCR_CONDITION_OBJECT,				// 135

	// Counter
	20030530, QDSCR_COUNTER,
	20030530, QDSCR_COUNTER_ELEMENT,
	20030530, QDSCR_COUNTER_INC_VALUE,
	20030530, QDSCR_COUNTER_LIMIT,
	20060420, QDSCR_COUNTER_DIGITS,				// 140
	20060423, QDSCR_COUNTER_NAME,

	// Object Scale
	20021114, QDSCR_OBJ_SCALE_INFO,

	// Mouse object
	20021114, QDSCR_MOUSE_OBJECT,
	20030131, QDSCR_MOUSE_DEFAULT_CURSORS,

	// Moving Object
	20021114, QDSCR_MOVEMENT_ANIMATION_NUMBER,	// 145
	20021114, QDSCR_MOVEMENT_ANIMATIONS,
	20021114, QDSCR_STATIC_ANIMATIONS,
	20021114, QDSCR_MOVING_OBJECT,
	20030704, QDSCR_COLLISION_RADIUS,
	20030704, QDSCR_COLLISION_DELAY,				// 150
	20030704, QDSCR_COLLISION_PATH,
	20040406, QDSCR_FOLLOW_MIN_RADIUS,
	20040406, QDSCR_FOLLOW_MAX_RADIUS,
	20040428, QDSCR_ATTACH_SHIFT,
	20030704, QDSCR_PERSONAGE_CONTROL,			// 155
	20030716, QDSCR_PERSONAGE_MOVEMENT_TYPE,
	20060129, QDSCR_ROTATION_ANGLE_PER_QUANT,

	// Inventory
	20021114, QDSCR_INVENTORY_CELL_TYPE,
	20021114, QDSCR_INVENTORY_CELL_SET,
	20021114, QDSCR_INVENTORY_CELL_SET_SIZE,		// 160
	20060129, QDSCR_INVENTORY_CELL_SET_ADDITIONAL_CELLS,
	20021114, QDSCR_INVENTORY_CELL_SET_POS,
	20021114, QDSCR_INVENTORY,

	// Camera & grid
	20021114, QDSCR_CAMERA,
	20021114, QDSCR_CAMERA_GRID_SIZE,				// 165
	20021114, QDSCR_CAMERA_GRID_HEIGHT,
	20021114, QDSCR_CAMERA_GRID_ATTRIBUTES,
	20021114, QDSCR_CAMERA_GRID_CENTER,
	20021114, QDSCR_CAMERA_CELL_SIZE,
	20021114, QDSCR_CAMERA_FOCUS,					// 170
	20021114, QDSCR_CAMERA_ANGLES,
	20021114, QDSCR_CAMERA_SCREEN_SIZE,
	20021114, QDSCR_CAMERA_SCREEN_OFFSET,
	20021114, QDSCR_CAMERA_SCREEN_CENTER,
	20030125, QDSCR_CAMERA_MODE,					// 175
	20030125, QDSCR_CAMERA_SCROLLING_SPEED,
	20030125, QDSCR_CAMERA_SCROLLING_DIST,
	20030530, QDSCR_CAMERA_SMOOTH_SWITCH,
	20040406, QDSCR_CAMERA_SCALE_POW,
	20040406, QDSCR_CAMERA_SCALE_Z_OFFSET,		// 180

	// Game end
	20030508, QDSCR_GAME_END,
	20030508, QDSCR_GAME_END_SCREEN,

	// Interface
	20030109, QDSCR_INTERFACE,

	20040506, QDSCR_INTERFACE_SCENE_REDRAW,

	20030110, QDSCR_INTERFACE_MAIN_MENU,			// 185
	20030317, QDSCR_INTERFACE_INGAME_SCREEN0,
	20030317, QDSCR_INTERFACE_INGAME_SCREEN1,
	20070329, QDSCR_INTERFACE_HIGHSCORE_NAME_SCREEN,
	20060129, QDSCR_INTERFACE_SAVE_PROMPT_SCREEN,
	20070329, QDSCR_INTERFACE_SAVE_NAME_SCREEN,	// 190

	20060129, QDSCR_INTERFACE_NEED_SAVE_SCREENSHOT,
	20070329, QDSCR_INTERFACE_NEED_SHOW_SAVE_NAME,
	20060129, QDSCR_INTERFACE_NEED_SHOW_SAVE_TIME,
	20060129, QDSCR_INTERFACE_SAVE_FONT_TYPE,
	20060129, QDSCR_INTERFACE_SAVE_FONT_COLOR,	// 195
	20070315, QDSCR_INTERFACE_SAVE_IS_AUTOSAVE,

	20030109, QDSCR_INTERFACE_SCREEN,
	20030609, QDSCR_INTERFACE_SCREEN_HIDE_TIME,
	20030609, QDSCR_INTERFACE_SCREEN_HIDE_OFFSET,
	20030109, QDSCR_INTERFACE_ELEMENT,			// 200

	20021128, QDSCR_INTERFACE_ELEMENT_STATE,
	20030121, QDSCR_INTERFACE_ELEMENT_STATE_MODE,
	20030109, QDSCR_INTERFACE_ELEMENT_POS,

	20030414, QDSCR_INTERFACE_OPTION_ID,

	20030121, QDSCR_INTERFACE_EVENT,				// 205
	20030121, QDSCR_INTERFACE_EVENT_DATA,
	20030121, QDSCR_INTERFACE_EVENT_BEFORE_ANIMATION,
	20061128, QDSCR_INTERFACE_EVENT_ACTIVATION_TYPE,

	20021128, QDSCR_INTERFACE_ACTION,
	20021128, QDSCR_INTERFACE_ACTION_DATA,		// 210

	20030116, QDSCR_INTERFACE_ANIMATION_FLAGS,
	20030203, QDSCR_INTERFACE_SLIDER_RECTANGLE,
	20030203, QDSCR_INTERFACE_SLIDER_ORIENTATION,
	20070115, QDSCR_INTERFACE_BACKGROUND_OFFSET,

	20030226, QDSCR_INTERFACE_THUMBNAIL_SIZE,		// 215
	20060129, QDSCR_INTERFACE_TEXT_SHIFT,

	20040123, QDSCR_TEXT_WINDOW_BORDER_CORNER,
	20040123, QDSCR_TEXT_WINDOW_BORDER_LINE_H,
	20040123, QDSCR_TEXT_WINDOW_BORDER_LINE_V,
	20040123, QDSCR_TEXT_WINDOW_BORDER_BACK,		// 220
	20040123, QDSCR_TEXT_WINDOW_SLIDER,
	20070321, QDSCR_TEXT_WINDOW_TYPE,
	20070329, QDSCR_TEXT_WINDOW_MAX_STRING_LENGTH,
	20070404, QDSCR_TEXT_WINDOW_INFO_TYPE,
	20070404, QDSCR_TEXT_WINDOW_PLAYER_ID,		// 225

	// Scene
	20021114, QDSCR_SCENE,
	20030613, QDSCR_SCENE_SAVE_SLOT,
	20030226, QDSCR_STARTUP_SCENE,
	20060630, QDSCR_SCENE_RESTART_MINIGAME,

	// Location
	20021114, QDSCR_LOCATION,						// 230

	20030919, QDSCR_GAME_TITLE,
	20031014, QDSCR_TEXT_DB,
	20031014, QDSCR_CD_KEY,

	// For qdScreenTextFormat
	20040308, QDSCR_SCREEN_TEXT_FORMAT,
	20040308, QDSCR_FONT_TYPE,					// 235

	20040517, QDSCR_DEFAULT_FONT,
	20060129, QDSCR_GLOBAL_DEPEND,
	20060129, QDSCR_INVERSE_DIRECTION,
	20060129, QDSCR_ALPHA,
	20060129, QDSCR_ENABLE_BACKGROUND,			// 240
	20060129, QDSCR_FADE_TIME,

	20070404, QDSCR_HALL_OF_FAME_SIZE,

	20060626, QDSCR_RESOURCE_COMPRESSION,

	20060715, QDSCR_SCREEN_TRANSFORM,

	20021114, QDSCR_MAX_KEYWORD_ID				// 245
};

xml::parser &qdscr_XML_Parser();
const char *qdscr_XML_string(const char *p);

} // namespace QDEngine

#endif // QDENGINE_CORE_PARSER_QDSCR_PARSER_H
