## Process this file with automake to produce Makefile.in

SUBDIRS = layers channels paths palette_editor gradient_editor display_filters color_selectors preferences 

helpdatadir = $(gimpdatadir)/help/C/dialogs

helpdata_DATA = \
        about.html \
        border_selection.html \
        brush_selection.html \
        convert_to_indexed.html \
        copy_named.html \
        cut_named.html \
        device_status.html \
        document_index.html \
        edit_qmask_attributes.html \
        error_console.html \
        feather_selection.html \
        file_new.html \
        file_open.html \
        file_save.html \
        gradient_selection.html \
        grow_selection.html \
        help.html \
        index.html \
        indexed_palette.html \
        info_window.html \
        input_devices.html \
        layers_and_channels.html \
        module_browser.html \
        navigation_window.html \
        offset.html \
        palette_selection.html \
        paste_named.html \
        pattern_selection.html \
        really_close.html \
        really_quit.html \
        resize_image.html \
        scale_image.html \
        shrink_selection.html \
        tip_of_the_day.html \
        tool_options.html \
        undo_history.html 

EXTRA_DIST = $(helpdata_DATA)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
	@for subdir in $(SUBDIRS); do \
	  files=`cd $$subdir; $(MAKE) files | grep -v "make\[[1-9]\]"`; \
	  for file in $$files; do \
	    echo $$subdir/$$file; \
	  done; \
	done
