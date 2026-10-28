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
#include "tesoro/overlap.h"
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


// Whoever waits for a walk to end: a script or an object that Jim was sent to.
class ActionWaiter {
public:
	virtual ~ActionWaiter() {}
	virtual void actionDone() = 0;
};

struct TextEntry;
class Scene;

// What the game keeps in Jim between scenes: a flag that says the story is in a scripted phase (doors then
// behave differently), the number of the phase, and two more flags.
struct JimState {
	uint8 busy = 0;      // 0x110
	uint8 phase = 0;     // 0x111
	uint8 flag112 = 0;
	uint8 flag113 = 0;
};

// What the pointer's reactions need from the rest of the game.
class PointerHost {
public:
	virtual ~PointerHost() {}
	virtual void setPointerText(const Common::String &text) = 0;
	// The text of the dialogue tables with this name, if there is one.
	virtual const TextEntry *findText(const Common::String &name) const = 0;
	// Sends Jim to a point; the waiter is told when he gets there.
	virtual void walkJim(int32 x, int32 y, ActionWaiter *waiter) = 0;
	// Turns Jim to a direction (1 up, 2 down, 3 left, 4 right).
	virtual void turnJim(int direction) = 0;
	// Jim says a line. The pointer is switched off until he has finished.
	virtual void jimSays(const TextEntry &text) = 0;
	// The game clock, and the verb the pointer shows (the frame of its cursor).
	virtual uint32 now() const = 0;
	virtual void setVerb(int verb) = 0;
	// The scene at an index of the game's scene table (loaded when it is asked for), and the
	// interface scene that holds the inventory.
	virtual Scene *levelScene(int level) = 0;
	virtual Scene *interfaceScene() = 0;
	// Starts the label of a script block with a block of variables (null for none) in a scene.
	virtual void startObjectScript(const char *block, const char *label, uint8 *variables, Scene &scene) = 0;
	// Moves Jim to a point of a scene, facing a direction (1 up, 2 down, 3 left, 4 right). With `fade`
	// the screen fades to black first and Jim arrives 300 ms later; the door he comes out of is told.
	virtual void transferJim(Scene &scene, int32 x, int32 y, int direction, bool fade = false, SceneObject *exitDoor = nullptr) = 0;
	virtual JimState &jimState() = 0;
	// A random number below n.
	virtual uint randomBelow(uint n) = 0;
	// The index of the scene that Jim is in.
	virtual int jimLevel() const = 0;
	// The inventory item in Jim's hand (null for none). Holding an item shows it as the pointer
	// (its cursor frame); letting go (null) puts it back in the inventory and the pointer to the arrow.
	virtual SceneObject *heldItem() const = 0;
	virtual void holdItem(SceneObject *item, int cursorFrame) = 0;
	// The inventory page replaces the screen under the verb bar; this opens or closes it.
	virtual bool inventoryOpen() const = 0;
	virtual void closeInventory() = 0;
};

/**
 * Everything in a room derives from SceneObject, which holds one AnimationPlayer per animation bank and handles talking.
 */
class SceneObject {
public:
	// kCollidable objects are tested against each other when one of them moves; kSelectable ones
	// are the objects that the pointer can pick.
	enum Flags { kActive = 2, kShown = 4, kCollidable = 8, kHasAnimation = 16, kGrouped = 0x100, kSelectable = 0x400 };
	virtual ~SceneObject() {}

	virtual void setActive(bool active);
	void show(bool shown);
	bool play(uint bank, uint32 now, uint16 first = 0, int last = -1, bool loop = false, bool cacheDuration = false);

	// Advances the selected bank and refreshes the hit bounds.
	virtual void prepareFrame(uint32 now, Common::RandomSource &random);
	// Fills in the sprite this object submits for the frame; false when it draws nothing.
	virtual bool sprite(const SceneView &view, SceneSprite &sprite) const;

	// While an object talks, the first frame of its talking bank is its body, drawn just behind
	// the current frame (the mouth). This is the body's sprite; false when there is none.
	virtual bool backSprite(const SceneView &view, SceneSprite &sprite) const;

	const AnimationState &animationState() const { assert(!_players.empty()); return _players[_currentBank]->state(); }
	uint32 flags() const { return _flags; }
	uint32 kind() const { return _kind; }
	uint64 nameId() const { return _nameId; }
	uint16 depth() const { return _depth; }
	void setDepth(uint16 depth) { _depth = depth; }
	int32 x() const { return _x; }
	int32 y() const { return _y; }

