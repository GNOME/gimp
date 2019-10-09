# Introspectable sources for libgimp and libgimpui

PDB_WRAPPERS_C = \
	../libgimp/gimp_pdb.c			\
	../libgimp/gimpbrush_pdb.c		\
	../libgimp/gimpbrushes_pdb.c		\
	../libgimp/gimpbrushselect_pdb.c	\
	../libgimp/gimpbuffer_pdb.c		\
	../libgimp/gimpchannel_pdb.c		\
	../libgimp/gimpcontext_pdb.c		\
	../libgimp/gimpdebug_pdb.c		\
	../libgimp/gimpdisplay_pdb.c		\
	../libgimp/gimpdrawable_pdb.c		\
	../libgimp/gimpdrawablecolor_pdb.c	\
	../libgimp/gimpdrawableedit_pdb.c	\
	../libgimp/gimpdynamics_pdb.c		\
	../libgimp/gimpedit_pdb.c		\
	../libgimp/gimpfile_pdb.c		\
	../libgimp/gimpfloatingsel_pdb.c	\
	../libgimp/gimpfonts_pdb.c		\
	../libgimp/gimpfontselect_pdb.c		\
	../libgimp/gimpgimprc_pdb.c		\
	../libgimp/gimpgradient_pdb.c		\
	../libgimp/gimpgradients_pdb.c		\
	../libgimp/gimpgradientselect_pdb.c	\
	../libgimp/gimphelp_pdb.c		\
	../libgimp/gimpimage_pdb.c		\
	../libgimp/gimpimagecolorprofile_pdb.c	\
	../libgimp/gimpimageconvert_pdb.c	\
	../libgimp/gimpimagegrid_pdb.c		\
	../libgimp/gimpimageguides_pdb.c	\
	../libgimp/gimpimagesamplepoints_pdb.c	\
	../libgimp/gimpimageselect_pdb.c	\
	../libgimp/gimpimagetransform_pdb.c	\
	../libgimp/gimpimageundo_pdb.c		\
	../libgimp/gimpitem_pdb.c		\
	../libgimp/gimpitemtransform_pdb.c	\
	../libgimp/gimplayer_pdb.c		\
	../libgimp/gimpmessage_pdb.c		\
	../libgimp/gimppainttools_pdb.c		\
	../libgimp/gimppalette_pdb.c		\
	../libgimp/gimppalettes_pdb.c		\
	../libgimp/gimppaletteselect_pdb.c	\
	../libgimp/gimppattern_pdb.c		\
	../libgimp/gimppatterns_pdb.c		\
	../libgimp/gimppatternselect_pdb.c	\
	../libgimp/gimpprogress_pdb.c		\
	../libgimp/gimpselection_pdb.c		\
	../libgimp/gimptextlayer_pdb.c		\
	../libgimp/gimptexttool_pdb.c		\
	../libgimp/gimpvectors_pdb.c

PDB_WRAPPERS_H = \
	../libgimp/gimp_pdb_headers.h		\
	../libgimp/gimp_pdb.h			\
	../libgimp/gimpbrush_pdb.h		\
	../libgimp/gimpbrushes_pdb.h		\
	../libgimp/gimpbrushselect_pdb.h	\
	../libgimp/gimpbuffer_pdb.h		\
	../libgimp/gimpchannel_pdb.h		\
	../libgimp/gimpcontext_pdb.h		\
	../libgimp/gimpdebug_pdb.h		\
	../libgimp/gimpdisplay_pdb.h		\
	../libgimp/gimpdrawable_pdb.h		\
	../libgimp/gimpdrawablecolor_pdb.h	\
	../libgimp/gimpdrawableedit_pdb.h	\
	../libgimp/gimpdynamics_pdb.h		\
	../libgimp/gimpedit_pdb.h		\
	../libgimp/gimpfile_pdb.h		\
	../libgimp/gimpfloatingsel_pdb.h	\
	../libgimp/gimpfonts_pdb.h		\
	../libgimp/gimpfontselect_pdb.h		\
	../libgimp/gimpgimprc_pdb.h		\
	../libgimp/gimpgradient_pdb.h		\
	../libgimp/gimpgradients_pdb.h		\
	../libgimp/gimpgradientselect_pdb.h	\
	../libgimp/gimphelp_pdb.h		\
	../libgimp/gimpimage_pdb.h		\
	../libgimp/gimpimagecolorprofile_pdb.h	\
	../libgimp/gimpimageconvert_pdb.h	\
	../libgimp/gimpimagegrid_pdb.h		\
	../libgimp/gimpimageguides_pdb.h	\
	../libgimp/gimpimagesamplepoints_pdb.h	\
	../libgimp/gimpimageselect_pdb.h	\
	../libgimp/gimpimagetransform_pdb.h	\
	../libgimp/gimpimageundo_pdb.h		\
	../libgimp/gimpitem_pdb.h		\
	../libgimp/gimpitemtransform_pdb.h	\
	../libgimp/gimplayer_pdb.h		\
	../libgimp/gimpmessage_pdb.h		\
	../libgimp/gimppainttools_pdb.h		\
	../libgimp/gimppalette_pdb.h		\
	../libgimp/gimppalettes_pdb.h		\
	../libgimp/gimppaletteselect_pdb.h	\
	../libgimp/gimppattern_pdb.h		\
	../libgimp/gimppatterns_pdb.h		\
	../libgimp/gimppatternselect_pdb.h	\
	../libgimp/gimpprogress_pdb.h		\
	../libgimp/gimpselection_pdb.h		\
	../libgimp/gimptextlayer_pdb.h		\
	../libgimp/gimptexttool_pdb.h		\
	../libgimp/gimpvectors_pdb.h

