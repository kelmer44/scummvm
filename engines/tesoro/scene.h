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

#ifndef TESORO_SCENE_H
#define TESORO_SCENE_H

#include "common/random.h"
#include "tesoro/animation.h"
#include "tesoro/resources.h"

namespace Tesoro {

// World coordinates have positive Y upwards.
struct SceneBounds {
	int32 left = 0, top = 0, right = 0, bottom = 0;
};

struct SceneView {
	int32 x = 0, y = 0;
};

/**
 * The camera is a 2D scrolling of the game window; scenes can be bigger than 640x480 pixels,
 * the camera decides what part is visible.
 * groupView groups objects by parallax.
 */
class SceneObject;

class SceneCamera {
public:
	void setBounds(const SceneBounds &bounds) { _bounds = bounds; }
	// With a duration the camera glides to the position over that many milliseconds from `now`.
	bool setPosition(int32 x, int32 y, uint32 now = 0, uint32 duration = 0);
	// The camera keeps this object in view horizontally. Pass nullptr to stop following.
	void setActor(const SceneObject *actor) { _actor = actor; }
	// Follows the actor and advances a glide; call it once per frame before reading the views.
	void update(uint32 now);
	SceneView groupView(int32 parallax) const;
	const SceneView &position() const { return _position; }

private:
	SceneBounds _bounds;
	SceneView _position;
	// Where the camera is going, where a glide started, and when.
	SceneView _target, _from;
	uint32 _glideStart = 0, _glideDuration = 0;
	const SceneObject *_actor = nullptr;
};

/**
 * Data about the room itself
 */
struct RoomEntry {
	uint floorGroup = 0, floorObject = 0; // which group and object is the floor located in
	int32 x = 0, y = 0; // Default entry point
	byte musicCue = 0;
	SceneBounds bounds; // Rectangle that can be covered by the camera
};

// Resolve the floor origin for default entry; scene transitions can supply another position.
bool findRoomEntry(const SceneData &scene, RoomEntry &entry);

// A drawing request for a SceneObject
struct SceneSprite {
	const Image *image = nullptr;
	int32 x = 0, y = 0;
	uint16 depth = 0;
	// 0 draws the image as it is; otherwise its scale in 256ths (256 is full size).
	int32 scale = 0;
};


/**
 * Everything in a room derives from SceneObject, which holds one AnimationPlayer per animation bank and handles talking.
 */
class SceneObject {
public:
	enum Flags { kActive = 2, kShown = 4, kHasAnimation = 16, kGrouped = 0x100 };
	virtual ~SceneObject() {}

	void setActive(bool active);
	void show(bool shown);
	bool play(uint bank, uint32 now, uint16 first = 0, int last = -1, bool loop = false, bool cacheDuration = false);

	// Advances the selected bank and refreshes the hit bounds.
	virtual void prepareFrame(uint32 now, Common::RandomSource &random);
	// Fills in the sprite this object submits for the frame; false when it draws nothing.
	virtual bool sprite(const SceneView &view, SceneSprite &sprite) const;
	// While an object talks, the first frame of its talking bank is its body, drawn just behind
	// the current frame (the mouth). This is the body's sprite; false when there is none.
	virtual bool backSprite(const SceneView &view, SceneSprite &sprite) const;

	const AnimationState &animationState() const { assert(!_players.empty()); return _players[_selected]->state(); }
	uint32 flags() const { return _flags; }
	uint32 kind() const { return _kind; }
	uint64 nameId() const { return _nameId; }
	uint16 depth() const { return _depth; }
	int32 x() const { return _x; }
	int32 y() const { return _y; }

	// The projection divisor of the group the object is in; it decides how its view moves with the camera.
	int32 parallax() const { return _parallax; }
	void setParallax(int32 parallax) { _parallax = parallax; }
	const SceneBounds &hitBounds() const { return _hitBounds; }

	// What an object needs to speak: the colour and width of its text, and the banks that
	// show it talking (the first frame of each is a closed mouth).
	struct TalkConfig {
		uint16 colour = 0;
		uint16 width = 0;
		uint16 narrationColour = 0;
		Common::Array<uint> banks;
	};

