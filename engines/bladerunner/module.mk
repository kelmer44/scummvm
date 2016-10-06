MODULE := engines/bladerunner

MODULE_OBJS = \
	adq.o \
	actor.o \
	actor_clues.o \
	actor_combat.o \
	actor_walk.o \
	adpcm_decoder.o \
	ambient_sounds.o \
	archive.o \
	aud_stream.o \
	audio_player.o \
	audio_speech.o \
	bladerunner.o \
	boundingbox.o \
	chapters.o \
	combat.o \
	crimes_database.o \
	decompress_lcw.o \
	decompress_lzo.o \
	detection.o \
	fog.o \
	font.o \
	gameflags.o \
	gameinfo.o \
	image.o \
	item.o \
	items.o \
	light.o \
	lights.o \
	matrix.o \
	mouse.o \
	movement_track.o \
	obstacles.o \
	outtake.o \
	regions.o \
	scene.o \
	scene_objects.o \
	script/ai_00_mccoy.o \
	script/aiscript_officer_leroy.o \
	script/init.o \
	script/kia.o \
	script/vk.o \
	script/esper.o \
	script/ar01.o \
	script/ar02.o \
	script/bb01.o \
	script/bb02.o \
	script/bb03.o \
	script/bb04.o \
	script/bb05.o \
	script/bb06.o \
	script/bb07.o \
	script/bb08.o \
	script/bb09.o \
	script/bb10.o \
	script/bb11.o \
	script/bb12.o \
	script/bb51.o \
	script/ct01.o \
	script/ct02.o \
	script/ct03.o \
	script/ct04.o \
	script/ct05.o \
	script/ct06.o \
	script/ct07.o \
	script/ct08.o \
	script/ct09.o \
	script/ct10.o \
	script/ct11.o \
	script/ct12.o \
	script/ct51.o \
	script/dr01.o \
	script/dr02.o \
	script/dr03.o \
	script/dr04.o \
	script/dr05.o \
	script/dr06.o \
	script/hc01.o \
	script/hc02.o \
	script/hc03.o \
	script/hc04.o \
	script/hf01.o \
	script/hf02.o \
	script/hf03.o \
	script/hf04.o \
	script/hf05.o \
	script/hf06.o \
	script/kp01.o \
	script/kp02.o \
	script/kp03.o \
	script/kp04.o \
	script/kp05.o \
	script/kp06.o \
	script/kp07.o \
	script/ma01.o \
	script/ma02.o \
	script/ma04.o \
	script/ma05.o \
	script/ma06.o \
	script/ma07.o \
	script/ma08.o \
	script/nr01.o \
	script/nr02.o \
	script/nr03.o \
	script/nr04.o \
	script/nr05.o \
	script/nr06.o \
	script/nr07.o \
	script/nr08.o \
	script/nr09.o \
	script/nr10.o \
	script/nr11.o \
	script/ps01.o \
	script/ps02.o \
	script/ps03.o \
	script/ps04.o \
	script/ps05.o \
	script/ps06.o \
	script/ps07.o \
	script/ps09.o \
	script/ps10.o \
	script/ps11.o \
	script/ps12.o \
	script/ps13.o \
	script/ps14.o \
	script/ps15.o \
	script/rc01.o \
	script/rc02.o \
	script/rc03.o \
	script/rc04.o \
	script/rc51.o \
	script/tb02.o \
	script/tb03.o \
	script/tb05.o \
	script/tb06.o \
	script/tb07.o \
	script/ug01.o \
	script/ug02.o \
	script/ug03.o \
	script/ug04.o \
	script/ug05.o \
	script/ug06.o \
	script/ug07.o \
	script/ug08.o \
	script/ug09.o \
	script/ug10.o \
	script/ug12.o \
	script/ug13.o \
	script/ug14.o \
	script/ug15.o \
	script/ug16.o \
	script/ug17.o \
	script/ug18.o \
	script/ug19.o \
	script/script.o \
	set.o \
	settings.o \
	set_effects.o \
	shape.o \
	slice_animations.o \
	slice_renderer.o \
	suspects_database.o \
	text_resource.o \
	view.o \
	vqa_decoder.o \
	vqa_player.o \
	waypoints.o

# This module can be built as a plugin
ifeq ($(ENABLE_BLADERUNNER), DYNAMIC_PLUGIN)
PLUGIN := 1
endif

# Include common rules
include $(srcdir)/rules.mk
