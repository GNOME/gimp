isl_in_files = \
	setup.isl.in
isl_files = $(isl_in_files:.isl.in=.isl)

stamp_files = \
	stamp-isl

CLEANFILES = $(isl_files) $(stamp_files)

languages = \
	ca:[ca]		\
	cs:[cs]		\
	da:[da]		\
	de:[de]		\
	el:[el]		\
	en:		\
	en_GB:[en_GB]	\
	eo:[eo]		\
	es:[es]		\
	eu:[eu]		\
	fi:[fi]		\
	fr:[fr]		\
	he:[he]		\
	hu:[hu]		\
	id:[id]		\
	is:[is]		\
	it:[it]		\
	ja:[ja]		\
	kab:[kab]	\
	ko:[ko]		\
	lt:[lt]		\
	lv:[lv]		\
	mr:[mr]   	\
	ms:[ms]   	\
	nl:[nl]		\
	pl:[pl]		\
	pt:[pt]		\
	pt_BR:[pt_BR]	\
	ro:[ro]		\
	ru:[ru]		\
	sk:[sk]		\
	sl:[sl]		\
	sv:[sv]		\
	tr:[tr]		\
	uk:[uk]		\
	vi:[vi]		\
	zh_CN:[zh_CN]	\
	zh_TW:[zh_TW]

stamp-isl: setup.isl Makefile.am
	@										\
	for lang in $(languages); do							\
		code=`echo $$lang | cut -d: -f1`;					\
		prefix=`echo $$lang | cut -d: -f2 | sed 's/[][]/\\\\\\0/g'`;		\
		isl="$(builddir)/$$code.setup.isl";					\
		echo "  GEN      $$isl";						\
		echo -ne "\\xEF\\xBB\\xBF" > "$$isl";					\
		sed '/^\w\+'"$$prefix"'=/{s/\(.\)'"$$prefix"'/\1/;n};/^\w.*=/d' $<  >> "$$isl";	\
	done &&										\
	echo timestamp > stamp-isl

setup.isl: setup.isl.in $(wildcard $(top_srcdir)/po-windows-installer/*.po) Makefile.am
	$(AM_V_GEN) $(INTLTOOL_MERGE) $(top_srcdir)/po-windows-installer $< $(@) -d -u -c $(top_builddir)/po-windows-installer/.intltool-merge-cache

all-local: $(stamp_files)

clean-local:
	@						\
	for lang in $(languages); do			\
		code=`echo $$lang | cut -d: -f1`;	\
		isl="$(builddir)/$$code.setup.isl";	\
		echo "  RM       $$isl";		\
		rm -f "$$isl";				\
	done

TESTS_ENVIRONMENT = \
	GIMP_TESTING_ABS_TOP_SRCDIR=@abs_top_srcdir@

TESTS = test-installer-langs.sh
