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

#include "math/matrix3.h"
#include "math/vector3d.h"
#include "common/algorithm.h"
#include "twp/scenegraph.h"
#include "twp/twp.h"
#include "twp/gfx.h"
#include "twp/object.h"

namespace Twp {

#define DEFAULT_FPS 10.f

static float _getFps(float fps, float animFps) {
	if (fps != 0.f)
		return fps;
	return (animFps < 1e-3) ? DEFAULT_FPS : animFps;
}

Node::Node(const Common::String &name, bool visible, Math::Vector2d scale, Color color)
	: _name(name),
	  _parent(NULL),
	  _color(color),
	  _computedColor(color),
	  _visible(visible),
	  _rotation(0.f),
	  _rotationOffset(0.f),
	  _scale(scale) {
}

Node::~Node() {}

void Node::addChild(Node *child) {
	if (child->_parent) {
		child->_pos -= getAbsPos();
		for (int i = 0; i < _children.size(); i++) {
			if (_children[i] == child) {
				child->_parent->_children.erase(child->_parent->_children.begin() + i);
				break;
			}
		}
	}
	_children.push_back(child);
	child->_parent = this;
	child->updateColor();
	child->updateAlpha();
}

const Node *Node::getRoot() const {
	const Node *result = this;
	while (result->_parent != NULL) {
		result = result->_parent;
	}
	return result;
}

int Node::find(Node *other) {
	for (int i = 0; i < _children.size(); i++) {
		if (_children[i] == other) {
			return i;
		}
	}
	return -1;
}

void Node::removeChild(Node *node) {
	int i = find(node);
	if (i != -1) {
		_children.remove_at(i);
	}
}

void Node::clear() {
	_children.clear();
}

void Node::remove() {
	if (_parent != NULL)
		_parent->removeChild(this);
}

void Node::setColor(Color c) {
	_color.rgba.r = c.rgba.r;
	_color.rgba.g = c.rgba.g;
	_color.rgba.b = c.rgba.b;
	_computedColor.rgba.r = c.rgba.r;
	_computedColor.rgba.g = c.rgba.g;
	_computedColor.rgba.b = c.rgba.b;
	updateColor();
}

void Node::setAlpha(float alpha) {
	_color.rgba.a = alpha;
	_computedColor.rgba.a = alpha;
	updateAlpha();
}

void Node::updateColor() {
	Color parentColor = !_parent ? Color(1.f, 1.f, 1.f) : _parent->_computedColor;
	updateColor(parentColor);
}

void Node::updateAlpha() {
	float parentOpacity = !_parent ? 1.f : _parent->_computedColor.rgba.a;
	updateAlpha(parentOpacity);
}

void Node::updateColor(Color parentColor) {
	_computedColor.rgba.r = _color.rgba.r * parentColor.rgba.r;
	_computedColor.rgba.g = _color.rgba.g * parentColor.rgba.g;
	_computedColor.rgba.b = _color.rgba.b * parentColor.rgba.b;
	onColorUpdated(_computedColor);
	for (int i = 0; i < _children.size(); i++) {
		Node *child = _children[i];
		child->updateColor(_computedColor);
	}
}

void Node::updateAlpha(float parentAlpha) {
	_computedColor.rgba.a = _color.rgba.a * parentAlpha;
	onColorUpdated(_computedColor);
	for (int i = 0; i < _children.size(); i++) {
		Node *child = _children[i];
		child->updateAlpha(_computedColor.rgba.a);
	}
}

void Node::setAnchor(Math::Vector2d anchor) {
	if (_anchor != anchor) {
		_anchorNorm = anchor / _size;
		_anchor = anchor;
	}
}

void Node::setAnchorNorm(Math::Vector2d anchorNorm) {
	if (_anchorNorm != anchorNorm) {
		_anchorNorm = anchorNorm;
		_anchor = _size * _anchorNorm;
	}
}

void Node::setSize(Math::Vector2d size) {
	if (_size != size) {
		_size = size;
		_anchor = size * _anchorNorm;
	}
}

static int cmpNodes(const Node *x, const Node *y) {
	return y->getZSort() < x->getZSort();
}

void Node::draw(Math::Matrix4 parent) {
	if (_visible) {
		Math::Matrix4 trsf = getTrsf(parent);
		Math::Matrix4 myTrsf(trsf);
		myTrsf.translate(Math::Vector3d(-_anchor.getX(), _anchor.getY(), 0.f));
		Common::sort(_children.begin(), _children.end(), cmpNodes);
		drawCore(myTrsf);
		for (int i = 0; i < _children.size(); i++) {
			Node *child = _children[i];
			child->draw(trsf);
		}
	}
}

Math::Vector2d Node::getAbsPos() const {
	return !_parent ? _pos : _parent->getAbsPos() + _pos;
}

Math::Matrix4 Node::getTrsf(Math::Matrix4 parentTrsf) {
	return parentTrsf * getLocalTrsf();
}

static void scale(Math::Matrix4& m, const Math::Vector2d &v) {
	m(0, 0) *= v.getX();
	m(1, 1) *= v.getY();
}

Math::Matrix4 Node::getLocalTrsf() {
	Math::Vector2d p = _pos + _offset;
	Math::Matrix4 m1;
	m1.translate(Math::Vector3d(p.getX(), p.getY(), 0.f));
	Math::Matrix3 mRot;
	mRot.buildAroundZ(Math::Angle(-_rotation + _rotationOffset));
	Math::Matrix4 m2;
	m2.setRotation(mRot);
	scale(m2, _scale);
	Math::Matrix4 m3;
	m3.translate(Math::Vector3d(_renderOffset.getX(), _renderOffset.getY(), 0.f));
	return m1 * m2 * m3;
}

Rectf Node::getRect() const {
	Math::Vector2d size = _size * _scale;
	return Rectf::fromPosAndSize(getAbsPos(), Math::Vector2d(-size.getX(), size.getY()) * _anchorNorm * _size);
}

OverlayNode::OverlayNode()
	: Node("overlay"),
	  _ovlColor(0, 0, 0, 0) {
}

OverlayNode::~OverlayNode() {}

void OverlayNode::drawCore(Math::Matrix4 trsf) {
	Gfx &gfx = g_engine->getGfx();
	gfx.drawQuad(gfx.camera(), _ovlColor);
}

ParallaxNode::ParallaxNode(const Math::Vector2d &parallax, const Common::String &sheet, const Common::StringArray &frames)
	: Node("parallax"),
	  _parallax(parallax),
	  _sheet(sheet),
	  _frames(frames) {
}

ParallaxNode::~ParallaxNode() {}

Math::Matrix4 ParallaxNode::getTrsf(Math::Matrix4 parentTrsf) {
	Gfx &gfx = g_engine->getGfx();
	Math::Matrix4 trsf = Node::getTrsf(parentTrsf);
	Math::Vector2d camPos = gfx.cameraPos();
	Math::Vector2d p = Math::Vector2d(-camPos.getX() * _parallax.getX(), -camPos.getY() * _parallax.getY());
	trsf.translate(Math::Vector3d(p.getX(), p.getY(), 0.0f));
	return trsf;
}

void ParallaxNode::drawCore(Math::Matrix4 trsf) {
	Gfx &gfx = g_engine->getGfx();
	SpriteSheet *sheet = g_engine->_resManager.spriteSheet(_sheet);
	Texture *texture = g_engine->_resManager.texture(sheet->meta.image);
	Math::Matrix4 t = trsf;
	float x = 0.f;
	for (int i = 0; i < _frames.size(); i++) {
		const SpriteSheetFrame &frame = sheet->frameTable[_frames[i]];
		Math::Matrix4 myTrsf = t;
		myTrsf.translate(Math::Vector3d(x + frame.spriteSourceSize.left, frame.sourceSize.getY() - frame.spriteSourceSize.height() - frame.spriteSourceSize.top, 0.0f));
		gfx.drawSprite(frame.frame, *texture, getColor(), myTrsf);
		t = trsf;
		x += frame.frame.width();
	}
}

Anim::Anim(Object *obj)
	: Node("anim") {
	_obj = obj;
	_zOrder = 1000;
}

void Anim::clearFrames() {
	_frames.clear();
}

void Anim::setAnim(const ObjectAnimation *anim, float fps, bool loop, bool instant) {
	_anim = anim;
	_disabled = false;
	setName(anim->name);
	_sheet = anim->sheet;
	_frames = anim->frames;
	_frameIndex = instant && _frames.size() > 0 ? _frames.size() - 1 : 0;
	_frameDuration = 1.0 / _getFps(fps, anim->fps);
	_loop = loop || anim->loop;
	_instant = instant;

	clear();
	for (int i = 0; i < _anim->layers.size(); i++) {
		const ObjectAnimation &layer = _anim->layers[i];
		Anim *node = new Anim(_obj);
		node->setAnim(&layer, fps, loop, instant);
		addChild(node);
	}
}

void Anim::trigSound() {
	if ((_anim->triggers.size() > 0) && _frameIndex < _anim->triggers.size()) {
		const Common::String &trigger = _anim->triggers[_frameIndex];
		if (trigger.size() > 0) {
			_obj->trig(trigger);
		}
	}
}

void Anim::update(float elapsed) {
	if (_anim)
		setVisible(Common::find(_obj->_hiddenLayers.begin(), _obj->_hiddenLayers.end(), _anim->name) == NULL);
	if (_instant)
		disable();
	else if (_frames.size() != 0) {
		_elapsed += elapsed;
		if (_elapsed > _frameDuration) {
			_elapsed = 0;
			if (_frameIndex < _frames.size() - 1) {
				_frameIndex++;
				trigSound();
			} else if (_loop) {
				_frameIndex = 0;
				trigSound();
			} else {
				disable();
			}
		}
		if (_anim->offsets.size() > 0) {
			Math::Vector2d off = _frameIndex < _anim->offsets.size() ? _anim->offsets[_frameIndex] : Math::Vector2d();
			if (_obj->getFacing() == FACE_LEFT) {
				off.setX(-off.getX());
			}
			_offset = off;
		}
	} else if (_children.size() != 0) {
		bool disabled = true;
		for (int i = 0; i < _children.size(); i++) {
			Anim *layer = static_cast<Anim *>(_children[i]);
			layer->update(elapsed);
			disabled = disabled && layer->_disabled;
		}
		if (disabled) {
			disable();
		}
	} else {
		disable();
	}
}

void Anim::drawCore(Math::Matrix4 trsf) {
	if (_frameIndex < _frames.size()) {
		const Common::String &frame = _frames[_frameIndex];
		bool flipX = _obj->getFacing() == FACE_LEFT;
		if (_sheet.size() == 0) {
			_sheet = _obj->_sheet;
			if (_sheet.size() == 0 && _obj->_room) {
				_sheet = _obj->_room->_sheet;
			}
		}
		SpriteSheet *sheet = g_engine->_resManager.spriteSheet(_sheet);
		const SpriteSheetFrame &sf = sheet->frameTable[frame];
		Texture *texture = g_engine->_resManager.texture(sheet->meta.image);
		float x = flipX ? -0.5f * (-1.f + sf.sourceSize.getX()) + sf.frame.width() + sf.spriteSourceSize.left : 0.5f * (-1.f + sf.sourceSize.getX()) - sf.spriteSourceSize.left;
		float y = 0.5f * (sf.sourceSize.getY() + 1.f) - sf.spriteSourceSize.height() - sf.spriteSourceSize.top;
		Math::Vector3d pos(int(-x), int(y), 0.f);
		trsf.translate(pos);
		g_engine->getGfx().drawSprite(sf.frame, *texture, getComputedColor(), trsf, flipX);
	}
}

ActorNode::ActorNode(Object *obj)
	: Node(obj->_key), _object(obj) {
}

int ActorNode::getZSort() const { return getPos().getY(); }

Math::Vector2d ActorNode::getScale() const {
	float y = _object->_room->getScaling(_object->_node->getPos().getY());
	return Math::Vector2d(y, y);
}

TextNode::TextNode() : Node("text") {
}

TextNode::~TextNode() {}

void TextNode::setText(Text text) {
	_text = text;
	setSize(text.getBounds());
}

void TextNode::updateBounds() {
	setSize(_text.getBounds());
}

Rectf TextNode::getRect() const {
	Math::Vector2d size = _size * _scale;
	return Rectf::fromPosAndSize(getAbsPos() + Math::Vector2d(0, -size.getY()) + Math::Vector2d(-size.getX(), size.getY()) * _anchorNorm, size);
}

void TextNode::onColorUpdated(Color color) {
	_text.setColor(color);
}

void TextNode::drawCore(Math::Matrix4 trsf) {
	_text.draw(g_engine->getGfx(), trsf);
}

Scene::Scene() : Node("Scene") {}
Scene::~Scene() {}

} // namespace Twp