	bool canTalk() const { return _talkable; }
	uint16 speechColour() const { return _talk.colour; }
	uint16 speechWidth() const { return _talk.width; }
	uint16 narrationColour() const { return _talk.narrationColour; }

	// Starts talking: the object keeps the animation it had, shows its talking bank, and moves
	// its mouth if mouthMoves. It goes on showing the closed mouth until restoreAnimation.
	void beginSpeech(uint32 now, bool mouthMoves);
	void endSpeech() { _speaking = false; }
	bool isSpeaking() const { return _speaking; }

	// Puts back the animation that the object had before it talked.
	void restoreAnimation(uint32 now);
	bool hasSavedAnimation() const { return _animationSaved; }

	// Where the text of what the object says is centered on the screen, for a view of its group.
	virtual void speechAnchor(const SceneView &view, int32 &outX, int32 &outY) const;

	// Turns the object to face a direction (1 up, 2 down, 3 left, 4 right).
	virtual void turn(int direction, uint32 now);

	// A source of random numbers for the objects' mouths.

	// True while the selected bank is still playing an animation.
	bool isPlaying() const { return !_players.empty() && (_players[_selected]->state().flags & AnimationPlayer::kPlaying); }

protected:
	SceneObject(const SceneObjectRecord &record, const SceneGroup &group, const SceneData &scene);
	// For an object that does not come from a scene file.
	SceneObject(uint32 kind, uint64 nameId, const Common::Array<Common::SharedPtr<Animation>> &banks, int32 x, int32 y, uint16 depth);

	void addFlags(uint32 flags) { _flags |= flags; }
	uint animationCount() const { return _banks.size(); }
	const Common::String &animationName(uint bank) const { return _banks[bank]->name; }
	const TalkConfig &talkConfig() const { return _talk; }
	void setTalkConfig(const TalkConfig &config) { _talk = config; _talkable = true; }

	// Selects the bank that shows the object talking; the choice is up to the object.
	virtual void selectTalkBank(uint32 now);

	// Plays a bank without forgetting the animation that was saved (play does).
	void playBank(uint bank, uint32 now, uint16 first, int last, bool loop = false);

	// Whether the object shows itself talking on this frame.
	virtual bool showsTalk() const { return _animationSaved; }

	// Where the current frame is drawn, shared by every object that shows its animation.
	bool frameSprite(const SceneView &view, SceneSprite &sprite) const;

	const AnimationPlayer &selectedPlayer() const { assert(!_players.empty()); return *_players[_selected]; }
	AnimationPlayer &selectedPlayer() { assert(!_players.empty()); return *_players[_selected]; }
	uint selectedBank() const { return _selected; }
	void moveTo(int32 x, int32 y) { _x = x; _y = y; }

private:
	SceneObject(const SceneObject &) = delete;
	SceneObject &operator=(const SceneObject &) = delete;

	Common::Array<Common::SharedPtr<Animation>> _banks;
	Common::Array<Common::SharedPtr<AnimationPlayer>> _players;
	uint _selected = 0;
	uint32 _kind;
	uint64 _nameId;
	uint32 _flags = kActive | kHasAnimation;
	TalkConfig _talk;
	bool _talkable = false;
	bool _speaking = false, _mouthMoves = false;
	bool _animationSaved = false; // the object is showing a talking bank
	uint _savedBank = 0;
	uint32 _savedFlags = 0, _lastMouthChange = 0;
	int32 _x, _y;
	uint16 _depth;
	int32 _parallax = 300;
	SceneBounds _hitBounds;
};

// Kind 3: backgrounds and ambient animation.
class PropObject : public SceneObject {
public:
	PropObject(const SceneObjectRecord &record, const SceneGroup &group, const SceneData &scene, uint32 now);
	bool sprite(const SceneView &view, SceneSprite &sprite) const override;
};

// A message scripts can send to scene objects.
struct SceneMessage {
	enum Kind { kActivate, kShow, kPlay };
	// How a play message ends: stop on `first`, run to the end of the sequence, loop,
	// or run to the end while the sender waits for it.
	enum PlayMode { kStop, kPlayOnce, kLoop, kPlayAndWait };

