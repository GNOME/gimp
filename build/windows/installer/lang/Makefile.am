isl_xml_in_files = setup.isl.xml.in
isl_xml_files = $(isl_xml_in_files:.isl.xml.in=.isl.xml)

stamp_files = \
	stamp-isl

CLEANFILES = $(isl_xml_files) $(stamp_files)
EXTRA_DIST = $(isl_xml_in_files)

# First we use the XML file to be able to do gettext translation
$(isl_xml_files): $(isl_xml_in_files) $(wildcard $(top_srcdir)/po-windows-installer/*.po) Makefile.am
	$(AM_V_GEN) GETTEXTDATADIRS=$(abs_srcdir) $(MSGFMT) --xml --template $< -d $(top_srcdir)/po-windows-installer -o $@

# Then, we generate the .isl file for each language using some xsltproc magic
languages = \
	bg:bg		\
	ca:ca		\
	cs:cs		\
	da:da		\
	de:de		\
	el:el		\
	en:		\
	en_GB:en_GB	\
	eo:eo		\
	es:es		\
	eu:eu		\
	fi:fi		\
	fr:fr		\
	gl:gl		\
	he:he		\
	hu:hu		\
	id:id		\
	is:is		\
	it:it		\
	ja:ja		\
	ka:ka		\
	kab:kab	\
	ko:ko		\
	lt:lt		\
	lv:lv		\
	mr:mr   	\
	ms:ms   	\
	nl:nl		\
	pl:pl		\
	pt:pt		\
	pt_BR:pt_BR	\
	ro:ro		\
	ru:ru		\
	sk:sk		\
	sl:sl		\
	sv:sv		\
	tr:tr		\
	uk:uk		\
	vi:vi		\
	zh_CN:zh_CN	\
	zh_TW:zh_TW

stamp-isl: setup.isl.xml gimp-ms-installer-config.xsl.in Makefile.am
	@										\
	for lang in $(languages); do							\
		code=`echo $$lang | cut -d: -f1`;					\
		prefix=`echo $$lang | cut -d: -f2`;		\
		lang_check=`[ "$$prefix" ] && echo "value[lang('$$code')]" || echo "value"`;	\
		xslin="$(srcdir)/gimp-ms-installer-config.xsl.in";				\
		xsl="$(builddir)/gimp-ms-installer-config.xsl";					\
		isl="$(builddir)/$$code.setup.isl";					\
		echo "  GEN      $$isl";						\
		sed "s/@LANG_CHECK@/$$lang_check/" "$$xslin"  > "$$xsl";	\
		$(XSLTPROC) --xinclude --output "$$isl" "$$xsl" $< || rm -f "$$isl"; \
	done &&										\
	echo timestamp > stamp-isl

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
