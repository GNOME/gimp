#!/usr/bin/env python

from gimpfu import *

have_gimp11 = gimp.major_version > 1 or gimp.major_version == 1 and \
	      gimp.minor_version >= 1

def shadow_bevel(img, drawable, blur, bevel, do_shadow, drop_x, drop_y):
	# disable undo for the image
	img.disable_undo()

	# copy the layer
	shadow = drawable.copy(TRUE)
	img.add_layer(shadow, img.layers.index(drawable)+1)
	shadow.name = drawable.name + " shadow"
	shadow.preserve_transparency = FALSE

	# threshold the shadow layer to all white
	if have_gimp11:
		pdb.gimp_threshold(shadow, 0, 255)
	else:
		pdb.gimp_threshold(img, shadow, 0, 255)

	# blur the shadow layer
	pdb.plug_in_gauss_iir(img, shadow, blur, TRUE, TRUE)

	# do the bevel thing ...
	if bevel:
		pdb.plug_in_bump_map(img, drawable, shadow, 135, 45, 3,
				     0, 0, 0, 0, TRUE, FALSE, 0)

	# make the shadow layer black now ...
	if have_gimp11:
		pdb.gimp_invert(shadow)
	else:
		pdb.gimp_invert(img, shadow)

	# translate the drop shadow
	shadow.translate(drop_x, drop_y)

	if not do_shadow:
		# delete shadow ...
		gimp.delete(shadow)

	# enable undo again
	img.enable_undo()

register(
	"shadow_bevel",
	"Add a drop shadow to a layer, and optionally bevel it.",
	"Add a drop shadow to a layer, and optionally bevel it.",
	"James Henstridge",
	"James Henstridge",
	"1999",
	"<Image>/Python-Fu/Effects/Drop Shadow and Bevel",
	"RGBA, GRAYA",
	[
		(PF_SLIDER, "blur",   "Shadow Blur", 6, (1, 30, 1)),
		(PF_BOOL,   "bevel",  "Bevel the image", TRUE),
		(PF_BOOL,   "shadow", "Make a drop shadow", TRUE),
		(PF_INT,    "drop_x", "Drop shadow X displacement", 3),
		(PF_INT,    "drop_y", "Drop shadow Y displacement", 6)
	],
	[],
	shadow_bevel)

main()

