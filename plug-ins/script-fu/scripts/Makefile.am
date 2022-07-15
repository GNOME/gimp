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
	tileblur.scm			\
	unsharp-mask.scm		\
	waves-anim.scm			\
	weave.scm			\
	xach-effect.scm

# FIXME: when 3.0 ships, ship only the v3 versions
# of clothify.scm and test-sphere.scm
# During dev of 2.99, install old and new so devs can compare their GUIs.
test_scripts = \
	contactsheet.scm   \
	test-sphere.scm    \
	clothify-v3.scm


# scripts interpreted by gimp-script-fu-interpreter
# Each installs to a subdir of /plug-ins
# Each should have a shebang and execute permission
independent_scripts = \
	ts-helloworld.scm   \
	test-sphere-v3.scm

ts_helloworlddir = $(gimpplugindir)/plug-ins/ts-helloworld
ts_helloworld_SCRIPTS = ts-helloworld.scm
test_sphere_v3dir = $(gimpplugindir)/plug-ins/test-sphere-v3
test_sphere_v3_SCRIPTS = test-sphere-v3.scm


scriptdata_DATA = $(scripts)

if GIMP_UNSTABLE
scriptdata_DATA += $(test_scripts)
endif

EXTRA_DIST = $(scripts) $(test_scripts) $(independent_scripts)
