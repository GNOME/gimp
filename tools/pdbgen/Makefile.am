enum_headers = \
	../../app/convert.h		\
	../../app/channel_ops.h		\
	../../app/text_tool.h		\
	../../app/gimpdrawable.h	\
	../../app/gimpimage.h

enums.pl: enumgen.pl $(enum_headers)
	cd $(srcdir) \
	&& $(PERL) enumgen.pl $(enum_headers)
