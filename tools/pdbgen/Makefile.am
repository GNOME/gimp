enum_headers = \
	../../app/convert.h		\
	../../app/channel_ops.h		\
	../../app/text_tool.h		\
	../../app/gimpdrawable.h	\
	../../app/gimpimage.h		\
	../../app/procedural_db.h	\
	../../app/paint_funcs.h		\
	../../app/blend.h		\
	../../app/bucket_fill.h		\
	../../app/clone.h		\
	../../app/convolve.h		\
	../../app/channel.h		\
	../../app/eraser.h		\
	../../app/paint_core.h		\
	../../app/lut_funcs.h		\
	../../app/shear_tool.h		\
	../../app/color_balance.h	\
	../../app/hue_saturation.h

enums.pl: enumgen.pl $(enum_headers)
	cd $(srcdir) \
	&& $(PERL) enumgen.pl $(enum_headers)
