## Process this file with automake to produce Makefile.in

SUBDIRS = images

scriptdatadir = $(gimpdatadir)/scripts

scripts = \
	script-fu.init			\
	script-fu-compat.init		\
	plug-in-compat.init		\
	add-bevel.scm			\
	addborder.scm			\
	blend-anim.scm			\
	burn-in-anim.scm		\
	carve-it.scm			\
	chrome-it.scm			\
	circuit.scm			\
	clothify.scm			\
	coffee.scm			\
	copy-visible.scm		\
	difference-clouds.scm		\
	distress-selection.scm		\
	drop-shadow.scm			\
	erase-rows.scm			\
	font-map.scm			\
	fuzzyborder.scm			\
	gimp-online.scm			\
	gradient-example.scm		\
	grid-system.scm			\
	guides-from-selection.scm	\
	guides-new.scm			\
	guides-new-percent.scm		\
	guides-remove-all.scm		\
	lava.scm			\
	line-nova.scm			\
	mkbrush.scm			\
	old-photo.scm			\
	palette-export.scm		\
	paste-as-brush.scm		\
	paste-as-pattern.scm		\
	perspective-shadow.scm		\
	predator.scm			\
	reverse-layers.scm		\
	ripply-anim.scm			\
	round-corners.scm \
	script-fu-set-cmap.scm		\
	script-fu-util.scm		\
	select-to-brush.scm		\
	select-to-image.scm		\
	select-to-pattern.scm		\
	selection-round.scm		\
	slide.scm			\
	spinning-globe.scm		\
	spyrogimp.scm			\
	tileblur.scm			\
	unsharp-mask.scm		\
	waves-anim.scm			\
	weave.scm			\
	xach-effect.scm

test_scripts = \
	contactsheet.scm		\
	test-sphere.scm			\
	ts-helloworld.scm


scriptdata_DATA = $(scripts)

if GIMP_UNSTABLE
scriptdata_DATA += $(test_scripts)
endif

EXTRA_DIST = $(scripts) $(test_scripts)
