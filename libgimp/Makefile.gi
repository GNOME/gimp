# Introspectable sources for libligma and libligmaui

PDB_WRAPPERS_C = \
	../libligma/ligma_pdb.c			\
	../libligma/ligmabrush_pdb.c		\
	../libligma/ligmabrushes_pdb.c		\
	../libligma/ligmabrushselect_pdb.c	\
	../libligma/ligmabuffer_pdb.c		\
	../libligma/ligmachannel_pdb.c		\
	../libligma/ligmacontext_pdb.c		\
	../libligma/ligmadebug_pdb.c		\
	../libligma/ligmadisplay_pdb.c		\
	../libligma/ligmadrawable_pdb.c		\
	../libligma/ligmadrawablecolor_pdb.c	\
	../libligma/ligmadrawableedit_pdb.c	\
	../libligma/ligmadynamics_pdb.c		\
	../libligma/ligmaedit_pdb.c		\
	../libligma/ligmafile_pdb.c		\
	../libligma/ligmafloatingsel_pdb.c	\
	../libligma/ligmafonts_pdb.c		\
	../libligma/ligmafontselect_pdb.c		\
	../libligma/ligmaligmarc_pdb.c		\
	../libligma/ligmagradient_pdb.c		\
	../libligma/ligmagradients_pdb.c		\
	../libligma/ligmagradientselect_pdb.c	\
	../libligma/ligmahelp_pdb.c		\
	../libligma/ligmaimage_pdb.c		\
	../libligma/ligmaimagecolorprofile_pdb.c	\
	../libligma/ligmaimageconvert_pdb.c	\
	../libligma/ligmaimagegrid_pdb.c		\
	../libligma/ligmaimageguides_pdb.c	\
	../libligma/ligmaimagesamplepoints_pdb.c	\
	../libligma/ligmaimageselect_pdb.c	\
	../libligma/ligmaimagetransform_pdb.c	\
	../libligma/ligmaimageundo_pdb.c		\
	../libligma/ligmaitem_pdb.c		\
	../libligma/ligmaitemtransform_pdb.c	\
	../libligma/ligmalayer_pdb.c		\
	../libligma/ligmamessage_pdb.c		\
	../libligma/ligmapainttools_pdb.c		\
	../libligma/ligmapalette_pdb.c		\
	../libligma/ligmapalettes_pdb.c		\
	../libligma/ligmapaletteselect_pdb.c	\
	../libligma/ligmapattern_pdb.c		\
	../libligma/ligmapatterns_pdb.c		\
	../libligma/ligmapatternselect_pdb.c	\
	../libligma/ligmaprogress_pdb.c		\
	../libligma/ligmaselection_pdb.c		\
	../libligma/ligmatextlayer_pdb.c		\
	../libligma/ligmatexttool_pdb.c		\
	../libligma/ligmavectors_pdb.c

PDB_WRAPPERS_H = \
	../libligma/ligma_pdb_headers.h		\
	../libligma/ligma_pdb.h			\
	../libligma/ligmabrush_pdb.h		\
	../libligma/ligmabrushes_pdb.h		\
	../libligma/ligmabrushselect_pdb.h	\
	../libligma/ligmabuffer_pdb.h		\
	../libligma/ligmachannel_pdb.h		\
	../libligma/ligmacontext_pdb.h		\
	../libligma/ligmadebug_pdb.h		\
	../libligma/ligmadisplay_pdb.h		\
	../libligma/ligmadrawable_pdb.h		\
	../libligma/ligmadrawablecolor_pdb.h	\
	../libligma/ligmadrawableedit_pdb.h	\
	../libligma/ligmadynamics_pdb.h		\
	../libligma/ligmaedit_pdb.h		\
	../libligma/ligmafile_pdb.h		\
	../libligma/ligmafloatingsel_pdb.h	\
	../libligma/ligmafonts_pdb.h		\
	../libligma/ligmafontselect_pdb.h		\
	../libligma/ligmaligmarc_pdb.h		\
	../libligma/ligmagradient_pdb.h		\
	../libligma/ligmagradients_pdb.h		\
	../libligma/ligmagradientselect_pdb.h	\
	../libligma/ligmahelp_pdb.h		\
	../libligma/ligmaimage_pdb.h		\
	../libligma/ligmaimagecolorprofile_pdb.h	\
	../libligma/ligmaimageconvert_pdb.h	\
	../libligma/ligmaimagegrid_pdb.h		\
	../libligma/ligmaimageguides_pdb.h	\
	../libligma/ligmaimagesamplepoints_pdb.h	\
	../libligma/ligmaimageselect_pdb.h	\
	../libligma/ligmaimagetransform_pdb.h	\
	../libligma/ligmaimageundo_pdb.h		\
	../libligma/ligmaitem_pdb.h		\
	../libligma/ligmaitemtransform_pdb.h	\
	../libligma/ligmalayer_pdb.h		\
	../libligma/ligmamessage_pdb.h		\
	../libligma/ligmapainttools_pdb.h		\
	../libligma/ligmapalette_pdb.h		\
	../libligma/ligmapalettes_pdb.h		\
	../libligma/ligmapaletteselect_pdb.h	\
	../libligma/ligmapattern_pdb.h		\
	../libligma/ligmapatterns_pdb.h		\
	../libligma/ligmapatternselect_pdb.h	\
	../libligma/ligmaprogress_pdb.h		\
	../libligma/ligmaselection_pdb.h		\
	../libligma/ligmatextlayer_pdb.h		\
	../libligma/ligmatexttool_pdb.h		\
	../libligma/ligmavectors_pdb.h