libgimp_built_sources = \
	gimpenums.c

libgimp_introspectable_headers = \
	../libgimp/gimp.h			\
	../libgimp/gimptypes.h			\
	../libgimp/gimpenums.h			\
	${PDB_WRAPPERS_H}			\
	../libgimp/gimpbrushselect.h		\
	../libgimp/gimpchannel.h		\
	../libgimp/gimpdisplay.h		\
	../libgimp/gimpdrawable.h		\
	../libgimp/gimpfileprocedure.h		\
	../libgimp/gimpfontselect.h		\
	../libgimp/gimpgimprc.h			\
	../libgimp/gimpgradientselect.h		\
	../libgimp/gimpimage.h			\
	../libgimp/gimpimagecolorprofile.h	\
	../libgimp/gimpimageprocedure.h		\
	../libgimp/gimpitem.h			\
	../libgimp/gimplayer.h			\
	../libgimp/gimplayermask.h		\
	../libgimp/gimploadprocedure.h		\
	../libgimp/gimppaletteselect.h		\
	../libgimp/gimpparamspecs.h		\
	../libgimp/gimppatternselect.h		\
	../libgimp/gimppdb.h			\
	../libgimp/gimpplugin.h			\
	../libgimp/gimpprocedure.h		\
	../libgimp/gimpprocedureconfig.h	\
	../libgimp/gimpprogress.h		\
	../libgimp/gimpsaveprocedure.h		\
	../libgimp/gimpselection.h		\
	../libgimp/gimpthumbnailprocedure.h	\
	../libgimp/gimpvectors.h

libgimp_introspectable = \
	$(libgimp_introspectable_headers)	\
	$(libgimp_built_sources)		\
	../libgimp/gimp.c			\
	${PDB_WRAPPERS_C}			\
	../libgimp/gimpbrushselect.c		\
	../libgimp/gimpchannel.c		\
	../libgimp/gimpdisplay.c		\
	../libgimp/gimpdrawable.c		\
	../libgimp/gimpfileprocedure.c		\
	../libgimp/gimpfontselect.c		\
	../libgimp/gimpgimprc.c			\
	../libgimp/gimpgradientselect.c		\
	../libgimp/gimpimage.c			\
	../libgimp/gimpimagecolorprofile.c	\
	../libgimp/gimpimagemetadata-save.c	\
	../libgimp/gimpimageprocedure.c		\
	../libgimp/gimpitem.c			\
	../libgimp/gimplayer.c			\
	../libgimp/gimplayermask.c		\
	../libgimp/gimploadprocedure.c		\
	../libgimp/gimppaletteselect.c		\
	../libgimp/gimpparamspecs.c		\
	../libgimp/gimppatternselect.c		\
	../libgimp/gimppdb.c			\
	../libgimp/gimpplugin.c			\
	../libgimp/gimpprocedure.c		\
	../libgimp/gimpprocedureconfig.c	\
	../libgimp/gimpprogress.c		\
	../libgimp/gimpsaveprocedure.c		\
	../libgimp/gimpselection.c		\
	../libgimp/gimpthumbnailprocedure.c	\
	../libgimp/gimpvectors.c

libgimpui_introspectable_headers = \
	../libgimp/gimpui.h			\
	../libgimp/gimpuitypes.h		\
	../libgimp/gimpaspectpreview.h		\
	../libgimp/gimpbrushselectbutton.h	\
	../libgimp/gimpdrawablepreview.h   	\
	../libgimp/gimpexport.h			\
	../libgimp/gimpfontselectbutton.h	\
	../libgimp/gimpgradientselectbutton.h	\
	../libgimp/gimpimagecombobox.h		\
	../libgimp/gimpimagemetadata.h		\
	../libgimp/gimpitemcombobox.h		\
	../libgimp/gimppaletteselectbutton.h	\
	../libgimp/gimppatternselectbutton.h	\
	../libgimp/gimpprocbrowserdialog.h	\
	../libgimp/gimpproceduredialog.h	\
	../libgimp/gimpprocview.h		\
	../libgimp/gimpprogressbar.h		\
	../libgimp/gimpselectbutton.h		\
	../libgimp/gimpzoompreview.h

libgimpui_introspectable = \
	$(libgimpui_introspectable_headers)	\
	../libgimp/gimpui.c			\
	../libgimp/gimpaspectpreview.c     	\
	../libgimp/gimpbrushselectbutton.c	\
	../libgimp/gimpdrawablepreview.c	\
	../libgimp/gimpexport.c			\
	../libgimp/gimpfontselectbutton.c	\
	../libgimp/gimpgradientselectbutton.c	\
	../libgimp/gimpimagecombobox.c		\
	../libgimp/gimpimagemetadata.c		\
	../libgimp/gimpitemcombobox.c		\
	../libgimp/gimppaletteselectbutton.c	\
	../libgimp/gimppatternselectbutton.c	\
	../libgimp/gimpprocbrowserdialog.c	\
	../libgimp/gimpproceduredialog.c	\
	../libgimp/gimpprocview.c		\
	../libgimp/gimpprogressbar.c		\
	../libgimp/gimpselectbutton.c		\
	../libgimp/gimpzoompreview.c
