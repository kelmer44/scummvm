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

#ifndef MTROPOLIS_PLUGIN_STANDARD_H
#define MTROPOLIS_PLUGIN_STANDARD_H

#include "mtropolis/modifiers.h"
#include "mtropolis/modifier_factory.h"
#include "mtropolis/runtime.h"
#include "mtropolis/plugin/standard_data.h"

namespace MTropolis {

namespace Standard {

class StandardPlugIn;

class CursorModifier : public Modifier {
public:
	bool load(const PlugInModifierLoaderContext &context, const Data::Standard::CursorModifier &data);

private:
	Event _applyWhen;
	Event _removeWhen;
	uint32 _cursorID;
};

// Some sort of scene transition modifier
class STransCtModifier : public Modifier {
public:
	bool load(const PlugInModifierLoaderContext &context, const Data::Standard::STransCtModifier &data);
};

class MediaCueMessengerModifier : public Modifier {
public:
	bool load(const PlugInModifierLoaderContext &context, const Data::Standard::MediaCueMessengerModifier &data);

private:
	enum TriggerTiming {
		kTriggerTimingStart = 0,
		kTriggerTimingDuring = 1,
		kTriggerTimingEnd = 2,
	};

	Event _enableWhen;
	Event _disableWhen;
	TriggerTiming _triggerTiming;
	MessengerSendSpec _send;
};

class ObjectReferenceVariableModifier : public Modifier {
public:
	bool load(const PlugInModifierLoaderContext &context, const Data::Standard::ObjectReferenceVariableModifier &data);

private:
	Event _setToSourceParentWhen;
	Common::String _objectPath;
};

class MidiModifier : public Modifier {
public:
	bool load(const PlugInModifierLoaderContext &context, const Data::Standard::MidiModifier &data);

private:
	struct FilePart {
		bool loop;
		bool overrideTempo;
		bool volume;
		double tempo;
		double fadeIn;
		double fadeOut;
	};

	struct SingleNotePart {
		uint8 channel;
		uint8 note;
		uint8 velocity;
		uint8 program;
		double duration;
	};

	union ModeSpecificUnion {
		FilePart file;
		SingleNotePart singleNote;
	};

	enum Mode {
		kModeFile,
		kModeSingleNote,
	};

	Event _executeWhen;
	Event _terminateWhen;

	Mode _mode;
	ModeSpecificUnion _modeSpecific;

	Common::SharedPtr<Data::Standard::MidiModifier::EmbeddedFile> _embeddedFile;
};

class StandardPlugIn : public MTropolis::PlugIn {
public:
	StandardPlugIn();

	void registerModifiers(IPlugInModifierRegistrar *registrar) const override;

private:
	PlugInModifierFactory<CursorModifier, Data::Standard::CursorModifier> _cursorModifierFactory;
	PlugInModifierFactory<STransCtModifier, Data::Standard::STransCtModifier> _sTransCtModifierFactory;
	PlugInModifierFactory<MediaCueMessengerModifier, Data::Standard::MediaCueMessengerModifier> _mediaCueModifierFactory;
	PlugInModifierFactory<ObjectReferenceVariableModifier, Data::Standard::ObjectReferenceVariableModifier> _objRefVarModifierFactory;
	PlugInModifierFactory<MidiModifier, Data::Standard::MidiModifier> _midiModifierFactory;
};

} // End of namespace Standard

} // End of namespace MTropolis

#endif
