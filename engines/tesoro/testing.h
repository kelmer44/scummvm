
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

#ifndef TESORO_TESTING_H
#define TESORO_TESTING_H

namespace Tesoro {

/**
 * Temporary shortcuts that speed up testing. They skip parts of the game that the original gets
 * from its scripts and movies, so every one of them differs from the original; switch them all off
 * (and delete this header) when they are not needed. Each one is independent of the others.
 */
namespace TestingShortcuts {

// Skips the splash movie at launch, the movie of the new game and the censorship screen, going from
// the main menu straight to the difficulty menu. Off: the startup plays as the original does.
const bool kSkipStartupScreens = true;

// Starts the adventure in the office with the state that the introduction leaves (the captain gone,
// Jim at the door, the harbour's door active but hidden), without playing the introduction. Off: the
// adventure starts in the harbour with the introduction.
const bool kStartInOffice = true;

} // End of namespace TestingShortcuts

} // End of namespace Tesoro

#endif