libligma_built_sources = \
	ligmaenums.c

libligma_introspectable_headers = \
	../libligma/ligma.h			\
	../libligma/ligmatypes.h			\
	../libligma/ligmaenums.h			\
	${PDB_WRAPPERS_H}			\
	../libligma/ligmabatchprocedure.h		\
	../libligma/ligmabrushselect.h		\
	../libligma/ligmachannel.h		\
	../libligma/ligmadisplay.h		\
	../libligma/ligmadrawable.h		\
	../libligma/ligmafileprocedure.h		\
	../libligma/ligmafontselect.h		\
	../libligma/ligmaligmarc.h			\
	../libligma/ligmagradientselect.h		\
	../libligma/ligmaimage.h			\
	../libligma/ligmaimagecolorprofile.h	\
	../libligma/ligmaimagemetadata.h		\
	../libligma/ligmaimageprocedure.h		\
	../libligma/ligmaitem.h			\
	../libligma/ligmalayer.h			\
	../libligma/ligmalayermask.h		\
	../libligma/ligmaloadprocedure.h		\
	../libligma/ligmapaletteselect.h		\
	../libligma/ligmaparamspecs.h		\
	../libligma/ligmapatternselect.h		\
	../libligma/ligmapdb.h			\
	../libligma/ligmaplugin.h			\
	../libligma/ligmaprocedure.h		\
	../libligma/ligmaprocedureconfig.h	\
	../libligma/ligmaprogress.h		\
	../libligma/ligmasaveprocedure.h		\
	../libligma/ligmaselection.h		\
	../libligma/ligmatextlayer.h		\
	../libligma/ligmathumbnailprocedure.h	\
	../libligma/ligmavectors.h

libligma_introspectable = \
	$(libligma_introspectable_headers)	\
	$(libligma_built_sources)		\
	../libligma/ligma.c			\
	${PDB_WRAPPERS_C}			\
	../libligma/ligmabatchprocedure.c		\
	../libligma/ligmabrushselect.c		\
	../libligma/ligmachannel.c		\
	../libligma/ligmadisplay.c		\
	../libligma/ligmadrawable.c		\
	../libligma/ligmafileprocedure.c		\
	../libligma/ligmafontselect.c		\
	../libligma/ligmaligmarc.c			\
	../libligma/ligmagradientselect.c		\
	../libligma/ligmaimage.c			\
	../libligma/ligmaimagecolorprofile.c	\
	../libligma/ligmaimagemetadata.c		\
	../libligma/ligmaimagemetadata-save.c	\
	../libligma/ligmaimageprocedure.c		\
	../libligma/ligmaitem.c			\
	../libligma/ligmalayer.c			\
	../libligma/ligmalayermask.c		\
	../libligma/ligmaloadprocedure.c		\
	../libligma/ligmapaletteselect.c		\
	../libligma/ligmaparamspecs.c		\
	../libligma/ligmapatternselect.c		\
	../libligma/ligmapdb.c			\
	../libligma/ligmaplugin.c			\
	../libligma/ligmaprocedure.c		\
	../libligma/ligmaprocedureconfig.c	\
	../libligma/ligmaprogress.c		\
	../libligma/ligmasaveprocedure.c		\
	../libligma/ligmaselection.c		\
	../libligma/ligmatextlayer.c		\
	../libligma/ligmathumbnailprocedure.c	\
	../libligma/ligmavectors.c

libligmaui_introspectable_headers = \
	../libligma/ligmaui.h			\
	../libligma/ligmauitypes.h		\
	../libligma/ligmaaspectpreview.h		\
	../libligma/ligmabrushselectbutton.h	\
	../libligma/ligmadrawablepreview.h   	\
	../libligma/ligmaexport.h			\
	../libligma/ligmafontselectbutton.h	\
	../libligma/ligmagradientselectbutton.h	\
	../libligma/ligmaimagecombobox.h		\
	../libligma/ligmaitemcombobox.h		\
	../libligma/ligmapaletteselectbutton.h	\
	../libligma/ligmapatternselectbutton.h	\
	../libligma/ligmaprocbrowserdialog.h	\
	../libligma/ligmaproceduredialog.h	\
	../libligma/ligmaprocview.h		\
	../libligma/ligmaprogressbar.h		\
	../libligma/ligmasaveproceduredialog.h	\
	../libligma/ligmaselectbutton.h		\
	../libligma/ligmazoompreview.h

libligmaui_introspectable = \
	$(libligmaui_introspectable_headers)	\
	../libligma/ligmaui.c			\
	../libligma/ligmaaspectpreview.c     	\
	../libligma/ligmabrushselectbutton.c	\
	../libligma/ligmadrawablepreview.c	\
	../libligma/ligmaexport.c			\
	../libligma/ligmafontselectbutton.c	\
	../libligma/ligmagradientselectbutton.c	\
	../libligma/ligmaimagecombobox.c		\
	../libligma/ligmaitemcombobox.c		\
	../libligma/ligmapaletteselectbutton.c	\
	../libligma/ligmapatternselectbutton.c	\
	../libligma/ligmaprocbrowserdialog.c	\
	../libligma/ligmaproceduredialog.c	\
	../libligma/ligmaprocview.c		\
	../libligma/ligmasaveproceduredialog.c	\
	../libligma/ligmaprogressbar.c		\
	../libligma/ligmaselectbutton.c		\
	../libligma/ligmazoompreview.c