	// The object's group divides the camera offset by this value, which gives the parallax effect.
	int32 parallax() const { return _parallax; }
	void setParallax(int32 parallax) { _parallax = parallax; }
	const SceneBounds &hitBounds() const { return _hitBounds; }
	// For objects that only the scene's own logic moves (the interface).
	void setPosition(int32 x, int32 y) { _x = x; _y = y; }

	// What an object needs to speak: the color and width of its text, and the talking animations (banks)
	struct TalkConfig {
		uint16 color = 0;
		uint16 width = 0;
		uint16 narrationColor = 0;
		Common::Array<uint> banks;
	};

	bool canTalk() const { return _talkable; }
	uint16 speechColor() const { return _talk.color; }
	uint16 speechWidth() const { return _talk.width; }
	uint16 narrationColor() const { return _talk.narrationColor; }

	// Starts talking: stores the animation it had, shows its talking bank, and moves
	// its mouth if mouthMoves. It goes on showing the closed mouth until restoreAnimation.
	void beginSpeech(uint32 now, bool mouthMoves, int option = 0);
	void endSpeech() { _speaking = false; }

	// What the object does once what it was saying has ended (or was skipped).
	virtual void speechEnded() {}
	bool isSpeaking() const { return _speaking; }

	// Puts back the animation that the object was playing before talking
	void restoreAnimation(uint32 now);
	bool hasSavedAnimation() const { return _animationSaved; }

	// Where the text of the object is centered relative to, stored in outX and outY.
	virtual void speechAnchor(const SceneView &view, int32 &outX, int32 &outY) const;

	// Turns the object to face a direction (1 up, 2 down, 3 left, 4 right).
	virtual void turn(int direction, uint32 now);

	// When a collidable object moves, the scene tests it against the collidable objects of its
	// group. If this object is "in front" of the other one the pair is tested with this one as
	// the first, else if the other is in front of this one with the other first. Whatever
	// the first object of a pair overlaps is told through onOverlap, nearest contact first.
	virtual bool inFront(const SceneObject &other) const { return false; }
	virtual void onOverlap(SceneObject &other, int32 overlap) {}

	// Works out what the overlap tests need for this frame: the hit box and outline of the
	// frame, and where the object was and is going to be.
	virtual void updateOverlap(uint32 now);
	const OverlapBody &overlapBody() const { return _overlap; }

	// Makes the position that the object was moved to the one it is drawn at.
	virtual void commitPosition() {}

	/**
	 * How the pointer reacts to the object once the probe overlaps it. Each returns 0 when the
	 * object did nothing about it, 1 when it took it (so that the click is not a click on the
	 * ground) and 3 when the frame's input is over. Only selectable objects are ever asked.
	 */
	virtual bool hitTest(PointerHost &host) { return true; }
	virtual int onHover(PointerHost &host) { return 0; }
	virtual void onUnhover(PointerHost &host) {}
	virtual int onLeftClick(PointerHost &host, int frame) { return 0; }
	virtual int onRightClick(PointerHost &host, int frame) { return onLeftClick(host, frame); }
	// Jim has come out of this object (a door) into its scene.
	virtual void arrivedThrough(PointerHost &host) {}
	const Common::String &name() const { return _name; }

	// The group of the scene that the object belongs to.
	uint group() const { return _group; }
	void setGroup(uint group) { _group = group; }
	// The scene that holds the object.
	Scene *scene() const { return _scene; }
	void setScene(Scene *scene) { _scene = scene; }
	// The block of variables that the object owns, which scripts started by it work on (null when it has none).
	uint8 *variables() { return _variables.empty() ? nullptr : &_variables[0]; }
	// The animation bank the object is showing.
	uint currentBank() const { return _currentBank; }
	bool isActive() const { return (_flags & kActive) != 0; }

	// A source of random numbers for the objects' mouths.

	// True while the selected bank is still playing an animation.
	bool isPlaying() const { return !_players.empty() && (_players[_currentBank]->state().flags & AnimationPlayer::kPlaying); }

protected:
	SceneObject(const SceneObjectRecord &record, const SceneGroup &group, const SceneData &scene);
	// For an object that does not come from a scene file.
	SceneObject(uint32 kind, uint64 nameId, const Common::Array<Common::SharedPtr<Animation>> &banks, int32 x, int32 y, uint16 depth);

	void addFlags(uint32 flags) { _flags |= flags; }
	void allocateVariables(uint size) { _variables.resize(size); }

	uint animationCount() const { return _banks.size(); }
	const Common::String &animationName(uint bank) const { return _banks[bank]->name; }
	const TalkConfig &talkConfig() const { return _talk; }
	void setTalkConfig(const TalkConfig &config) { _talk = config; _talkable = true; }

