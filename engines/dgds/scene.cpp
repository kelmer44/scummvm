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

#include "common/debug.h"
#include "common/endian.h"
#include "common/file.h"
#include "common/rect.h"
#include "common/system.h"
#include "graphics/cursorman.h"

#include "graphics/surface.h"
#include "graphics/primitives.h"

#include "dgds/dgds.h"
#include "dgds/includes.h"
#include "dgds/resource.h"
#include "dgds/request.h"
#include "dgds/scene.h"
#include "dgds/scripts.h"
#include "dgds/font.h"
#include "dgds/globals.h"

namespace Dgds {

template<class S> Common::String _dumpStructList(const Common::String &indent, const Common::String &name, const Common::Array<S> &list) {
	if (list.empty())
		return "";

	const Common::String nextind = indent + "    ";
	Common::String str = Common::String::format("\n%s%s=", Common::String(indent + "  ").c_str(), name.c_str());
	for (const auto &s : list) {
		str += "\n";
		str += s.dump(nextind);
	}
	return str;
}


Common::String Rect::dump(const Common::String &indent) const {
	return Common::String::format("%sRect<%d,%d %d,%d>", indent.c_str(), x, y, width, height);
}

Common::String _sceneConditionStr(SceneCondition cflag) {
	Common::String ret;

	if (cflag & kSceneCondAlwaysTrue)
		return "true";

	if (cflag & kSceneCondSceneState)
		ret += "state|";
	if (cflag & kSceneCondNeedItemField12)
		ret += "item12|";
	if (cflag & kSceneCondNeedItemField14)
		ret += "item14|";
	if ((cflag & (kSceneCondSceneState | kSceneCondNeedItemField12 | kSceneCondNeedItemField14)) == 0)
		ret += "global|";

	cflag = static_cast<SceneCondition>(cflag & ~(kSceneCondSceneState | kSceneCondNeedItemField12 | kSceneCondNeedItemField14));
	if (cflag == kSceneCondNone)
		ret += "nocond";
	if (cflag & kSceneCondLessThan)
		ret += "less";
	if (cflag & kSceneCondEqual)
		ret += "equal";
	if (cflag & kSceneCondNegate)
		ret += "-not";

	return ret;
}

Common::String SceneConditions::dump(const Common::String &indent) const {
	return Common::String::format("%sSceneCondition<flg 0x%02x(%s) num %d val %d>", indent.c_str(),
			_flags, _sceneConditionStr(_flags).c_str(), _num, _val);
}


Common::String HotArea::dump(const Common::String &indent) const {
	Common::String str = Common::String::format("%sHotArea<%s num %d cursor %d",
			indent.c_str(), rect.dump("").c_str(), _num, _cursorNum);
	str += _dumpStructList(indent, "enableConditions", enableConditions);
	str += _dumpStructList(indent, "opList1", opList1);
	str += _dumpStructList(indent, "opList2", opList2);
	str += _dumpStructList(indent, "onClickOps", onClickOps);
	str += "\n";
	str += indent + ">";
	return str;
}


static Common::String _sceneOpCodeName(SceneOpCode code) {
	switch (code) {
	case kSceneOpNone: 		  	return "none";
	case kSceneOpChangeScene: 	return "changeScene";
	case kSceneOpNoop:		  	return "noop";
	case kSceneOpGlobal:		return "global";
	case kSceneOpSegmentStateOps:   return "sceneOpSegmentStateOps";
	case kSceneOpSetItemAttr:   return "setitemattr?";
	case kSceneOpGiveItem:      return "giveitem?";
	case kSceneOpShowDlg:		return "showdlg";
	case kSceneOpEnableTrigger: return "enabletrigger";
	case kSceneOpChangeSceneToStored: return "changeSceneToStored";
	case kSceneOpRestartGame:   return "restartGame";
	case kSceneOpShowClock:		return "sceneOpShowClock";
	case kSceneOpHideClock:		return "sceneOpHideClock";
	case kSceneOpShowMouse:		return "sceneOpShowMouse";
	case kSceneOpHideMouse:		return "sceneOpHideMouse";
	case kSceneOpMeanwhile:   	return "meanwhile";
	case kSceneOpOpenPlaySkipIntroMenu: return "openPlaySkipIntroMovie";
	default:
		return Common::String::format("sceneOp%d", (int)code);
	}
}

Common::String SceneOp::dump(const Common::String &indent) const {
	Common::String argsStr;
	if (_args.empty()) {
		argsStr = "[]";
	} else {
		argsStr = "[";
		for  (uint i : _args)
			argsStr += Common::String::format("%d ", i);
		argsStr.setChar(']', argsStr.size() - 1);
	}
	Common::String str = Common::String::format("%sSceneOp<op: %s args: %s", indent.c_str(), _sceneOpCodeName(_opCode).c_str(), argsStr.c_str());

	str += _dumpStructList(indent, "conditionList", _conditionList);
	if (!_conditionList.empty()) {
		str += "\n";
		str += indent;
	}
	str += ">";
	return str;
}

Common::String GameItem::dump(const Common::String &indent) const {
	Common::String super = HotArea::dump(indent + "  ");

	Common::String str = Common::String::format(
			"%sGameItem<\n%s\n%sunk10 %d icon %d unk12 %d flags %d unk14 %d",
			indent.c_str(), super.c_str(), indent.c_str(), field10_0x24,
			_iconNum, field12_0x28, _flags, field14_0x2c);
	str += _dumpStructList(indent, "opList4", opList4);
	str += _dumpStructList(indent, "opList5", opList5);
	str += "\n";
	str += indent + ">";
	return str;
}


Common::String MouseCursor::dump(const Common::String &indent) const {
	return Common::String::format("%sMouseCursor<%d %d>", indent.c_str(), _hotX, _hotY);
}


Common::String SceneStruct4::dump(const Common::String &indent) const {
	Common::String str = Common::String::format("%sSceneStruct4<%d %d", indent.c_str(), val1, val2);

	str += _dumpStructList(indent, "opList", opList);
	str += "\n";
	str += indent + ">";
	return str;
}


Common::String SceneTrigger::dump(const Common::String &indent) const {
	Common::String str = Common::String::format("%sSceneTrigger<num %d %s", indent.c_str(), _num, _enabled ? "enabled" : "disabled");
	str += _dumpStructList(indent, "conditionList", conditionList);
	str += _dumpStructList(indent, "opList", sceneOpList);
	str += "\n";
	str += indent + ">";
	return str;
}


int Dialog::_lastSelectedDialogItemNum = 0;
Dialog *Dialog::_lastDialogSelectionChangedFor = nullptr;


Dialog::Dialog() : _num(0), _bgColor(0), _fontColor(0), _selectionBgCol(0), _selectonFontCol(0),
	_fontSize(0), _flags(kDlgFlagNone), _frameType(kDlgFramePlain), _time(0), _nextDialogNum(0),
	_field18_0x28(0)
{}


void Dialog::draw(Graphics::Surface *dst, DialogDrawStage stage) {
	if (!_state) {
		_state.reset(new DialogState());
	}

	switch (_frameType) {
	case kDlgFramePlain: 	return drawType1(dst, stage);
	case kDlgFrameBorder: 	return drawType2(dst, stage);
	case kDlgFrameThought: 	return drawType3(dst, stage);
	case kDlgFrameRounded: 	return drawType4(dst, stage);
	default: error("unexpected frame type %d for dialog %d", _frameType, _num);
	}
}

static void _drawPixel(int x, int y, int color, void *data) {
	Graphics::Surface *surface = (Graphics::Surface *)data;

	if (x >= 0 && x < surface->w && y >= 0 && y < surface->h)
		*((byte *)surface->getBasePtr(x, y)) = (byte)color;
}


const Font *Dialog::getDlgTextFont() const {
	DgdsEngine *engine = static_cast<DgdsEngine *>(g_engine);
	const FontManager *fontman = engine->getFontMan();
	FontManager::FontType fontType = FontManager::kGameDlgFont;
	if (_fontSize == 1)
		fontType = FontManager::k8x8Font;
	else if (_fontSize == 3)
		fontType = FontManager::k4x5Font;
	return fontman->getFont(fontType);
}

//  box with simple frame
void Dialog::drawType1(Graphics::Surface *dst, DialogDrawStage stage) {
	if (!_state)
		return;
	int x = _rect.x;
	int y = _rect.y;
	int w = _rect.width;
	int h = _rect.height;

	if (stage == kDlgDrawStageBackground) {
		dst->fillRect(Common::Rect(x, y, x + w, y + h), _bgColor);
		dst->fillRect(Common::Rect(x + 1, y + 1, x + w - 1, y + h - 1), _fontColor);
	} else if (stage == kDlgDrawFindSelectionPointXY) {
		drawFindSelectionXY();
	} else if (stage == kDlgDrawFindSelectionTxtOffset) {
		drawFindSelectionTxtOffset();
	} else {
		_textDrawRect = Common::Rect(x + 3, y + 3, x + w - 3, y + h - 3);
		drawForeground(dst, _bgColor, _str);
	}
}

// box with fancy frame and optional title (everything before ":")
void Dialog::drawType2(Graphics::Surface *dst, DialogDrawStage stage) {
	if (!_state)
		return;

	Common::String title;
	Common::String txt;
	uint32 colonpos = _str.find(':');
	if (colonpos != Common::String::npos) {
		title = _str.substr(0, colonpos);
		txt = _str.substr(colonpos + 1);
	} else {
		txt = _str;
	}

	if (stage == kDlgDrawStageBackground) {
		_textDrawRect = Common::Rect (_rect.x + 6, _rect.y + 6, _rect.x + _rect.width - 6, _rect.y + _rect.height - 6);
		Common::Rect drawRect(_rect.x, _rect.y, _rect.x + _rect.width, _rect.y + _rect.height);
		RequestData::fillBackground(dst, _rect.x, _rect.y, _rect.width, _rect.height, 0);
		RequestData::drawCorners(dst, 11, _rect.x, _rect.y, _rect.width, _rect.height);
		if (!title.empty()) {
			// TODO: Maybe should measure the font?
			_textDrawRect.top += 10;
			RequestData::drawHeader(dst, _rect.x, _rect.y, _rect.width, 4, title);
		}
		if (hasFlag(kDlgFlagFlatBg))
			dst->fillRect(_textDrawRect, 0);
		else
			RequestData::fillBackground(dst, _textDrawRect.left, _textDrawRect.top, _textDrawRect.width(), _textDrawRect.height(), 6);

		RequestData::drawCorners(dst, 19, _textDrawRect.left - 2, _textDrawRect.top - 2, _textDrawRect.width() + 4, _textDrawRect.height() + 4);

		_textDrawRect.left += 8;
		_textDrawRect.right -= 8;
	} else if (stage == kDlgDrawFindSelectionPointXY) {
		drawFindSelectionXY();
	} else if (stage == kDlgDrawFindSelectionTxtOffset) {
		drawFindSelectionTxtOffset();
	} else {
		drawForeground(dst, _fontColor, txt);
	}
}

static void _filledCircle(int x, int y, int xr, int yr, Graphics::Surface *dst, byte fgcol, byte bgcol) {
	Graphics::drawEllipse(x - xr, y - yr, x + xr, y + yr, bgcol, true, _drawPixel, dst);
	Graphics::drawEllipse(x - xr, y - yr, x + xr, y + yr, fgcol, false, _drawPixel, dst);
}

// Comic tought box made up of circles with 2 circles going up to it.
// Draw circles with 5/4 more pixels in x because the pixels are not square.
void Dialog::drawType3(Graphics::Surface *dst, DialogDrawStage stage) {
	if (!_state)
		return;

	if (stage == kDlgDrawStageBackground) {
		uint16 xradius = 9999;
		uint16 yradius = 40;
		const int16 usabley = _rect.height - 31;
		const int16 usablex = _rect.width - 30;
		for (uint16 testyradius = 40; testyradius != 0; testyradius--) {
			int16 testxradius = (testyradius * 5) / 4;
			if ((usablex / testxradius > 2) && (usabley / testyradius > 2)) {
				testxradius = usablex % testxradius + usabley % testyradius;
				if (testxradius < xradius) {
					yradius = testyradius;
					xradius = testxradius;
				}
			}
			if (testyradius < 20 && xradius != 9999)
				break;
		}

		xradius = (yradius * 5) / 4;
		const int16 circlesAcross = usablex / xradius - 1;
		const int16 circlesDown = usabley / yradius - 1;

		uint16 x = _rect.x + xradius;
		uint16 y = _rect.y + yradius;

		bool isbig = _rect.x + _rect.width / 2 > 160;
		if (isbig)
			x = x + 30;

		byte fgcol = 0;
		byte bgcol = 15;
		if (hasFlag(kDlgFlagFlatBg)) {
			bgcol = _bgColor;
			fgcol = _fontColor;
		}

		for (int i = 1; i < circlesDown; i++) {
			_filledCircle(x, y, xradius, yradius, dst, fgcol, bgcol);
			y += yradius;
		}
		for (int i = 1; i < circlesAcross; i++) {
			_filledCircle(x, y, xradius, yradius, dst, fgcol, bgcol);
			x += xradius;
		}
		for (int i = 1; i < circlesDown; i++) {
			_filledCircle(x, y, xradius, yradius, dst, fgcol, bgcol);
			y -= yradius;
		}
		for (int i = 1; i < circlesAcross; i++) {
			_filledCircle(x, y, xradius, yradius, dst, fgcol, bgcol);
			x -= xradius;
		}

		uint16 smallCircleX;
		if (isbig) {
			_filledCircle((x - xradius) - 5, y + circlesDown * yradius + 5, 10, 8, dst, fgcol, bgcol);
			smallCircleX = (x - xradius) - 20;
		} else {
			_filledCircle(x + circlesAcross * xradius + 5, y + circlesDown * yradius + 5, 10, 8, dst, fgcol, bgcol);
			smallCircleX = x + circlesAcross * xradius + 20;
		}

		_filledCircle(smallCircleX, y + circlesDown * yradius + 25, 5, 4, dst, fgcol, bgcol);

		int16 yoff = (yradius * 27) / 32;
		dst->fillRect(Common::Rect(x, y - yoff,
					x + (circlesAcross - 1) * xradius + 1,
					y + (circlesDown - 1) * yradius + yoff + 1), bgcol);
		int16 xoff = (xradius * 27) / 32;
		dst->fillRect(Common::Rect(x - xoff, y,
					x + (circlesAcross - 1) * xradius + xoff + 1,
					y + (circlesDown - 1) * yradius + 1), bgcol);

		int16 textRectX = x - xradius / 2;
		int16 textRectY = y - yradius / 2;
		_textDrawRect = Common::Rect(textRectX, textRectY, textRectX + circlesAcross * xradius , textRectY + circlesDown * yradius);
	} else if (stage == kDlgDrawFindSelectionPointXY) {
		drawFindSelectionXY();
	} else if (stage == kDlgDrawFindSelectionTxtOffset) {
		drawFindSelectionTxtOffset();
	} else {
		drawForeground(dst, _fontColor, _str);
	}
}

// ellipse
void Dialog::drawType4(Graphics::Surface *dst, DialogDrawStage stage) {
	if (!_state)
		return;

	int x = _rect.x;
	int y = _rect.y;
	int w = _rect.width;
	int h = _rect.height;

	int midy = (h - 1) / 2;
	byte fillcolor;
	byte fillbgcolor;
	if (!hasFlag(kDlgFlagFlatBg)) {
		fillcolor = 0;
		fillbgcolor = 15;
	} else {
		fillcolor = _fontColor;
		fillbgcolor = _bgColor;
	}

	if (stage == kDlgDrawStageBackground) {
		//int radius = (midy * 5) / 4;

		// This is not exactly the same as the original - might need some work to get pixel-perfect
		Common::Rect drawRect(x, y, x + w, y + h);
		Graphics::drawRoundRect(drawRect, midy, fillbgcolor, true, _drawPixel, dst);
		Graphics::drawRoundRect(drawRect, midy, fillcolor, false, _drawPixel, dst);
	} else if (stage == kDlgDrawFindSelectionPointXY) {
		drawFindSelectionXY();
	} else if (stage == kDlgDrawFindSelectionTxtOffset) {
		drawFindSelectionTxtOffset();
	} else {
		_textDrawRect = Common::Rect(x + midy, y + 1, x + w - midy, y + h - 1);
		drawForeground(dst, fillcolor, _str);
	}
}

void Dialog::drawFindSelectionXY() {
	if (!_state)
		return;

	const Font *font = getDlgTextFont();

	// Find the appropriate _lastMouseX/lastMouseY value given the last _strMouseLoc.

	int x = _state->_loc.x;
	_state->_lastMouseX = x;
	int y = _state->_loc.y + 1;
	_state->_lastMouseY = y;
	_state->_charWidth = font->getMaxCharWidth();
	_state->_charHeight = font->getFontHeight();
	if (_state->_strMouseLoc) {
		Common::Array<Common::String> lines;
		int maxWidth = font->wordWrapText(_str, _state->_loc.width, lines);

		if (hasFlag(kDlgFlagLeftJust)) {
			x = x + (_state->_loc.width - maxWidth - 1) / 2;
			_state->_lastMouseX = x;
			y = y + (_state->_loc.height - (lines.size() * _state->_charHeight) - 1) / 2;
			_state->_lastMouseY = y;
		}

		if (_state->_strMouseLoc >= (int)_str.size())
			_state->_strMouseLoc = _str.size() - 1;

		// Find the location of the mouse loc in the wrapped string.
		int totalchars = 0;
		for (uint lineno = 0; lineno < lines.size(); lineno++) {
			// +1 char for the space or CR that caused the wrap.
			int nexttotalchars = totalchars + lines[lineno].size() + 1;
			if (nexttotalchars > _state->_strMouseLoc)
				break;
			totalchars = nexttotalchars;
			y += _state->_charHeight;
		}

		// now get width of the remaining string to the mouse str offset
		x += font->getStringWidth(_str.substr(totalchars, _state->_strMouseLoc - totalchars));

		// TODO: does this make sense?
		if (_state->_loc.x + _state->_loc.width < (x + font->getCharWidth(_state->_strMouseLoc))) {
			if (_str[_state->_strMouseLoc] < '!') {
				_state->_charHeight = 0;
				_state->_charWidth = 0;
				_state->_lastMouseY = 0;
				_state->_lastMouseX = 0;
				return;
			}
			x = _state->_loc.x;
			y += _state->_charHeight;
		}

		_state->_lastMouseX = x;
		_state->_lastMouseY = y;
		_state->_charWidth = font->getCharWidth(_state->_strMouseLoc);
	}
}

void Dialog::drawFindSelectionTxtOffset() {
	if (!_state)
		return;

	// Find the appropriate _strMouseLoc value given the last x/y position.

	const Font *font = getDlgTextFont();
	int lastMouseX = _state->_lastMouseX;
	int lastMouseY = _state->_lastMouseY;
	int lineHeight = font->getFontHeight();
	int dlgx = _state->_loc.x;
	int dlgy = _state->_loc.y;

	Common::Array<Common::String> lines;
	int maxWidth = font->wordWrapText(_str, dlgy, lines);

	if (hasFlag(kDlgFlagLeftJust)) {
		dlgx += (_state->_loc.width - maxWidth - 1) / 2;
		dlgy += (_state->_loc.height - (lines.size() * lineHeight) - 1) / 2;
	}

	uint lineno;
	uint totalchars = 0;
	for (lineno = 0; lineno < lines.size() && dlgy + lineHeight < lastMouseY; lineno++) {
		totalchars += lines[lineno].size() + 1;
		dlgy = dlgy + lineHeight;
	}

	int startx = dlgx;
	while (lineno < lines.size()) {
		const Common::String &line = lines[lineno];
		for (uint charno = 0; charno < line.size(); charno++) {
			int charwidth = font->getCharWidth(line[charno]);
			if (lastMouseX <= dlgx + charwidth) {
				_state->_strMouseLoc = totalchars + charno;
				return;
			}
			dlgx += charwidth;
		}
		dlgx = startx;
		totalchars += line.size() + 1;
	}

	_state->_strMouseLoc = _str.size();
	return;
}

void Dialog::drawForeground(Graphics::Surface *dst, uint16 fontcol, const Common::String &txt) {
	// TODO: some more text calcuations happen here.
	// This is where we actually draw the text.
	// For now do the simplest wrapping, no highlighting.
	Common::StringArray lines;
	const Font *font = getDlgTextFont();
	const int h = font->getFontHeight();
	font->wordWrapText(txt, _textDrawRect.width(), lines);

	int ystart = _textDrawRect.top + (_textDrawRect.height() - lines.size() * h) / 2;

	int x = _textDrawRect.left;
	if (hasFlag(kDlgFlagLeftJust)) {
		// each line left-aligned, but overall block is still centered
		int maxlen = -1;
		for (const auto &line : lines)
			maxlen = MAX(maxlen, font->getStringWidth(line));

		x += (_textDrawRect.width() - maxlen) / 2;

		for (uint i = 0; i < lines.size(); i++)
			font->drawString(dst, lines[i], x, ystart + i * h, maxlen, fontcol, Graphics::kTextAlignLeft);
	} else {
		// center each line
		for (uint i = 0; i < lines.size(); i++)
			font->drawString(dst, lines[i], x, ystart + i * h, _textDrawRect.width(), fontcol, Graphics::kTextAlignCenter);
	}

	if (_state->_selectedAction) {
		warning("TODO: Draw highlight on selected action.");
	}
}

void Dialog::setFlag(DialogFlags flg) {
	_flags = static_cast<DialogFlags>(_flags | flg);
}

void Dialog::clearFlag(DialogFlags flg) {
	_flags = static_cast<DialogFlags>(_flags & ~flg);
}

void Dialog::flipFlag(DialogFlags flg) {
	_flags = static_cast<DialogFlags>(_flags ^ flg);
}

bool Dialog::hasFlag(DialogFlags flg) const {
	return _flags & flg;
}

void Dialog::updateSelectedAction(int delta) {
	if (!_lastDialogSelectionChangedFor)
		_lastDialogSelectionChangedFor = this;
	else
		_lastSelectedDialogItemNum = 0;

	if (!_state)
		return;

	if (_state->_selectedAction) {
		for (uint i = 0; i < _action.size(); i++) {
			if (_state->_selectedAction == &_action[i]) {
				_lastSelectedDialogItemNum = i;
				break;
			}
		}
	}
	_lastSelectedDialogItemNum += delta;
	if (!_action.empty()) {
		while (_lastSelectedDialogItemNum < 0)
			_lastSelectedDialogItemNum += _action.size();
		_lastSelectedDialogItemNum = _lastSelectedDialogItemNum % _action.size();
	}
	_lastDialogSelectionChangedFor = this;

	int mouseX = _state->_loc.x + _state->_loc.width;
	int mouseY = _state->_loc.y + _state->_loc.height - 2;
	if (_action.size() > 1) {
		_state->_strMouseLoc = _action[_lastSelectedDialogItemNum].strStart;
		draw(nullptr, kDlgDrawFindSelectionPointXY);
		// Move the mouse over the selected item
		mouseY = _state->_lastMouseY + _state->_charHeight / 2;
	}

	if (_action.size() > 1 || !delta) {
		g_system->warpMouse(mouseX, mouseY);
	}
}

struct DialogAction *Dialog::pickAction(bool isClosing) {
	struct DialogAction *retval = nullptr;
	DgdsEngine *engine = static_cast<DgdsEngine *>(g_engine);
	if (/* some game flags?? && */isClosing) {
		return &_action[engine->getRandom().getRandomNumber(_action.size() - 1)];
	}
	assert(_state);
	const Common::Point lastMouse = engine->getLastMouse();
	if (_state->_loc.x <= lastMouse.x &&
		_state->_loc.x + _state->_loc.width <= lastMouse.x &&
		_state->_loc.y <= lastMouse.y &&
		_state->_loc.y + _state->_loc.height <= lastMouse.y) {
		_state->_lastMouseX = lastMouse.x;
		_state->_lastMouseY = lastMouse.y;
		draw(nullptr, kDlgDrawFindSelectionTxtOffset);

		char underMouse = _str[_state->_strMouseLoc];
		for (auto &action : _action) {
			if ((action.strStart <= _state->_strMouseLoc && _state->_strMouseLoc <= action.strEnd) ||
				(_state->_strMouseLoc == action.strEnd + 1 && underMouse == '\r' && _str[action.strEnd] != '\r')) {
				return &action;
			}
		}
	}
	return retval;
}


Common::String Dialog::dump(const Common::String &indent) const {
	Common::String str = Common::String::format(
			"%sDialog<num %d %s bgcol %d fcol %d selbgcol %d selfontcol %d fntsz %d flags 0x%02x frame %d delay %d next %d unk18 %d",
			indent.c_str(), _num, _rect.dump("").c_str(), _bgColor, _fontColor, _selectionBgCol, _selectonFontCol, _fontSize,
			_flags, _frameType, _time, _nextDialogNum, _field18_0x28);
	str += indent + "state=" + (_state ? _state->dump("") : "null");
	str += "\n";
	str += _dumpStructList(indent, "actions", _action);
	str += "\n";
	str += indent + "  str='" + _str + "'>";
	return str;
}

Common::String DialogState::dump(const Common::String &indent) const {
	return Common::String::format("%sDialogState<hide %d loc %s lastmouse %d %d charsz %d %d mousestr %d selaction %p>",
			indent.c_str(), _hideTime, _loc.dump("").c_str(), _lastMouseX, _lastMouseY, _charWidth,
			_charHeight, _strMouseLoc, _selectedAction);
}

Common::String DialogAction::dump(const Common::String &indent) const {
	Common::String str = Common::String::format("%sDialogueAction<%d span: %d-%d", indent.c_str(), val, strStart, strEnd);
	str += _dumpStructList(indent, "opList", sceneOpList);
	if (!sceneOpList.empty()) {
		str += "\n";
		str += indent;
	}
	str += ">";
	return str;
}

Common::String PerSceneGlobal::dump(const Common::String &indent) const {
	return Common::String::format("%sPerSceneGlobal<num %d scene %d val %d>", indent.c_str(), _num, _sceneNo, _val);
}

// //////////////////////////////////// //

Scene::Scene() : _magic(0) {
}

bool Scene::isVersionOver(const char *version) const {
	return strncmp(_version.c_str(), version, _version.size()) > 0;
}

bool Scene::isVersionUnder(const char *version) const {
	return strncmp(_version.c_str(), version, _version.size()) < 0;
}


bool Scene::readConditionList(Common::SeekableReadStream *s, Common::Array<SceneConditions> &list) const {
	list.resize(s->readUint16LE());
	for (SceneConditions &dst : list) {
		dst._num = s->readUint16LE();
		dst._flags = static_cast<SceneCondition>(s->readUint16LE());
		dst._val = s->readUint16LE();
	}
	return !s->err();
}


bool Scene::readHotArea(Common::SeekableReadStream *s, HotArea &dst) const {
	dst.rect.x = s->readUint16LE();
	dst.rect.y = s->readUint16LE();
	dst.rect.width = s->readUint16LE();
	dst.rect.height = s->readUint16LE();
	dst._num = s->readUint16LE();
	dst._cursorNum = s->readUint16LE();
	readConditionList(s, dst.enableConditions);
	readOpList(s, dst.opList1);
	readOpList(s, dst.opList2);
	readOpList(s, dst.onClickOps);
	return !s->err();
}


bool Scene::readHotAreaList(Common::SeekableReadStream *s, Common::Array<HotArea> &list) const {
	list.resize(s->readUint16LE());
	for (HotArea &dst : list) {
		readHotArea(s, dst);
	}
	return !s->err();
}


bool Scene::readGameItemList(Common::SeekableReadStream *s, Common::Array<GameItem> &list) const {
	list.resize(s->readUint16LE());
	for (GameItem &dst : list) {
		readHotArea(s, dst);
	}
	for (GameItem &dst : list) {
		dst._iconNum = s->readUint16LE();
		dst.field12_0x28 = s->readUint16LE();
		dst.field14_0x2c = s->readUint16LE();
		if (!isVersionUnder(" 1.211"))
			dst._flags = s->readUint16LE() & 0xfffe;
		if (!isVersionUnder(" 1.204")) {
			dst.field10_0x24 = s->readUint16LE();
			readOpList(s, dst.opList4);
			readOpList(s, dst.opList5);
		}
	}
	return !s->err();
}


bool Scene::readMouseHotspotList(Common::SeekableReadStream *s, Common::Array<MouseCursor> &list) const {
	list.resize(s->readUint16LE());
	for (MouseCursor &dst : list) {
		dst._hotX = s->readUint16LE();
		dst._hotY = s->readUint16LE();
	}
	return !s->err();
}


bool Scene::readStruct4List(Common::SeekableReadStream *s, Common::Array<SceneStruct4> &list) const {
	list.resize(s->readUint16LE());
	for (SceneStruct4 &dst : list) {
		if (!isVersionOver(" 1.205")) {
			dst.val2 = s->readUint16LE();
			dst.val1 = s->readUint16LE();
			dst.val2 += s->readUint16LE();
		} else {
			dst.val1 = s->readUint16LE();
			dst.val2 = s->readUint16LE();
		}
		readOpList(s, dst.opList);
	}
	return !s->err();
}


bool Scene::readOpList(Common::SeekableReadStream *s, Common::Array<SceneOp> &list) const {
	list.resize(s->readUint16LE());
	for (SceneOp &dst : list) {
		readConditionList(s, dst._conditionList);
		dst._opCode = static_cast<SceneOpCode>(s->readUint16LE());
		int nvals = s->readUint16LE();
		for (int i = 0; i < nvals / 2; i++) {
			dst._args.push_back(s->readUint16LE());
		}
	}

	return !s->err();
}


bool Scene::readDialogList(Common::SeekableReadStream *s, Common::Array<Dialog> &list) const {
	// Some data on this format here https://www.oldgamesitalia.net/forum/index.php?showtopic=24055&st=25&p=359214&#entry359214

	list.resize(s->readUint16LE());
	for (Dialog &dst : list) {
		dst._num = s->readUint16LE();
		dst._rect.x = s->readUint16LE();
		dst._rect.y = s->readUint16LE();
		dst._rect.width = s->readUint16LE();
		dst._rect.height = s->readUint16LE();
		dst._bgColor = s->readUint16LE();
		dst._fontColor = s->readUint16LE(); // 0 = black, 0xf = white
		if (isVersionUnder(" 1.209")) {
			dst._selectionBgCol = dst._bgColor;
			dst._selectonFontCol = dst._fontColor;
		} else {
			dst._selectionBgCol = s->readUint16LE();
			dst._selectonFontCol = s->readUint16LE();
		}
		dst._fontSize = s->readUint16LE(); // 01 = 8x8, 02 = 6x6, 03 = 4x5
		if (isVersionUnder(" 1.210")) {
			dst._flags = static_cast<DialogFlags>(s->readUint16LE());
		} else {
			// Game reads a 32 bit int but then truncates anyway..
			// probably never used the full thing in SDS files
			// as most higher bits are render state.
			dst._flags = static_cast<DialogFlags>(s->readUint32LE() & 0xffff);
		}

		dst._frameType = static_cast<DialogFrameType>(s->readUint16LE());
		dst._time = s->readUint16LE();
		if (isVersionOver(" 1.207")) {
			dst._nextDialogNum = s->readUint16LE();
		}

		uint16 nbytes = s->readUint16LE();
		if (nbytes > 0) {
			dst._str = s->readString('\0', nbytes);
		} else {
			dst._str.clear();
		}
		readDialogActionList(s, dst._action);

		if (isVersionUnder(" 1.209") && !dst._action.empty()) {
			if (dst._fontColor == 0)
				dst._selectonFontCol = 4;
			else if (dst._fontColor == 0xff)
				dst._fontColor = 7;
			else
				dst._fontColor = dst._fontColor ^ 8;
		}
	}

	return !s->err();
}


bool Scene::readTriggerList(Common::SeekableReadStream *s, Common::Array<SceneTrigger> &list) const {
	list.resize(s->readUint16LE());
	for (SceneTrigger &dst : list) {
		dst._num = s->readUint16LE();
		dst._enabled = false;
		readConditionList(s, dst.conditionList);
		readOpList(s, dst.sceneOpList);
	}

	return !s->err();
}


bool Scene::readDialogActionList(Common::SeekableReadStream *s, Common::Array<DialogAction> &list) const {
	list.resize(s->readUint16LE());

	if (!list.empty())
		list[0].val = 1;

	for (DialogAction &dst : list) {
		dst.strStart = s->readUint16LE();
		dst.strEnd = s->readUint16LE();
		readOpList(s, dst.sceneOpList);
	}

	return !s->err();
}


void SDSScene::enableTrigger(uint16 num) {
	for (auto &trigger : _triggers) {
		if (trigger._num == num)
			trigger._enabled = true;
	}
}


void Scene::segmentStateOps(const Common::Array<uint16> &args) {
	ADSInterpreter *interp = static_cast<DgdsEngine *>(g_engine)->adsInterpreter();

	for (uint i = 0; i < args.size(); i += 2) {
		uint16 subop = args[i];
		uint16 arg = args[i + 1];
		if (!subop && !arg)
			return;
		switch (subop) {
		case 1:
			interp->segmentOrState(arg, 3);
			break;
		case 2:
			interp->segmentOrState(arg, 4);
			break;
		case 3:
			interp->segmentSetState(arg, 6);
			break;
		case 4:
			interp->segmentSetState(arg, 5);
			break;
		case 9:
			warning("TODO: Apply segment state 3 to all loaded ADS texts");
			interp->segmentOrState(arg, 3);
			break;
		case 10:
			warning("TODO: Apply segment state 4 to all loaded ADS texts");
			interp->segmentOrState(arg, 4);
			break;
		case 11:
			warning("TODO: Apply segment state 6 to all loaded ADS texts");
			interp->segmentSetState(arg, 6);
			break;
		case 12:
			warning("TODO: Apply segment state 5 to all loaded ADS texts");
			interp->segmentSetState(arg, 5);
			break;
		default:
			error("Unknown scene op 4 sub-opcode %d", subop);
		}
	}
}


bool Scene::runOps(const Common::Array<SceneOp> &ops) {
	DgdsEngine *engine = static_cast<DgdsEngine *>(g_engine);
	for (const SceneOp &op : ops) {
		debug("Exec %s", op.dump("").c_str());
		switch(op._opCode) {
		case kSceneOpChangeScene:
			if (engine->changeScene(op._args[0], true))
				// This probably reset the list - stop now.
				return false;
			break;
		case kSceneOpNoop:
			break;
		case kSceneOpGlobal:
			globalOps(op._args);
			break;
		case kSceneOpSegmentStateOps:
			segmentStateOps(op._args);
			break;
		case kSceneOpShowDlg:
			showDialog(op._args[0]);
			break;
		case kSceneOpEnableTrigger:
			enableTrigger(op._args[0]);
			break;
		case kSceneOpChangeSceneToStored: {
			uint16 sceneNo = engine->getGameGlobals()->getGlobal(0x61);
			if (engine->changeScene(sceneNo, true))
				// This probably reset the list - stop now.
				return false;
			break;
		}
		case kSceneOpRestartGame:
			error("TODO: Implement restart game scene op");
			break;
		case kSceneOpShowClock:
			engine->setShowClock(true);
			break;
		case kSceneOpHideClock:
			engine->setShowClock(false);
			break;
		case kSceneOpShowMouse:
			CursorMan.showMouse(true);
			break;
		case kSceneOpHideMouse:
			CursorMan.showMouse(false);
			break;
		case kSceneOpMeanwhile:
			warning("TODO: Implement meanwhile screen");
			break;
		case kSceneOp10:
			warning("TODO: Implement scene op 10 (find SDS hot spot?)");
			break;
		case kSceneOpOpenPlaySkipIntroMenu:
			warning("TODO: Implement scene op 107 (inject key code 0xfc, open menu to play intro or not)");
			break;
		default:
			warning("TODO: Implement scene op %d", op._opCode);
			break;
		}
	}
	return true;
}

bool Scene::checkConditions(const Common::Array<struct SceneConditions> &conds) {
	if (conds.empty())
		return true;
	uint truecount = 0;

	DgdsEngine *engine = static_cast<DgdsEngine *>(g_engine);
	Globals *globals = engine->getGameGlobals();

	for (const auto &c : conds) {
		uint16 refval = c._val;
		uint16 checkval;
		SceneCondition cflag = c._flags;
		if (cflag & kSceneCondAlwaysTrue)
			return true;

		if (cflag & kSceneCondSceneState) {
			refval = 1;
			checkval = engine->adsInterpreter()->getStateForSceneOp(c._num);
			SceneCondition equalOrNegate = static_cast<SceneCondition>(cflag & (kSceneCondEqual | kSceneCondNegate));
			if (equalOrNegate != kSceneCondEqual && equalOrNegate != kSceneCondNegate)
				refval = 0;
			cflag = kSceneCondEqual;
		} else if (cflag & kSceneCondNeedItemField14 || cflag & kSceneCondNeedItemField12) {
			debug("TODO: Check game item attribute for scene condition");
			// TODO: Get game item c._num and check value from item attributes
			checkval = 0;
		} else {
			checkval = globals->getGlobal(c._num);
			if (!(cflag & kSceneCondAbsVal))
				refval = globals->getGlobal(refval);
		}

		bool result = false;
		cflag = static_cast<SceneCondition>(cflag & ~(kSceneCondSceneState | kSceneCondNeedItemField12 | kSceneCondNeedItemField14));
		if (cflag == kSceneCondNone)
			cflag = static_cast<SceneCondition>(kSceneCondEqual | kSceneCondNegate);
		if ((cflag & kSceneCondLessThan) && checkval < refval)
			result = true;
		if ((cflag & kSceneCondEqual) && checkval == refval)
			result = true;
		if (cflag & kSceneCondNegate)
			result = !result;

		debug("Cond: %s -> %s", c.dump("").c_str(), result ? "true": "false");

		if (result)
			truecount++;
	}
	return truecount == conds.size();
}


SDSScene::SDSScene() : _num(-1) {
}

bool SDSScene::load(const Common::String &filename, ResourceManager *resourceManager, Decompressor *decompressor) {
	Common::SeekableReadStream *sceneFile = resourceManager->getResource(filename);
	if (!sceneFile)
		error("Scene file %s not found", filename.c_str());

	DgdsChunkReader chunk(sceneFile);

	bool result = false;

	while (chunk.readNextHeader(EX_SDS, filename)) {
		if (chunk.isContainer()) {
			continue;
		}

		chunk.readContent(decompressor);
		Common::SeekableReadStream *stream = chunk.getContent();

		if (chunk.isSection(ID_SDS)) {
			result = parse(stream);
		}
	}

	delete sceneFile;

	return result;
}

bool SDSScene::parse(Common::SeekableReadStream *stream) {
	_magic = stream->readUint32LE();
	_version = stream->readString();
	if (isVersionOver(" 1.211")) {
	//if (isVersionOver(" 1.216")) { // HoC
	//if (isVersionOver(" 1.224")) { // Beamish
		error("Unsupported scene version '%s'", _version.c_str());
	}
	_num = stream->readUint16LE();
	readOpList(stream, _enterSceneOps);
	readOpList(stream, _leaveSceneOps);
	if (isVersionOver(" 1.206")) {
		readOpList(stream, _preTickOps);
	}
	readOpList(stream, _postTickOps);
	_field6_0x14 = stream->readUint16LE();
	_adsFile = stream->readString();
	readHotAreaList(stream, _hotAreaList);
	readStruct4List(stream, _struct4List1);
	if (isVersionOver(" 1.205")) {
		readStruct4List(stream, _struct4List2);
	}
	readDialogList(stream, _dialogs);
	if (isVersionOver(" 1.203")) {
		readTriggerList(stream, _triggers);
	}

	return !stream->err();
}

void SDSScene::unload() {
	_num = 0;
	_enterSceneOps.clear();
	_leaveSceneOps.clear();
	_preTickOps.clear();
	_postTickOps.clear();
	_field6_0x14 = 0;
	_adsFile.clear();
	_hotAreaList.clear();
	_struct4List1.clear();
	_struct4List2.clear();
	_dialogs.clear();
	_triggers.clear();
}


Common::String SDSScene::dump(const Common::String &indent) const {
	Common::String str = Common::String::format("%sSDSScene<num %d %d ads %s", indent.c_str(), _num, _field6_0x14, _adsFile.c_str());
	str += _dumpStructList(indent, "enterSceneOps", _enterSceneOps);
	str += _dumpStructList(indent, "leaveSceneOps", _leaveSceneOps);
	str += _dumpStructList(indent, "preTickOps", _preTickOps);
	str += _dumpStructList(indent, "postTickOps", _postTickOps);
	str += _dumpStructList(indent, "hotAreaList", _hotAreaList);
	str += _dumpStructList(indent, "struct4List1", _struct4List1);
	str += _dumpStructList(indent, "struct4List2", _struct4List2);
	str += _dumpStructList(indent, "dialogues", _dialogs);
	str += _dumpStructList(indent, "triggers", _triggers);

	str += "\n";
	str += indent + ">";
	return str;
}

void SDSScene::checkTriggers() {
	// scene can change on these triggers.  if that happens we stop.
	int startSceneNum = _num;

	for (struct SceneTrigger &trigger : _triggers) {
		if (!trigger._enabled)
			continue;

		if (!checkConditions(trigger.conditionList))
			continue;

		trigger._enabled = false;
		runOps(trigger.sceneOpList);

		// If the scene changed, the list is no longer valid. Abort!
		if (_num != startSceneNum)
			return;
	}
}

void SDSScene::showDialog(uint16 num) {
	for (auto &dialog : _dialogs) {
		if (dialog._num == num) {
			dialog.clearFlag(kDlgFlagHi2);
			dialog.clearFlag(kDlgFlagHi8);
			dialog.clearFlag(kDlgFlagHi10);
			dialog.clearFlag(kDlgFlagHi20);
			dialog.clearFlag(kDlgFlagHi40);
			dialog.setFlag(kDlgFlagHi20);
			dialog.setFlag(kDlgFlagVisible);
			dialog.setFlag(kDlgFlagOpening);
			// hide time gets set the first time it's drawn.
			if (dialog.hasFlag(kDlgFlagLo8)) {
				// do something with some global dlg flags here.
			}
		}
	}
}

bool SDSScene::checkDialogActive() {
	uint32 timeNow = g_engine->getTotalPlayTime();
	bool retval = false;

	bool someFlag = false; // ((g_gameStateFlag_41f6 | UINT_39e5_41f8) & 6) != 0); ??

	for (auto &dlg : _dialogs) {
		if (!dlg.hasFlag(kDlgFlagVisible))
			continue;

		if (!dlg._state)
			dlg._state.reset(new DialogState());

		bool finished = false;
		if (dlg._state->_hideTime && dlg._state->_hideTime < timeNow) {
			finished = someFlag;
		}

		bool no_options = false;
		if ((dlg._state->_hideTime == 0) && dlg._action.size() < 2)
			no_options = true;

		if ((!finished && !no_options) || (dlg.hasFlag(kDlgFlagHi20) || dlg.hasFlag(kDlgFlagHi40))) {
			if (!finished && dlg._action.size() > 1 && !dlg.hasFlag(kDlgFlagHiFinished)) {
				struct DialogAction *action = dlg.pickAction(false);
				if (dlg._state->_selectedAction != action) {
					dlg._state->_selectedAction = action;
					dlg.clearFlag(kDlgFlagHi10);
					dlg.setFlag(kDlgFlagHi8);
				}
			}
		} else {
			// this dialog is finished - call the ops and maybe show the next one
			struct DialogAction *action = dlg.pickAction(true);
			if (action || dlg._action.empty()) {
				dlg.setFlag(kDlgFlagHiFinished);
				if (action) {
					if (!runOps(action->sceneOpList))
						return true;
				}
			}
			if (dlg._nextDialogNum) {
				dlg.setFlag(kDlgFlagHiFinished);
				showDialog(dlg._nextDialogNum);
			}
		}
	}
	return retval;
}

bool SDSScene::drawActiveDialogBgs(Graphics::Surface *dst) {
	bool retval = false;
	const DgdsEngine *engine = static_cast<const DgdsEngine *>(g_engine);
	for (auto &dlg : _dialogs) {
		if (dlg.hasFlag(kDlgFlagVisible) && !dlg.hasFlag(kDlgFlagOpening)) {
			assert(dlg._state);
			if (dlg._state->_hideTime == 0 && dlg._time > 0) {
				dlg._state->_hideTime = g_engine->getTotalPlayTime() +
							dlg._time * (9 - engine->getTextSpeed());
			}
			dlg.draw(dst, kDlgDrawStageBackground);
			dlg.clearFlag(kDlgFlagHi20);
			dlg.setFlag(kDlgFlagHi40);
			retval = true;
		}
	}
	return retval;
}

bool SDSScene::drawAndUpdateDialogs(Graphics::Surface *dst) {
	bool retval = false;
	const DgdsEngine *engine = static_cast<const DgdsEngine *>(g_engine);
	for (auto &dlg : _dialogs) {
		if (dlg.hasFlag(kDlgFlagLo80) && !dlg.hasFlag(kDlgFlagLo4) &&
				!dlg.hasFlag(kDlgFlagHi20) && !dlg.hasFlag(kDlgFlagHi40)) {
			// TODO: do something with "transfer"s?
			dlg.setFlag(kDlgFlagHi4);
		}
		if (!dlg.hasFlag(kDlgFlagVisible) || (!dlg.hasFlag(kDlgFlagLo4) && !dlg.hasFlag(kDlgFlagHi4) && !dlg.hasFlag(kDlgFlagHi20) && !dlg.hasFlag(kDlgFlagHi40))) {
			if (dlg.hasFlag(kDlgFlagHi8) || dlg.hasFlag(kDlgFlagHi10)) {
				dlg.draw(dst, kDlgDrawStageForeground);
				if (!dlg.hasFlag(kDlgFlagHi8)) {
					dlg.clearFlag(kDlgFlagHi10);
				} else {
					dlg.flipFlag(kDlgFlagHi8);
					dlg.flipFlag(kDlgFlagHi10);
				}
			}
		} else if (!dlg.hasFlag(kDlgFlagOpening)) {
			dlg.draw(dst, kDlgDrawStageBackground);
			if (dlg.hasFlag(kDlgFlagHi20)) {
				// Reset the dialog time and selected action
				int delay = -1;
				if (dlg._time)
					delay = dlg._time;

				int time = delay * (9 - engine->getTextSpeed());
				assert(dlg._state);

				dlg._state->_hideTime = g_engine->getTotalPlayTime() + time;
				dlg._state->_selectedAction = nullptr;
				dlg.updateSelectedAction(0);
				if (dlg._action.size() > 1 && !dlg._state->_selectedAction) {
					dlg._state->_selectedAction = dlg.pickAction(false);
					if (dlg._state->_selectedAction)
						dlg.draw(dst, kDlgDrawStageForeground);
				}
			}

			if (!dlg.hasFlag(kDlgFlagHi20)) {
				dlg.clearFlag(kDlgFlagHi40);
			} else {
				dlg.flipFlag(kDlgFlagHi20);
				dlg.flipFlag(kDlgFlagHi40);
			}
			dlg.clearFlag(kDlgFlagHi4);
			retval = true;
		} else if (!engine->justChangedScene1()) {
			dlg.clearFlag(kDlgFlagOpening);
		}

		if (dlg.hasFlag(kDlgFlagVisible) && !dlg.hasFlag(kDlgFlagLo4) &&
				!dlg.hasFlag(kDlgFlagHi20) && !dlg.hasFlag(kDlgFlagHi40)) {
			// TODO: do something with "transfer"s?
			// warning("SDSScene::drawActiveDrawAndUpdateDialogs: Do something with transfers?");
			dlg.setFlag(kDlgFlagHi4);
		}
	}
	return retval;
}

void SDSScene::globalOps(const Common::Array<uint16> &args) {
	// The globals are held by the GDS scene
	static_cast<DgdsEngine *>(g_engine)->getGDSScene()->globalOps(args);
}

GDSScene::GDSScene() {
}

bool GDSScene::load(const Common::String &filename, ResourceManager *resourceManager, Decompressor *decompressor) {
	Common::SeekableReadStream *sceneFile = resourceManager->getResource(filename);
	if (!sceneFile)
		error("Scene file %s not found", filename.c_str());

	DgdsChunkReader chunk(sceneFile);

	bool result = false;

	while (chunk.readNextHeader(EX_GDS, filename)) {
		if (chunk.isContainer()) {
			continue;
		}

		chunk.readContent(decompressor);
		Common::SeekableReadStream *stream = chunk.getContent();

		if (chunk.isSection(ID_GDS)) {
			// do nothing, this is the container.
			assert(chunk.isContainer());
		} else if (chunk.isSection(ID_INF)) {
			result = parseInf(stream);
		} else if (chunk.isSection(ID_SDS)) {
			result = parse(stream);
		}
	}

	delete sceneFile;

	return result;
}

bool GDSScene::readPerSceneGlobals(Common::SeekableReadStream *s) {
	_perSceneGlobals.resize(s->readUint16LE());
	for (PerSceneGlobal &dst : _perSceneGlobals) {
		dst._num = s->readUint16LE();
		dst._sceneNo = s->readUint16LE();
		dst._val = s->readUint16LE();
	}
	return !s->err();
}

bool GDSScene::parseInf(Common::SeekableReadStream *s) {
	_magic = s->readUint32LE();
	_version = s->readString();
	return !s->err();
}

bool GDSScene::parse(Common::SeekableReadStream *stream) {
	readOpList(stream, _startGameOps);
	readOpList(stream, _quitGameOps);
	if (isVersionOver(" 1.206"))
		readOpList(stream, _preTickOps);
	readOpList(stream, _postTickOps);
	if (isVersionOver(" 1.208"))
		readOpList(stream, _onChangeSceneOps);
	readPerSceneGlobals(stream);
	Common::Array<struct MouseCursor> cursorList;
	_iconFile = stream->readString();
	readMouseHotspotList(stream, cursorList);
	readGameItemList(stream, _gameItems);
	readStruct4List(stream, _struct4List2);
	if (isVersionOver(" 1.205"))
		readStruct4List(stream, _struct4List1);

	return !stream->err();
}

Common::String GDSScene::dump(const Common::String &indent) const {
	Common::String str = Common::String::format("%sGDSScene<icons %s", indent.c_str(), _iconFile.c_str());
	str += _dumpStructList(indent, "gameItems", _gameItems);
	str += _dumpStructList(indent, "startGameOps", _startGameOps);
	str += _dumpStructList(indent, "quitGameOps", _quitGameOps);
	str += _dumpStructList(indent, "preTickOps", _preTickOps);
	str += _dumpStructList(indent, "postTickOps", _postTickOps);
	str += _dumpStructList(indent, "onChangeSceneOps", _onChangeSceneOps);
	str += _dumpStructList(indent, "perSceneGlobals", _perSceneGlobals);
	str += _dumpStructList(indent, "struct4List1", _struct4List1);
	str += _dumpStructList(indent, "struct4List2", _struct4List2);

	str += "\n";
	str += indent + ">";
	return str;
}

void GDSScene::globalOps(const Common::Array<uint16> &args) {
	for (uint i = 0; i < args.size() / 3; i++) {
		uint16 num = args[i * 3 + 0];
		uint16 op  = args[i * 3 + 1];
		uint16 val = args[i * 3 + 2];

		// CHECK ME: The original uses a different function here, but the
		// result appears to be the same as just calling getGlobal?
		num = getGlobal(num);

		// Op bit 3 on means use absolute val of val.
		// Off means val is another global to lookup
		if (op & 8)
			op = op & 0xfff7;
        else
			val = getGlobal(val);

        if (op == 1)
			val = num + val;
		else if (op == 6)
			val = (val == 0);
		else if (op == 5)
			val = num - val;

		setGlobal(num, val);
	}
}

uint16 GDSScene::getGlobal(uint16 num) {
	DgdsEngine *engine = static_cast<DgdsEngine *>(g_engine);
	int curSceneNum = engine->getScene()->getNum();
	for (const auto &global : _perSceneGlobals) {
		if (global._num == num && (global._sceneNo == 0 || global._sceneNo == curSceneNum))
			return global._val;
	}
	Globals *gameGlobals = engine->getGameGlobals();
	return gameGlobals->getGlobal(num);
}

uint16 GDSScene::setGlobal(uint16 num, uint16 val) {
	DgdsEngine *engine = static_cast<DgdsEngine *>(g_engine);
	int curSceneNum = engine->getScene()->getNum();
	for (auto &global : _perSceneGlobals) {
		if (global._num == num && (global._sceneNo == 0 || global._sceneNo == curSceneNum)) {
			global._val = val;
			return val;
		}
	}
	Globals *gameGlobals = engine->getGameGlobals();
	return gameGlobals->setGlobal(num, val);
}

} // End of namespace Dgds

