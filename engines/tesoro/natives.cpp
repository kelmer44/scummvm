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


#include "tesoro/tesoro.h"
#include "tesoro/objects.h"

namespace Tesoro {

// The index of some scenes in the game's scene table.
static const int kCubierta = 3, kCamarote = 4, kAlmacen = 5, kCocina = 6, kPlaya = 7;

void TesoroEngine::startTransition(const Common::SharedPtr<Scene> &scene, int32 x, int32 y, uint steps, uint delay, uint pause) {
	_transition = Transition();
	_transition.active = true;
	_transition.start = _gameTime;
	_transition.steps = steps;
	_transition.delay = delay;
	_transition.pause = pause;
	_transition.scene = scene;
	_transition.x = x;
	_transition.y = y;
	_transition.inputWas = _inputEnabled;
	// The last frame is what fades.
	Graphics::Surface &surface = *_screen->surfacePtr();
	_transition.frame.resize(640 * 480);
	for (int row = 0; row < 480; ++row)
		memcpy(&_transition.frame[row * 640], surface.getBasePtr(0, row), 640 * 4);
	_pointerText.clear();
	setInputEnabled(false);
}

void TesoroEngine::updateTransition() {
	if (!_transition.active)
		return;
	if (int32(_gameTime - _transition.start) < int32(_transition.steps * _transition.delay + _transition.pause))
		return;
	Transition done = _transition;
	_transition = Transition();
	placeJim(done.scene, done.x, done.y);
	if (done.direction)
		_jim->look(ScriptOp::Direction(done.direction), _gameTime);
	if (done.jimShown >= 0)
		_jim->show(done.jimShown != 0);
	if (done.setBusy >= 0)
		_jimState.busy = done.setBusy;
	if (done.setPhase >= 0)
		_jimState.phase = done.setPhase;
	if (done.script) {
		done.script->setScene(*done.scene);
		if (done.rebind)
			done.script->rebindParticipants();
	}
	setInputEnabled(done.inputWas);
	if (done.exitDoor)
		done.exitDoor->arrivedThrough(*this);
}

void TesoroEngine::drawTransition() {
	// Each step darkens the frame a little more (the colour of each pixel times (256 - step * k) / 256, in RGB565), the
	// last one is black, and the pause that follows stays black.
	const int32 elapsed = int32(_gameTime - _transition.start);
	const uint step = 256 / _transition.steps;
	uint level = elapsed < 0 ? 1 : MIN<uint>(_transition.steps, 1 + uint(elapsed) / _transition.delay);
	const int scale = MAX<int>(0, 256 - int(level * step));
	Graphics::Surface &surface = *_screen->surfacePtr();
	for (int row = 0; row < 480; ++row) {
		uint32 *target = (uint32 *)surface.getBasePtr(0, row);
		const uint32 *source = &_transition.frame[row * 640];
		for (int x = 0; x < 640; ++x) {
			const uint32 pixel = source[x];
			const uint r = (pixel >> 19) & 31, g = (pixel >> 10) & 63, b = (pixel >> 3) & 31;
			target[x] = rgb565ToArgb(uint16(((r * scale >> 8) << 11) | ((g * scale >> 8) << 5) | (b * scale >> 8)));
		}
	}
}

bool TesoroEngine::runNative(uint scriptType, uint ordinal, Script &script) {
	// A script moved to another scene by native code looks its participants up there.
	auto moveNow = [&](int level, int32 x, int32 y, bool jimShown) {
		Scene *scene = levelScene(level);
		script.setScene(*scene);
		placeJim(sharedScene(*scene), x, y);
		_jim->show(jimShown);
		script.rebindParticipants();
	};
	// The same after a fade, when the script is let go again.
	auto moveAfterFade = [&](int level, int32 x, int32 y, uint pause) {
		startTransition(sharedScene(*levelScene(level)), x, y, 4, 100, pause);
		_transition.script = &script;
	};
	switch (scriptType) {
	case 2: // the captain's cabin, on the way to the mutiny
		switch (ordinal) {
		case 0: case 1: case 2: case 3: case 4: case 5: {
			// Jim leaves through the cabin's door.
			SceneObject *door = levelScene(kCamarote)->find("PUERTA1");
			if (!door || door->kind() != 6)
				error("Tesoro: The cabin has no door.");
			static_cast<DoorObject *>(door)->useFromScript(*this);
			return true;
		}
		case 6:
			if (SceneObject *captain = levelScene(kCamarote)->find("CAPITAN"))
				captain->setDepth(10);
			return true;
		case 7: {
			// The mutiny begins: the storeroom's second door is closed, the sky of the deck moves, the kitchen's people
			// leave, and Jim is on the deck.
			if (SceneObject *door = levelScene(kAlmacen)->find("PUERTA2"))
				door->setActive(false);
			if (SceneObject *sky = levelScene(kCubierta)->find("CIELO"))
				sky->play(1, _gameTime, 0, -1, true);
			Scene *kitchen = levelScene(kCocina);
			const char *gone[] = {"DICK", "LOROPOCHO", "CUBO"};
			for (uint i = 0; i < ARRAYSIZE(gone); ++i)
				if (SceneObject *object = kitchen->find(gone[i]))
					object->setActive(false);
			moveAfterFade(kCubierta, 0x1b3, 0x2d, 0);
			_transition.jimShown = 0;
			_transition.rebind = true;
			return true;
		}
		case 8:
			_jim->show(true);
			return true;
		case 9:
			moveAfterFade(kCamarote, -0xfa, 0x82, 0);
			_transition.setBusy = 1;
			_transition.setPhase = 2;
			return true;
		}
		break;
	case 3: // the embarkation
		switch (ordinal) {
		case 0:
			return true;
		case 1:
			moveNow(kCubierta, 0x1e, 0x7d, false);
			return true;
		case 2:
			_jim->placeInScene(*_currentScene, 0x1b3, 0x2d, _gameTime);
			_jim->show(true);
			return true;
		}
		break;
	case 4: // Jim goes to the storeroom and the cabin during the mutiny
		switch (ordinal) {
		case 0:
			moveAfterFade(kAlmacen, 0x87, 0x87, 0);
			_transition.setBusy = 1;
			return true;
		case 1:
		case 2:
			moveAfterFade(kCamarote, -0xfa, 0x82, 0);
			_transition.setBusy = 1;
			return true;
		}
		break;
	case 5: // arrival on the island
		if (ordinal == 0) {
			Scene *beach = levelScene(kPlaya);
			script.setScene(*beach);
			placeJim(sharedScene(*beach), beach->entry().x, beach->entry().y);
			_jim->show(true);
			_jimState.busy = 0;
		}
		return true;
	}
	return false;
}

} // namespace Tesoro
