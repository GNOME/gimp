isl_in_files = \
	setup.isl.in
isl_files = $(isl_in_files:.isl.in=.isl)

stamp_files = \
	stamp-isl

CLEANFILES = $(isl_files) $(stamp_files)

languages = \
	ca:[ca]:WINDOWS-1252		\
	cs:[cs]:WINDOWS-1250		\
	da:[da]:WINDOWS-1252		\
	de:[de]:WINDOWS-1252		\
	el:[el]:WINDOWS-1253		\
	en::WINDOWS-1252		\
	eo:[eo]:ISO-8859-3		\
	es:[es]:WINDOWS-1252		\
	eu:[eu]:WINDOWS-1252		\
	fi:[fi]:WINDOWS-1252		\
	fr:[fr]:WINDOWS-1252		\
	hu:[hu]:WINDOWS-1250		\
	id:[id]:ISO-8859-1		\
	is:[is]:WINDOWS-1252		\
	it:[it]:WINDOWS-1252		\
	ko:[ko]:EUC-KR			\
	lv:[lv]:ISO-8859-13		\
	nl:[nl]:WINDOWS-1252		\
	pl:[pl]:WINDOWS-1250		\
	pt_BR:[pt_BR]:WINDOWS-1252	\
	ru:[ru]:WINDOWS-1251		\
	sl:[sl]:WINDOWS-1250		\
	sv:[sv]:WINDOWS-1252		\
	tr:[tr]:WINDOWS-1254		\
	zh_CN:[zh_CN]:WINDOWS-936	\
	zh_TW:[zh_TW]:CP950

stamp-isl: setup.isl Makefile.am
	@										\
	for lang in $(languages); do							\
		code=`echo $$lang | cut -d: -f1`;					\
		prefix=`echo $$lang | cut -d: -f2 | sed 's/[][]/\\\\\\0/g'`;		\
		encoding=`echo $$lang | cut -d: -f3`;					\
		isl="$(srcdir)/$$code.setup.isl";					\
		echo "  GEN      $$isl";						\
		sed '/^\w\+'"$$prefix"'=/{s/\(.\)'"$$prefix"'/\1/;n};/^\w.*=/d' $< |	\
		iconv -f UTF-8 -t $$encoding > "$$isl";					\
	done &&										\
	echo timestamp > stamp-isl

setup.isl: setup.isl.in $(wildcard $(top_srcdir)/po-windows-installer/*.po) Makefile.am
	$(AM_V_GEN) $(INTLTOOL_MERGE) $(top_srcdir)/po-windows-installer $< $(@) -d -u -c $(top_builddir)/po-windows-installer/.intltool-merge-cache

all-local: $(stamp_files)

clean-local:
	@						\
	for lang in $(languages); do			\
		code=`echo $$lang | cut -d: -f1`;	\
		isl="$(srcdir)/$$code.setup.isl";	\
		echo "  RM       $$isl";		\
		rm -f "$$isl";				\
	done