	// Selects the talking anim
	virtual void selectTalkBank(uint32 now);

	// Plays a bank without forgetting the animation that was saved (play does).
	void playBank(uint bank, uint32 now, uint16 first, int last, bool loop = false);

	// Whether the object shows itself talking on this frame.
	virtual bool showsTalk() const { return _animationSaved; }

	// Where the current frame is drawn, shared by every object that shows its animation.
	bool frameSprite(const SceneView &view, SceneSprite &sprite) const;

	const AnimationPlayer &selectedPlayer() const { assert(!_players.empty()); return *_players[_currentBank]; }
	AnimationPlayer &selectedPlayer() { assert(!_players.empty()); return *_players[_currentBank]; }
	uint selectedBank() const { return _currentBank; }
	void moveTo(int32 x, int32 y) { _x = x; _y = y; }

	// Where the object is going to be drawn once its position is committed.
	virtual void pendingPosition(int32 &x, int32 &y) const { x = _x; y = _y; }
	// Whether the animation's movement is dropped each time it advances, as the interactive objects
	// of the original do (0x407770). Jim and scenery keep it.
	virtual bool dropsMovement() const { return false; }
	// Lets an object change the hit box of its frame (Jim's follows his scale).
	virtual void adjustBox(int32 box[4]) const {}
	virtual bool isPlayer() const { return false; }

private:
	SceneObject(const SceneObject &) = delete;
	SceneObject &operator=(const SceneObject &) = delete;

	Common::Array<Common::SharedPtr<Animation>> _banks;
	Common::Array<Common::SharedPtr<AnimationPlayer>> _players;
	uint _currentBank = 0;
	uint32 _kind;
	uint64 _nameId;
	uint32 _flags = kActive | kHasAnimation;
	TalkConfig _talk;
	bool _talkable = false;
	bool _speaking = false, _mouthMoves = false;
	int _talkOption = 0;   // which of the talking banks the speech uses
	bool _animationSaved = false; // the object is showing a talking bank
	uint _savedBank = 0;
	uint32 _savedFlags = 0, _lastMouthChange = 0;
	int32 _x, _y;
	uint16 _depth;
	int32 _parallax = 300;
	SceneBounds _hitBounds;
	uint _group = 0;
	Scene *_scene = nullptr;
	Common::Array<uint8> _variables;
	Common::Array<int16> _outline; // the frame's floor points as x, y pairs
	Common::String _name;

protected:
	OverlapBody _overlap;
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
class InteractiveObject : public SceneObject, public ActionWaiter {
public:
	InteractiveObject(const SceneObjectRecord &record, const SceneGroup &group, const SceneData &scene, uint32 now, uint parameterOffset = 0, uint variableSize = 0);
	bool sprite(const SceneView &view, SceneSprite &sprite) const override;
	bool dropsMovement() const override { return true; }

	// Hovering shows the object's name; a click sends Jim to its approach point, and when he
	// gets there he turns to the object and the verb of the click runs.
	int onHover(PointerHost &host) override;
	void onUnhover(PointerHost &host) override;
	int onLeftClick(PointerHost &host, int frame) override;
	void actionDone() override;

protected:
	// What the object does for a verb (the pointer's frame when it was clicked). Returns the
	// same codes as onLeftClick: 3 when Jim started speaking, 0 when nothing happened.
	virtual int runVerb(int verb);
	// Jim has an inventory item (by its number) in his hand when he gets to the object: it goes
	// back to the inventory, the pointer to the arrow, and the object answers. By default Jim
	// refuses; characters and puzzle objects answer in their own way.
	virtual int itemUsed(int item);
	// Jim answers a verb with the line the object has for it, or with the game's generic line
	// when it has none (empty for the verbs that have no generic line).
	int answerVerb(int verb, const char *genericLine);
	// Where Jim stands to use the object and the way he faces there.
	int32 approachX() const { return _approachX; }
	int32 approachY() const { return _approachY; }
	int approachDirection() const { return _approachDirection; }
	// Jim arrives at the approach point: he turns to the object and the verb runs.
	void arrive();
	PointerHost *host() const { return _host; }
	void bindHost(PointerHost &host) { _host = &host; }
	void setTextName(const Common::String &name) { _textName = name; }
	void setVerbText(int verb, const Common::String &text) { _verbTexts[verb] = text; }
	void setClickedVerb(int verb) { _verb = verb; }
	int clickedVerb() const { return _verb; }

private:
	int32 _approachX = 0, _approachY = 0;
	int _approachDirection = 1;     // 1 up, 2 down, 3 left, 4 right
	Common::String _textName;       // its name in the dialogue tables, if it has one
	Common::String _verbTexts[7];   // the lines that its verbs answer with
	PointerHost *_host = nullptr;   // who sent Jim
	int _verb = 0;
};

// Kind 5: the characters of the game.
class CharacterObject : public InteractiveObject {
public:
	CharacterObject(const SceneObjectRecord &record, const SceneGroup &group, const SceneData &scene, uint32 now);