	Kind kind = kActivate;
	bool on = false;             // kActivate, kShow
	uint bank = 0;               // kPlay
	uint16 first = 0;            // kPlay
	PlayMode mode = kPlayOnce;   // kPlay
};

// Kinds 6, 9, 12, 13, 17 and 20: objects the player can interact with.
class InteractiveObject : public SceneObject {
public:
	InteractiveObject(const SceneObjectRecord &record, const SceneGroup &group, const SceneData &scene, uint32 now, uint parameterOffset = 0);
	bool sprite(const SceneView &view, SceneSprite &sprite) const override;
};

// Kind 5: the characters of the game.
class CharacterObject : public InteractiveObject {
public:
	CharacterObject(const SceneObjectRecord &record, const SceneGroup &group, const SceneData &scene, uint32 now);

	// A character stands facing a direction in its idle bank, which has a frame for each.
	void turn(int direction, uint32 now) override;

private:
	uint _idleBank = 0;
};

// Kind 7: the walkable ground of a room. It is not drawn; its outline gives the scale of
// the characters that stand on it.
class FloorObject : public SceneObject {
public:
	FloorObject(const SceneObjectRecord &record, const SceneGroup &group,
	            const SceneData &scene, uint32 now);
	// The scale (256 is full size) of a character at a point of the room.
	int scaleAt(int32 x, int32 y) const;
};

// A loaded scene owns every object in its LVL. Kinds without a SceneObject subclass
// yet are built as inert placeholders (with a warning) so nothing is silently dropped.
class Scene {
public:
	Scene(const SceneData &scene, uint32 now);

	/**
	 * Each frame, Scene::appendSprites goes through every active, shown object.
	 * It first lets the object advance its animation (prepareFrame).
	 * It then asks the object for its sprite (sprite(), plus backSprite() for a talking body)
	 * and collects them into an array.
	 * Each object works out its own screen position using the view for its group (the camera and parallax).
	 * SceneRenderer::draw then draws the array into the screen surface, using a depth buffer
	 * so overlapping sprites end up in the right order.
	 */
	void appendSprites(uint32 now, Common::RandomSource &random, Common::Array<SceneSprite> &sprites);

	// Each scene has its own camera, which starts on the room's floor.
	SceneCamera &camera() { return _camera; }
	const SceneCamera &camera() const { return _camera; }

	// Where the room places a character by default, its camera bounds and its music cue.
	const RoomEntry &entry() const { return _entry; }

	// Finds an object by its LVL name (compared by the original's 64-bit encoding).
	// If several objects share a name, the first in LVL order is returned.
	SceneObject *find(const Common::String &name) const { return find(encodeName(name)); }
	SceneObject *find(uint64 id) const;

	// Delivers a message to the named object. Returns the object when the sender has to
	// wait for its animation to finish (kPlayAndWait), otherwise nullptr.
	SceneObject *send(uint32 now, const Common::String &target, const SceneMessage &message);
	SceneObject *send(uint32 now, uint64 target, const SceneMessage &message);

	// Adds an object that does not come from the scene file, such as Jim, to a group.
	void addObject(const Common::SharedPtr<SceneObject> &object, uint group);
	void removeObject(SceneObject *object);

	// The floor that the room names, if it has one.
	const FloorObject *floor() const { return _floor; }

private:
	struct Group {
		int32 parallax;
		Common::Array<Common::SharedPtr<SceneObject>> objects;
	};
	Common::Array<Group> _groups;
	Common::Array<Common::SharedPtr<SceneObject>> _objects;
	uint32 _warnedKinds = 0;
	const FloorObject *_floor = nullptr;
	SceneCamera _camera;
	RoomEntry _entry;

	SceneObject *deliver(uint32 now, SceneObject *object, const char *name, const SceneMessage &message);
	SceneObject *createObject(const SceneObjectRecord &record, const SceneGroup &group,  const SceneData &scene, uint32 now);
};

} // namespace Tesoro

#endif