	// A character stands facing a direction in its idle bank, which has a frame for each.
	void turn(int direction, uint32 now) override;

protected:
	// Talking to the captain and to the recruiter starts their conversations; so does giving them an item.
	int runVerb(int verb) override;
	int itemUsed(int item) override;
	// The parrot's words end with Dick coming into view.
	void speechEnded() override;

private:
	uint _idleBank = 0;
	uint _class = 0;   // the kind of character, which decides its variables and what it does for a verb
};

// Kind 7: the walkable ground of a room. It is not drawn; its outline gives the scale of
// the characters that stand on it.
class FloorObject : public SceneObject {
public:
	FloorObject(const SceneObjectRecord &record, const SceneGroup &group,
	            const SceneData &scene, uint32 now);
	// The scale (256 is full size) of a character at a point of the room.
	int scaleAt(int32 x, int32 y) const;
	bool dropsMovement() const override { return true; }
};

/**
 * The pointer's probe, a point that follows the mouse. When it is moved the scene tests it against
 * the selectable objects of its group; the ones that it overlaps become the candidates that the
 * pointer reacts to (hover, click) on the following frame.
 */
class PointerProbe : public SceneObject {
public:
	PointerProbe();
	// Puts the probe on a point of the world.
	void placeAt(int32 x, int32 y) { moveTo(x, y); }
	void clearCandidates() { _candidates.clear(); }
	const Common::Array<SceneObject *> &candidates() const { return _candidates; }

	bool inFront(const SceneObject &other) const override { return (other.flags() & kSelectable) != 0; }
	void onOverlap(SceneObject &other, int32) override { _candidates.push_back(&other); }
	void updateOverlap(uint32 now) override;

private:
	Common::Array<SceneObject *> _candidates;
};

// What the hotspot debug layer draws for an object: its hit box and outline on the screen.
struct SceneDebugShape {
	const SceneObject *object = nullptr;
	Common::Rect box;
	Common::Array<Common::Point> outline;
};

// A loaded scene owns every object in its LVL. Kinds without a SceneObject subclass
// yet are built as inert placeholders (with a warning) so nothing is silently dropped.
class Scene {
public:
	Scene(const SceneData &scene, uint32 now);

	/**
	 * Each frame, Scene::appendSprites goes through every active, shown object.
	 * - It first lets the object advance its animation (prepareFrame).
	 * - Then asks the object for its sprite (sprite(), plus backSprite() for a talking body)
	 *   and collects them into an array.
	 * - Each object works out its own screen position using the view for its group (the camera and parallax).
	 * - SceneRenderer::draw then draws the array into the screen surface, using a depth buffer
	 *   so overlapping sprites end up in the right order.
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

	// Tells the scene that a collidable object moved this frame. It is tested against the
	// others by the next resolveOverlaps.
	void objectMoved(SceneObject *object);

	/**
	 * The end of the scene's frame. Every object that moved and the collidable objects of its group
	 * are prepared; each moved object is paired with the collidable, active objects of its group,
	 * the pairs that touch are queued by how far along their movement they touch, and each is
	 * delivered to the first object of the pair. Finally the moved objects commit their positions.
	 */
	void resolveOverlaps(uint32 now);

	// The hit box and outline of every collidable, active object as the last frame's overlap tests
	// saw them, on the screen.
	void debugShapes(Common::Array<SceneDebugShape> &shapes) const;

	// Every object the scene has, in file order.
	const Common::Array<Common::SharedPtr<SceneObject>> &objects() const { return _objects; }

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

	// The pair queue: the first object touched the second at `overlap` (in 65536ths).
	struct OverlapPair {
		SceneObject *first, *second;
		int32 overlap;
		uint16 priority;
	};
	Common::Array<SceneObject *> _moved;
	Common::Array<OverlapPair> _pairs;
	void queuePair(SceneObject &first, SceneObject &second);

	SceneObject *deliver(uint32 now, SceneObject *object, const char *name, const SceneMessage &message);
	SceneObject *createObject(const SceneObjectRecord &record, const SceneGroup &group,  const SceneData &scene, uint32 now);
};

} // namespace Tesoro

#endif
