/* Depth Merge -- Combine two image layers via corresponding depth maps
 * Copyright (C) 1997, 1998 Sean Cier (scier@PostHorizon.com)
 *
 * A plug-in for The GIMP
 * The GIMP is Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/* Version 1.0.0: (14 August 1998)
 *   Math optimizations, miscellaneous speedups
 *
 * Version 0.1: (6 July 1997)
 *   Initial Release
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

#include "config.h"
#include "libgimp/stdplugins-intl.h"

#define DEBUG

#ifndef LERP
#define LERP(frac,a,b) ((frac)*(b) + (1-(frac))*(a))
#endif

#define MUL255(i) ((i)*256 - (i))
#define DIV255(i) (((i) + (i)/256 + 1) / 256)

#define PLUG_IN_NAME    "plug_in_depth_merge"
#define PLUG_IN_VERSION "1.0.0; 14 August 1998"

#define PREVIEW_SIZE 256
#define SCALE_WIDTH  200
#define ENTRY_WIDTH  60
#define CHECK_SIZE   8
#define CHECK_DARK   255*1/3
#define CHECK_LIGHT  255*2/3



/* ----- DepthMerge ----- */

struct _DepthMerge;

typedef struct _DepthMergeValueEdit {
  struct _DepthMerge   *dm;
  float                *value;
  GtkAdjustment        *scaleData;
  GtkWidget            *entry;
  float                min, max;
} DepthMergeValueEdit;

typedef struct _DepthMergeInterface {
  gint      active;

  GtkWidget *dialog;

  GtkWidget *preview;
  gint      previewWidth;
  gint      previewHeight;

  DepthMergeValueEdit *overlapValueEdit;
  DepthMergeValueEdit *offsetValueEdit;
  DepthMergeValueEdit *scale1ValueEdit;
  DepthMergeValueEdit *scale2ValueEdit;

  guchar              *checkRow0, *checkRow1;
  guchar              *previewSource1,   *previewSource2,
                      *previewDepthMap1, *previewDepthMap2;

  gint                run;
} DepthMergeInterface;

typedef struct _DepthMergeParams {
  gint32 result;
  gint32 source1,   source2;
  gint32 depthMap1, depthMap2;
  gfloat overlap;
  gfloat offset;
  gfloat scale1, scale2;
} DepthMergeParams;


typedef struct _DepthMerge {
  DepthMergeInterface *interface;
  DepthMergeParams    params;

  GDrawable           *resultDrawable,
                      *source1Drawable, *source2Drawable,
                      *depthMap1Drawable, *depthMap2Drawable;
  gint                selectionX0, selectionY0, selectionX1, selectionY1,
                      selectionWidth, selectionHeight;
  gint                resultHasAlpha;
} DepthMerge;
 
void   DepthMerge_initParams(             DepthMerge *dm);
void   DepthMerge_construct(              DepthMerge *dm);
void   DepthMerge_destroy(                DepthMerge *dm);
gint32 DepthMerge_execute(                DepthMerge *dm);
void   DepthMerge_executeRegion(          DepthMerge *dm,
                                          guchar *source1Row,
                                          guchar *source2Row,
                                          guchar *depthMap1Row,
                                          guchar *depthMap2Row,
                                          guchar *resultRow,
					  gint length);
gint32 DepthMerge_dialog(                 DepthMerge *dm);
DepthMergeValueEdit *
       DepthMerge_createValueEdit(        DepthMerge *dm,
                                          char *title, GtkTable *table,
                                          int row, int startCol,
       		                          float *value,
                                          float min, float max, float step);
void   DepthMerge_buildPreviewSourceImage(DepthMerge *dm);
void   DepthMerge_updatePreview(          DepthMerge *dm);


gint constraintResultSizeAndResultColorOrGray(gint32 imageId,
					      gint32 drawableId, gpointer data);
gint constraintResultSizeAndGray(gint32 imageId,
				 gint32 drawableId, gpointer data);
void dialogOkCallback(    GtkWidget *widget, gpointer data);
void dialogCancelCallback(GtkWidget *widget, gpointer data);
void dialogCloseCallback( GtkWidget *widget, gpointer data);
void dialogSource1ChangedCallback(  gint32 id, gpointer data);
void dialogSource2ChangedCallback(  gint32 id, gpointer data);
void dialogDepthMap1ChangedCallback(gint32 id, gpointer data);
void dialogDepthMap2ChangedCallback(gint32 id, gpointer data);
void dialogValueScaleUpdateCallback(GtkAdjustment *adjustment, gpointer data);
void dialogValueEntryUpdateCallback(GtkWidget *widget, gpointer data);


void util_fillReducedBuffer(guchar *dest, gint destWidth, gint destHeight,
                            gint destBPP, gint destHasAlpha,
                            GDrawable *sourceDrawable,
                            gint x0, gint y0,
                            gint sourceWidth, gint sourceHeight);
void util_convertColorspace(guchar *dest,
			    gint destBPP,   gint destHasAlpha,
			    guchar *source,
			    gint sourceBPP, gint sourceHasAlpha,
			    gint length);

/* ----- plug-in entry points ----- */

static void query();
static void run(char *name,
    int numParams,      GParam *param,
    int *numReturnVals, GParam **returnVals);

GPlugInInfo PLUG_IN_INFO = {
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run    /* run_proc   */
};

MAIN()

static void query() {
  static GParamDef args[] = {
    { PARAM_INT32,    "run_mode",     "Interactive, non-interactive" },
    { PARAM_IMAGE,    "image",        "Input image (unused)" },
    { PARAM_DRAWABLE, "result",       "Result" },
    { PARAM_DRAWABLE, "source1",      "Source 1" },
    { PARAM_DRAWABLE, "source2",      "Source 2" },
    { PARAM_DRAWABLE, "depthMap1",    "Depth map 1" },
    { PARAM_DRAWABLE, "depthMap2",    "Depth map 2" },
    { PARAM_FLOAT,    "overlap",      "Overlap" },
    { PARAM_FLOAT,    "offset",       "Depth relative offset" },
    { PARAM_FLOAT,    "scale1",       "Depth relative scale 1" },
    { PARAM_FLOAT,    "scale2",       "Depth relative scale 2" },
  };
  static int       numArgs = sizeof(args) / sizeof(GParamDef);

  INIT_I18N();

  gimp_install_procedure(PLUG_IN_NAME,
    _("Combine two images using corresponding depth maps (z-buffers)"),
    _("Taking as input two full-colour, full-alpha images and two "
      "corresponding grayscale depth maps, this plug-in combines the "
      "images based on which is closer (has a lower depth map value) "
      "at each point."),
    "Sean Cier",
    "Sean Cier",
    PLUG_IN_VERSION,
    _("<Image>/Filters/Combine/Depth Merge"),
    "RGB*, GRAY*",
    PROC_PLUG_IN,
    numArgs,
    0,
    args,
    NULL);
}


static void run(char *name,
    int numParams,      GParam *param,
    int *numReturnVals, GParam **returnVals) {
  static GParam    values[1];
  GRunModeType     runMode;
  GStatusType      status;
  DepthMerge       dm;

  INIT_I18N_UI();

  runMode = (GRunModeType)param[0].data.d_int32;
  status = STATUS_SUCCESS;
  *numReturnVals = 1;
  *returnVals    = values;

  switch (runMode) {
    case RUN_INTERACTIVE:
      DepthMerge_initParams(&dm);
      gimp_get_data(PLUG_IN_NAME, &(dm.params));
      dm.params.result = param[2].data.d_drawable;
      DepthMerge_construct(&dm);
      if (!DepthMerge_dialog(&dm)) {
	values[0].type = PARAM_STATUS;
        values[0].data.d_status = STATUS_SUCCESS;
        return;
      }
      break;
    case RUN_NONINTERACTIVE:
      DepthMerge_initParams(&dm);
      if (numParams != 11)
	status = STATUS_CALLING_ERROR;
      else {
	dm.params.result    = param[ 2].data.d_drawable;
	dm.params.source1   = param[ 3].data.d_drawable;
	dm.params.source2   = param[ 4].data.d_drawable;
	dm.params.depthMap1 = param[ 5].data.d_drawable;
	dm.params.depthMap2 = param[ 6].data.d_drawable;
	dm.params.overlap   = param[ 7].data.d_float;
        dm.params.offset    = param[ 8].data.d_float;
        dm.params.scale1    = param[ 9].data.d_float;
        dm.params.scale2    = param[10].data.d_float;
      }
      DepthMerge_construct(&dm);
      break;
    case RUN_WITH_LAST_VALS:
      DepthMerge_initParams(&dm);
      gimp_get_data(PLUG_IN_NAME, &(dm.params));
      DepthMerge_construct(&dm);
      break;
    default:
      status = STATUS_CALLING_ERROR;
  }

  if (status == STATUS_SUCCESS) {
    gimp_tile_cache_ntiles((dm.resultDrawable->width + gimp_tile_width() - 1) /
                           gimp_tile_width());

    if (!DepthMerge_execute(&dm))
      status = STATUS_EXECUTION_ERROR;
    else {
      if (runMode != RUN_NONINTERACTIVE)
        gimp_displays_flush();
      if (runMode == RUN_INTERACTIVE)
	gimp_set_data(PLUG_IN_NAME, &(dm.params), sizeof(DepthMergeParams));
    }
  }

  DepthMerge_destroy(&dm);

  values[0].data.d_status = status;
}


/* ----- DepthMerge ----- */

void DepthMerge_initParams(DepthMerge *dm) {
  dm->params.result    = -1;
  dm->params.source1   = -1;
  dm->params.source2   = -1;
  dm->params.depthMap1 = -1;
  dm->params.depthMap2 = -1;
  dm->params.overlap   =  0;
  dm->params.offset    =  0;
  dm->params.scale1    =  1;
  dm->params.scale2    =  1;
}

void DepthMerge_construct(DepthMerge *dm) {
  dm->interface = NULL;

  dm->resultDrawable = gimp_drawable_get(dm->params.result);
  gimp_drawable_mask_bounds(dm->resultDrawable->id,
			    &(dm->selectionX0), &(dm->selectionY0),
			    &(dm->selectionX1), &(dm->selectionY1));
  dm->selectionWidth  = dm->selectionX1 - dm->selectionX0;
  dm->selectionHeight = dm->selectionY1 - dm->selectionY0;
  dm->resultHasAlpha = gimp_drawable_has_alpha(dm->resultDrawable->id);

  dm->source1Drawable   = (dm->params.source1   == -1) ? NULL :
    gimp_drawable_get(dm->params.source1  );
  dm->source2Drawable   = (dm->params.source2   == -1) ? NULL :
    gimp_drawable_get(dm->params.source2  );
  dm->depthMap1Drawable = (dm->params.depthMap1 == -1) ? NULL :
    gimp_drawable_get(dm->params.depthMap1);
  dm->depthMap2Drawable = (dm->params.depthMap2 == -1) ? NULL :
    gimp_drawable_get(dm->params.depthMap2);

  dm->params.overlap = CLAMP(dm->params.overlap, 0, 2);
  dm->params.offset  = CLAMP(dm->params.offset, -1, 1);
  dm->params.scale1  = CLAMP(dm->params.scale1, -1, 1);
  dm->params.scale2  = CLAMP(dm->params.scale2, -1, 1);
}

void DepthMerge_destroy(DepthMerge *dm) {
  if (dm->interface != NULL) {
    g_free(dm->interface->overlapValueEdit);
    g_free(dm->interface->offsetValueEdit );
    g_free(dm->interface->scale1ValueEdit );
    g_free(dm->interface->scale2ValueEdit );
    g_free(dm->interface->checkRow0);
    g_free(dm->interface->checkRow1);
    g_free(dm->interface->previewSource1  );
    g_free(dm->interface->previewSource2  );
    g_free(dm->interface->previewDepthMap1);
    g_free(dm->interface->previewDepthMap2);
    g_free(dm->interface);
  }
  if (dm->resultDrawable    != NULL)
    gimp_drawable_detach(dm->resultDrawable);
  if (dm->source1Drawable   != NULL)
    gimp_drawable_detach(dm->source1Drawable  );
  if (dm->source2Drawable   != NULL)
    gimp_drawable_detach(dm->source2Drawable  );
  if (dm->depthMap1Drawable != NULL)
    gimp_drawable_detach(dm->depthMap1Drawable);
  if (dm->depthMap2Drawable != NULL)
    gimp_drawable_detach(dm->depthMap2Drawable);
}


gint32 DepthMerge_execute(DepthMerge *dm) {
  int       x, y;
  GPixelRgn source1Rgn, source2Rgn, depthMap1Rgn, depthMap2Rgn,
            resultRgn;
  guchar    *source1Row, *source2Row, *depthMap1Row, *depthMap2Row,
            *resultRow,
            *tempRow;
  gint      source1HasAlpha, source2HasAlpha,
            depthMap1HasAlpha, depthMap2HasAlpha;

  /* initialize */

  source1HasAlpha = 0;
  source2HasAlpha = 0;
  depthMap1HasAlpha = 0;
  depthMap2HasAlpha = 0;
  
  gimp_progress_init(_("Depth-merging..."));

  resultRow    = (guchar *)g_malloc(dm->selectionWidth * 4);
  source1Row   = (guchar *)g_malloc(dm->selectionWidth * 4);
  source2Row   = (guchar *)g_malloc(dm->selectionWidth * 4);
  depthMap1Row = (guchar *)g_malloc(dm->selectionWidth    );
  depthMap2Row = (guchar *)g_malloc(dm->selectionWidth    );
  tempRow      = (guchar *)g_malloc(dm->selectionWidth * 4);

  if (dm->source1Drawable != NULL) {
    source1HasAlpha = gimp_drawable_has_alpha(dm->source1Drawable->id);
    gimp_pixel_rgn_init(&source1Rgn, dm->source1Drawable,
                        dm->selectionX0, dm->selectionY0,
                        dm->selectionWidth, dm->selectionHeight,
                        FALSE, FALSE);
  } else
    for (x = 0; x < dm->selectionWidth; x++) {
      source1Row[4*x  ] = 0;
      source1Row[4*x+1] = 0;
      source1Row[4*x+2] = 0;
      source1Row[4*x+3] = 255;
    }
  if (dm->source2Drawable != NULL) {
    source2HasAlpha = gimp_drawable_has_alpha(dm->source2Drawable->id);
    gimp_pixel_rgn_init(&source2Rgn, dm->source2Drawable,
                        dm->selectionX0, dm->selectionY0,
                        dm->selectionWidth, dm->selectionHeight,
                        FALSE, FALSE);
  } else
    for (x = 0; x < dm->selectionWidth; x++) {
      source2Row[4*x  ] = 0;
      source2Row[4*x+1] = 0;
      source2Row[4*x+2] = 0;
      source2Row[4*x+3] = 255;
    }
  if (dm->depthMap1Drawable != NULL) {
    depthMap1HasAlpha = gimp_drawable_has_alpha(dm->depthMap1Drawable->id);
    gimp_pixel_rgn_init(&depthMap1Rgn, dm->depthMap1Drawable,
                        dm->selectionX0, dm->selectionY0,
                        dm->selectionWidth, dm->selectionHeight,
                        FALSE, FALSE);
  } else
    for (x = 0; x < dm->selectionWidth; x++) {
      depthMap1Row[x  ] = 0;
    }
  if (dm->depthMap2Drawable != NULL) {
    depthMap2HasAlpha = gimp_drawable_has_alpha(dm->depthMap2Drawable->id);
    gimp_pixel_rgn_init(&depthMap2Rgn, dm->depthMap2Drawable,
                        dm->selectionX0, dm->selectionY0,
                        dm->selectionWidth, dm->selectionHeight,
                        FALSE, FALSE);
  } else
    for (x = 0; x < dm->selectionWidth; x++) {
      depthMap2Row[x  ] = 0;
    }
  gimp_pixel_rgn_init(&resultRgn, dm->resultDrawable,
                      dm->selectionX0, dm->selectionY0,
                      dm->selectionWidth, dm->selectionHeight,
                      TRUE, TRUE);

  for (y = dm->selectionY0; y < dm->selectionY1; y++) {
    if (dm->source1Drawable != NULL) {
      gimp_pixel_rgn_get_row(&source1Rgn, tempRow,
                             dm->selectionX0, y,
                             dm->selectionWidth);
      util_convertColorspace(source1Row, 4, TRUE,
                             tempRow,
                             dm->source1Drawable->bpp, source1HasAlpha,
                             dm->selectionWidth);
    }
    if (dm->source2Drawable != NULL) {
      gimp_pixel_rgn_get_row(&source2Rgn, tempRow,
                             dm->selectionX0, y,
                             dm->selectionWidth);
      util_convertColorspace(source2Row, 4, TRUE,
                             tempRow,
                             dm->source2Drawable->bpp, source2HasAlpha,
                             dm->selectionWidth);
    }
    if (dm->depthMap1Drawable != NULL) {
      gimp_pixel_rgn_get_row(&depthMap1Rgn, tempRow,
                             dm->selectionX0, y,
                             dm->selectionWidth);
      util_convertColorspace(depthMap1Row, 1, FALSE,
                             tempRow,
                             dm->depthMap1Drawable->bpp, depthMap1HasAlpha,
                             dm->selectionWidth);
    }
    if (dm->depthMap2Drawable != NULL) {
      gimp_pixel_rgn_get_row(&depthMap2Rgn, tempRow,
                             dm->selectionX0, y,
                             dm->selectionWidth);
      util_convertColorspace(depthMap2Row, 1, FALSE,
                             tempRow,
                             dm->depthMap2Drawable->bpp, depthMap2HasAlpha,
                             dm->selectionWidth);
    }

    DepthMerge_executeRegion(dm,
			     source1Row, source2Row, depthMap1Row, depthMap2Row,
			     resultRow,
			     dm->selectionWidth);
    util_convertColorspace(tempRow, dm->resultDrawable->bpp, dm->resultHasAlpha,
			   resultRow, 4, TRUE,
			   dm->selectionWidth);

    gimp_pixel_rgn_set_row(&resultRgn, tempRow,
                           dm->selectionX0, y,
                           dm->selectionWidth);

    gimp_progress_update((double)(y-dm->selectionY0) / (double)(dm->selectionHeight-1));
  }

  g_free(resultRow);
  g_free(source1Row);
  g_free(source2Row);
  g_free(depthMap1Row);
  g_free(depthMap2Row);
  g_free(tempRow);

  gimp_drawable_flush(dm->resultDrawable);
  gimp_drawable_merge_shadow(dm->resultDrawable->id, TRUE);
  gimp_drawable_update(dm->resultDrawable->id,
                       dm->selectionX0, dm->selectionY0,
                       dm->selectionWidth, dm->selectionHeight);
  return(TRUE);
}

void DepthMerge_executeRegion(DepthMerge *dm,
                              guchar *source1Row, guchar *source2Row,
			      guchar *depthMap1Row, guchar *depthMap2Row,
			      guchar *resultRow, gint length) {
  float          scale1, scale2, offset255, invOverlap255;
  float          frac, depth1, depth2;
  unsigned short c1[4], c2[4], cR1[4], cR2[4], cR[4], temp;
  int            i, tempInt;

  invOverlap255 = 1.0 / (MAX(dm->params.overlap, 0.001)*255);
  offset255  = dm->params.offset * 255;
  scale1     = dm->params.scale1;
  scale2     = dm->params.scale2;

  for (i = 0; i < length; i++) {
    depth1 = (float)depthMap1Row[i];
    depth2 = (float)depthMap2Row[i];

    frac = (depth2*scale2 - (depth1*scale1 + offset255)) * invOverlap255;
    frac = 0.5 * (frac+1.0);
    frac = CLAMP(frac, 0.0, 1.0);

      /* c1 -> color corresponding to source1 */
    c1[0] = source1Row[4*i  ];
    c1[1] = source1Row[4*i+1];
    c1[2] = source1Row[4*i+2];
    c1[3] = source1Row[4*i+3];

      /* c2 -> color corresponding to source2 */
    c2[0] = source2Row[4*i  ];
    c2[1] = source2Row[4*i+1];
    c2[2] = source2Row[4*i+2];
    c2[3] = source2Row[4*i+3];

    if (frac != 0) {
        /* cR1 -> result if c1 is completely on top */
      cR1[0] = c1[3]*c1[0]   + (255-c1[3])*c2[0];
      cR1[1] = c1[3]*c1[1]   + (255-c1[3])*c2[1];
      cR1[2] = c1[3]*c1[2]   + (255-c1[3])*c2[2];
      cR1[3] = MUL255(c1[3]) + (255-c1[3])*c2[3];
    }

    if (frac != 1) {
        /* cR2 -> result if c2 is completely on top */
      cR2[0] = c2[3]*c2[0]   + (255-c2[3])*c1[0];
      cR2[1] = c2[3]*c2[1]   + (255-c2[3])*c1[1];
      cR2[2] = c2[3]*c2[2]   + (255-c2[3])*c1[2];
      cR2[3] = MUL255(c2[3]) + (255-c2[3])*c1[3];
    }

    if (frac == 1) {
      cR[0] = cR1[0];
      cR[1] = cR1[1];
      cR[2] = cR1[2];
      cR[3] = cR1[3];
    } else if (frac == 0) {
      cR[0] = cR2[0];
      cR[1] = cR2[1];
      cR[2] = cR2[2];
      cR[3] = cR2[3];
    } else {
      tempInt = LERP(frac, cR2[0], cR1[0]); cR[0] = CLAMP(tempInt,0,255*255);
      tempInt = LERP(frac, cR2[1], cR1[1]); cR[1] = CLAMP(tempInt,0,255*255);
      tempInt = LERP(frac, cR2[2], cR1[2]); cR[2] = CLAMP(tempInt,0,255*255);
      tempInt = LERP(frac, cR2[3], cR1[3]); cR[3] = CLAMP(tempInt,0,255*255);
    }

    temp = DIV255(cR[0]); resultRow[4*i  ] = MIN(temp, 255);
    temp = DIV255(cR[1]); resultRow[4*i+1] = MIN(temp, 255);
    temp = DIV255(cR[2]); resultRow[4*i+2] = MIN(temp, 255);
    temp = DIV255(cR[3]); resultRow[4*i+3] = MIN(temp, 255);
  }
}

gint32 DepthMerge_dialog(DepthMerge *dm) {
  GtkWidget *topTable;
  GtkWidget   *previewFrame;
  GtkWidget *sourceTable;
  GtkWidget   *tempLabel;
  GtkWidget   *tempOptionMenu;
  GtkWidget   *tempMenu;
  GtkWidget *numericParameterTable;
  GtkWidget *tempButton;
  gint      argc;
  gchar     **argv;
  guchar    *color_cube;

  dm->interface = (DepthMergeInterface *)g_malloc(sizeof(DepthMergeInterface));
  dm->interface->active = FALSE;
  dm->interface->run = FALSE;

  argc = 1;
  argv = g_new(gchar *, 1);
  argv[0] = g_strdup(PLUG_IN_NAME);

  gtk_init(&argc, &argv);
  gtk_rc_parse(gimp_gtkrc());

  gdk_set_use_xshm(gimp_use_xshm());

  gtk_preview_set_gamma(gimp_gamma());
  gtk_preview_set_install_cmap(gimp_install_cmap());
  color_cube = gimp_color_cube();
  gtk_preview_set_color_cube(color_cube[0],
			     color_cube[1],
			     color_cube[2],
			     color_cube[3]);

  gtk_widget_set_default_visual(gtk_preview_get_visual());
  gtk_widget_set_default_colormap(gtk_preview_get_cmap());

  dm->interface->dialog = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(dm->interface->dialog), _("Depth Merge"));
  gtk_window_position(GTK_WINDOW(dm->interface->dialog), GTK_WIN_POS_MOUSE);
  gtk_container_border_width(GTK_CONTAINER(dm->interface->dialog), 0);
  gtk_signal_connect(GTK_OBJECT(dm->interface->dialog), "destroy",
		     (GtkSignalFunc)dialogCloseCallback, NULL);

  /* topTable */
  topTable = gtk_table_new(3, 3, FALSE);
  gtk_container_border_width(GTK_CONTAINER(topTable), 6);
  gtk_table_set_row_spacings(GTK_TABLE(topTable), 4);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dm->interface->dialog)->vbox), topTable,
		     FALSE, FALSE, 0);
  gtk_widget_show(topTable);

  /* Preview */
  previewFrame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(previewFrame), GTK_SHADOW_IN);
  gtk_table_attach(GTK_TABLE(topTable), previewFrame, 1, 2, 0, 1, 0, 0, 0, 0);
  gtk_widget_show(previewFrame);

  dm->interface->previewWidth  = MIN(dm->selectionWidth,  PREVIEW_SIZE);
  dm->interface->previewHeight = MIN(dm->selectionHeight, PREVIEW_SIZE);
  dm->interface->preview = gtk_preview_new(GTK_PREVIEW_COLOR);
  gtk_preview_size(GTK_PREVIEW(dm->interface->preview),
		   dm->interface->previewWidth,
		   dm->interface->previewHeight);
  gtk_container_add(GTK_CONTAINER(previewFrame), dm->interface->preview);
  gtk_widget_show(dm->interface->preview);

  DepthMerge_buildPreviewSourceImage(dm);

  /* Source and Depth Map selection */
  sourceTable = gtk_table_new(2, 4, FALSE);
  gtk_container_border_width(GTK_CONTAINER(sourceTable), 2);
  gtk_table_attach(GTK_TABLE(topTable), sourceTable, 0, 3, 1, 2,
		   GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
  gtk_widget_show(sourceTable);

  tempLabel = gtk_label_new(_("Source 1"));
  gtk_misc_set_alignment(GTK_MISC(tempLabel), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(sourceTable), tempLabel, 0, 1, 0, 1,
		   GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show(tempLabel);
  tempOptionMenu = gtk_option_menu_new();
  gtk_table_attach(GTK_TABLE(sourceTable), tempOptionMenu, 1, 2, 0, 1,
		   GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show(tempOptionMenu);
  tempMenu = gimp_drawable_menu_new(
				    constraintResultSizeAndResultColorOrGray,
				    dialogSource1ChangedCallback,
				    dm,
				    dm->params.source1);
  gtk_option_menu_set_menu(GTK_OPTION_MENU(tempOptionMenu), tempMenu);
  gtk_widget_show(tempOptionMenu);

  tempLabel = gtk_label_new(_("Depth Map"));
  gtk_misc_set_alignment(GTK_MISC(tempLabel), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(sourceTable), tempLabel, 2, 3, 0, 1,
		   GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show(tempLabel);
  tempOptionMenu = gtk_option_menu_new();
  gtk_table_attach(GTK_TABLE(sourceTable), tempOptionMenu, 3, 4, 0, 1,
		   GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show(tempOptionMenu);
  tempMenu = gimp_drawable_menu_new(
				    constraintResultSizeAndResultColorOrGray,
	        		    dialogDepthMap1ChangedCallback,
				    dm,
				    dm->params.depthMap1);
  gtk_option_menu_set_menu(GTK_OPTION_MENU(tempOptionMenu), tempMenu);
  gtk_widget_show(tempOptionMenu);

  tempLabel = gtk_label_new(_("Source 2"));
  gtk_misc_set_alignment(GTK_MISC(tempLabel), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(sourceTable), tempLabel, 0, 1, 1, 2,
		   GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show(tempLabel);
  tempOptionMenu = gtk_option_menu_new();
  gtk_table_attach(GTK_TABLE(sourceTable), tempOptionMenu, 1, 2, 1, 2,
		   GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show(tempOptionMenu);
  tempMenu = gimp_drawable_menu_new(
				    constraintResultSizeAndResultColorOrGray,
				    dialogSource2ChangedCallback,
				    dm,
				    dm->params.source2);
  gtk_option_menu_set_menu(GTK_OPTION_MENU(tempOptionMenu), tempMenu);
  gtk_widget_show(tempOptionMenu);

  tempLabel = gtk_label_new(_("Depth Map"));
  gtk_misc_set_alignment(GTK_MISC(tempLabel), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(sourceTable), tempLabel, 2, 3, 1, 2,
		   GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show(tempLabel);
  tempOptionMenu = gtk_option_menu_new();
  gtk_table_attach(GTK_TABLE(sourceTable), tempOptionMenu, 3, 4, 1, 2,
		   GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show(tempOptionMenu);
  tempMenu = gimp_drawable_menu_new(
				    constraintResultSizeAndResultColorOrGray,
	        		    dialogDepthMap2ChangedCallback,
				    dm,
				    dm->params.depthMap2);
  gtk_option_menu_set_menu(GTK_OPTION_MENU(tempOptionMenu), tempMenu);
  gtk_widget_show(tempOptionMenu);

  /* Numeric parameters */
  numericParameterTable = gtk_table_new(4, 3, FALSE);
  gtk_container_border_width(GTK_CONTAINER(numericParameterTable), 2);
  gtk_table_attach(GTK_TABLE(topTable), numericParameterTable, 0, 3, 2, 3,
		   GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
  gtk_widget_show(numericParameterTable);

  dm->interface->overlapValueEdit =
    DepthMerge_createValueEdit(dm, _("Overlap"),
			       GTK_TABLE(numericParameterTable), 0, 0,
			       &(dm->params.overlap),
			       0, 2, 0.001);
  dm->interface->offsetValueEdit =
    DepthMerge_createValueEdit(dm, _("Offset"),
			       GTK_TABLE(numericParameterTable), 1, 0,
			       &(dm->params.offset),
			       -1, 1, 0.001);
  dm->interface->scale1ValueEdit =
    DepthMerge_createValueEdit(dm, _("Scale 1"),
			       GTK_TABLE(numericParameterTable), 2, 0,
			       &(dm->params.scale1),
			       -1, 1, 0.001);
  dm->interface->scale2ValueEdit =
    DepthMerge_createValueEdit(dm, _("Scale 2"),
			       GTK_TABLE(numericParameterTable), 3, 0,
			       &(dm->params.scale2),
			       -1, 1, 0.001);

  /* Buttons */

  gtk_container_border_width(GTK_CONTAINER(GTK_DIALOG(dm->interface->dialog)->action_area), 6);
  tempButton = gtk_button_new_with_label(_("OK"));
  GTK_WIDGET_SET_FLAGS(tempButton, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(tempButton), "clicked",
		     (GtkSignalFunc)dialogOkCallback,
		     dm);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dm->interface->dialog)->action_area),
		     tempButton, TRUE, TRUE, 0);
  gtk_widget_grab_default(tempButton);
  gtk_widget_show(tempButton);

  tempButton = gtk_button_new_with_label(_("Cancel"));
  GTK_WIDGET_SET_FLAGS(tempButton, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(tempButton), "clicked",
		     (GtkSignalFunc)dialogCancelCallback,
		     dm);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dm->interface->dialog)->action_area),
		     tempButton, TRUE, TRUE, 0);
  gtk_widget_show(tempButton);

  /* Done */

  dm->interface->active = TRUE;
  gtk_widget_show(dm->interface->dialog);
  DepthMerge_updatePreview(dm);

  gtk_main();
  gdk_flush();

  return(dm->interface->run);
}


DepthMergeValueEdit *DepthMerge_createValueEdit(DepthMerge *dm,
						char *title, GtkTable *table,
						int row, int startCol,
						float *value,
						float min, float max,
                                                float step) {
  GtkWidget           *label;
  GtkWidget           *scale;
  GtkWidget           *entry;
  GtkObject           *scaleData;
  DepthMergeValueEdit *valueEdit;
  char                buf[256];

  valueEdit = (DepthMergeValueEdit *)g_malloc(sizeof(DepthMergeValueEdit));
  valueEdit->dm = dm;
  valueEdit->value = value;

  *(valueEdit->value) = CLAMP(*(valueEdit->value), min, max);

  label = gtk_label_new(title);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 1.0);
  gtk_table_attach(table, label, startCol, startCol+1, row, row+1,
		   GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show(label);

  scaleData = gtk_adjustment_new(*value,
                                 min, max,
                                 step, step,
                                 0.0);
  gtk_signal_connect(GTK_OBJECT(scaleData), "value_changed",
		     (GtkSignalFunc)dialogValueScaleUpdateCallback,
		     valueEdit);
  scale = gtk_hscale_new(GTK_ADJUSTMENT(scaleData));
  gtk_widget_set_usize(scale, SCALE_WIDTH, 0);
  gtk_table_attach(table, scale, startCol+1, startCol+2, row, row+1,
		   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
  gtk_scale_set_digits(GTK_SCALE(scale), 3);
  gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_CONTINUOUS);
  gtk_widget_show(scale);

  entry = gtk_entry_new();
  gtk_widget_set_usize(entry, ENTRY_WIDTH, 0);
  sprintf(buf, "%0.3f", (float)*value);
  gtk_entry_set_text(GTK_ENTRY(entry), buf);
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
		     (GtkSignalFunc) dialogValueEntryUpdateCallback,
		     valueEdit);
  gtk_table_attach(GTK_TABLE(table), entry, startCol+2, startCol+3, row, row+1,
		   GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show(entry);

  valueEdit->scaleData = GTK_ADJUSTMENT(scaleData);
  valueEdit->entry     = entry;
  valueEdit->min       = min;
  valueEdit->max       = max;

  return(valueEdit);
}


void DepthMerge_buildPreviewSourceImage(DepthMerge *dm) {
  int x;

  dm->interface->checkRow0 = (guchar *)g_malloc(dm->interface->previewWidth *
				                sizeof(guchar));
  dm->interface->checkRow1 = (guchar *)g_malloc(dm->interface->previewWidth *
				                sizeof(guchar));

  for (x = 0; x < dm->interface->previewWidth; x++) {
    if ((x / CHECK_SIZE) & 1) {
      dm->interface->checkRow0[x] = CHECK_DARK;
      dm->interface->checkRow1[x] = CHECK_LIGHT;
    } else {
      dm->interface->checkRow0[x] = CHECK_LIGHT;
      dm->interface->checkRow1[x] = CHECK_DARK;
    }
  }

  dm->interface->previewSource1   =
    (guchar *)g_malloc(dm->interface->previewWidth *
	               dm->interface->previewHeight * 4);
  util_fillReducedBuffer(dm->interface->previewSource1,
                         dm->interface->previewWidth,
                         dm->interface->previewHeight,
                         4, TRUE,
                         dm->source1Drawable,
                         dm->selectionX0, dm->selectionY0,
                         dm->selectionWidth, dm->selectionHeight);
  dm->interface->previewSource2   =
    (guchar *)g_malloc(dm->interface->previewWidth *
		       dm->interface->previewHeight * 4);
  util_fillReducedBuffer(dm->interface->previewSource2,
                         dm->interface->previewWidth,
                         dm->interface->previewHeight,
                         4, TRUE,
                         dm->source2Drawable,
                         dm->selectionX0, dm->selectionY0,
                         dm->selectionWidth, dm->selectionHeight);
  dm->interface->previewDepthMap1 =
    (guchar *)g_malloc(dm->interface->previewWidth *
		       dm->interface->previewHeight * 1);
  util_fillReducedBuffer(dm->interface->previewDepthMap1,
                         dm->interface->previewWidth,
                         dm->interface->previewHeight,
                         1, FALSE,
                         dm->depthMap1Drawable,
                         dm->selectionX0, dm->selectionY0,
                         dm->selectionWidth, dm->selectionHeight);
  dm->interface->previewDepthMap2 =
    (guchar *)g_malloc(dm->interface->previewWidth *
		       dm->interface->previewHeight * 1);
  util_fillReducedBuffer(dm->interface->previewDepthMap2,
                         dm->interface->previewWidth,
                         dm->interface->previewHeight,
                         1, FALSE,
                         dm->depthMap2Drawable,
                         dm->selectionX0, dm->selectionY0,
                         dm->selectionWidth, dm->selectionHeight);
}

void DepthMerge_updatePreview(DepthMerge *dm) {
  int    x, y, i;
  guchar *source1Row, *source2Row, *depthMap1Row, *depthMap2Row,
         *resultRowRGBA, *resultRow,
         *checkRow;

  if (!dm->interface->active) return;

  resultRowRGBA = (guchar *)g_malloc(dm->interface->previewWidth * 4);
  resultRow     = (guchar *)g_malloc(dm->interface->previewWidth * 3);

  for (y = 0; y < dm->interface->previewHeight; y++) {
    checkRow = ((y/CHECK_SIZE)&1) ?
      dm->interface->checkRow1 :
      dm->interface->checkRow0;

    source1Row =
      &(dm->interface->previewSource1[  y * dm->interface->previewWidth * 4]);
    source2Row =
      &(dm->interface->previewSource2[  y * dm->interface->previewWidth * 4]);
    depthMap1Row =
      &(dm->interface->previewDepthMap1[y * dm->interface->previewWidth    ]);
    depthMap2Row =
      &(dm->interface->previewDepthMap2[y * dm->interface->previewWidth    ]);

    DepthMerge_executeRegion(dm,
			     source1Row, source2Row, depthMap1Row, depthMap2Row,
			     resultRowRGBA,
			     dm->interface->previewWidth);
    for (x = 0; x < dm->interface->previewWidth; x++) {
      i = ((int)(      resultRowRGBA[x*4+3])*(int)resultRowRGBA[x*4  ] +
           (int)(255 - resultRowRGBA[x*4+3])*(int)checkRow[x]          );
      resultRow[x*3  ] = DIV255(i);
      i = ((int)(      resultRowRGBA[x*4+3])*(int)resultRowRGBA[x*4+1] +
           (int)(255 - resultRowRGBA[x*4+3])*(int)checkRow[x]          );
      resultRow[x*3+1] = DIV255(i);
      i = ((int)(      resultRowRGBA[x*4+3])*(int)resultRowRGBA[x*4+2] +
           (int)(255 - resultRowRGBA[x*4+3])*(int)checkRow[x]          );
      resultRow[x*3+2] = DIV255(i);
    }
    gtk_preview_draw_row(GTK_PREVIEW(dm->interface->preview), resultRow, 0, y,
			 dm->interface->previewWidth);
  }

  g_free(resultRowRGBA);
  g_free(resultRow);

  gtk_widget_draw(dm->interface->preview, NULL);
  gdk_flush();
}


/* ----- Callbacks ----- */


gint constraintResultSizeAndResultColorOrGray(gint32 imageId,
					      gint32 drawableId, gpointer data) {
  DepthMerge *dm = (DepthMerge *)data;

  return((drawableId == -1) ||
         ((gimp_drawable_width( drawableId) ==
	   gimp_drawable_width( dm->params.result)) &&
	  (gimp_drawable_height(drawableId) ==
	   gimp_drawable_height(dm->params.result)) &&
	  ((gimp_drawable_color(drawableId) &&
	    (gimp_drawable_color(dm->params.result))) ||
	   gimp_drawable_gray( drawableId))));
}

gint constraintResultSizeAndGray(gint32 imageId,
				 gint32 drawableId, gpointer data) {
  DepthMerge *dm = (DepthMerge *)data;

  return((drawableId == -1) ||
         ((gimp_drawable_width( drawableId) ==
	   gimp_drawable_width( dm->params.result)) &&
	  (gimp_drawable_height(drawableId) ==
	   gimp_drawable_height(dm->params.result)) &&
	  (gimp_drawable_gray( drawableId))));
}

void dialogOkCallback(GtkWidget *widget, gpointer data) {
  DepthMerge *dm = (DepthMerge *)data;
  dm->interface->run = TRUE;
  gtk_widget_destroy(dm->interface->dialog);
}

void dialogCancelCallback(GtkWidget *widget, gpointer data) {
  DepthMerge *dm = (DepthMerge *)data;
  dm->interface->run = FALSE;
  gtk_widget_destroy(dm->interface->dialog);
}

void dialogCloseCallback(GtkWidget *widget, gpointer data) {
  gtk_main_quit();
}


void dialogSource1ChangedCallback(gint32 id, gpointer data) {
  DepthMerge *dm = (DepthMerge *)data;

  if (dm->source1Drawable   != NULL)
    gimp_drawable_detach(dm->source1Drawable  );
  dm->params.source1   = id;
  dm->source1Drawable   = (dm->params.source1   == -1) ? NULL :
    gimp_drawable_get(dm->params.source1  );

  util_fillReducedBuffer(dm->interface->previewSource1,
                         dm->interface->previewWidth,
                         dm->interface->previewHeight,
                         4, TRUE,
                         dm->source1Drawable,
                         dm->selectionX0, dm->selectionY0,
                         dm->selectionWidth, dm->selectionHeight);

  DepthMerge_updatePreview(dm);
}
void dialogSource2ChangedCallback(gint32 id, gpointer data) {
  DepthMerge *dm = (DepthMerge *)data;

  if (dm->source2Drawable   != NULL)
    gimp_drawable_detach(dm->source2Drawable  );
  dm->params.source2   = id;
  dm->source2Drawable   = (dm->params.source2   == -1) ? NULL :
    gimp_drawable_get(dm->params.source2  );

  util_fillReducedBuffer(dm->interface->previewSource2,
                         dm->interface->previewWidth,
                         dm->interface->previewHeight,
                         4, TRUE,
                         dm->source2Drawable,
                         dm->selectionX0, dm->selectionY0,
                         dm->selectionWidth, dm->selectionHeight);

  DepthMerge_updatePreview(dm);
}
void dialogDepthMap1ChangedCallback(gint32 id, gpointer data) {
  DepthMerge *dm = (DepthMerge *)data;

  if (dm->depthMap1Drawable != NULL)
    gimp_drawable_detach(dm->depthMap1Drawable);
  dm->params.depthMap1 = id;
  dm->depthMap1Drawable = (dm->params.depthMap1 == -1) ? NULL :
    gimp_drawable_get(dm->params.depthMap1);

  util_fillReducedBuffer(dm->interface->previewDepthMap1,
                         dm->interface->previewWidth,
                         dm->interface->previewHeight,
                         1, FALSE,
                         dm->depthMap1Drawable,
                         dm->selectionX0, dm->selectionY0,
                         dm->selectionWidth, dm->selectionHeight);

  DepthMerge_updatePreview(dm);
}
void dialogDepthMap2ChangedCallback(gint32 id, gpointer data) {
  DepthMerge *dm = (DepthMerge *)data;

  if (dm->depthMap2Drawable != NULL)
    gimp_drawable_detach(dm->depthMap2Drawable);
  dm->params.depthMap2 = id;
  dm->depthMap2Drawable = (dm->params.depthMap2 == -1) ? NULL :
    gimp_drawable_get(dm->params.depthMap2);

  util_fillReducedBuffer(dm->interface->previewDepthMap2,
                         dm->interface->previewWidth,
                         dm->interface->previewHeight,
                         1, FALSE,
                         dm->depthMap2Drawable,
                         dm->selectionX0, dm->selectionY0,
                         dm->selectionWidth, dm->selectionHeight);

  DepthMerge_updatePreview(dm);
}

void dialogValueScaleUpdateCallback(GtkAdjustment *adjustment, gpointer data) {
  DepthMergeValueEdit *valueEdit = (DepthMergeValueEdit *)data;
  char buf[256];

  if (*(valueEdit->value) != adjustment->value) {
    *(valueEdit->value) = adjustment->value;

    sprintf(buf, "%0.3f", (float)(adjustment->value));

    gtk_signal_handler_block_by_data(GTK_OBJECT(valueEdit->entry), data);
    gtk_entry_set_text(GTK_ENTRY(valueEdit->entry), buf);
    gtk_signal_handler_unblock_by_data(GTK_OBJECT(valueEdit->entry), data);

    DepthMerge_updatePreview(valueEdit->dm);
  }
}

void dialogValueEntryUpdateCallback(GtkWidget *widget, gpointer data) {
  DepthMergeValueEdit *valueEdit = (DepthMergeValueEdit *)data;
  float newValue;

  newValue = atof(gtk_entry_get_text(GTK_ENTRY(widget)));

  if ((*(valueEdit->value) != newValue) &&
      (newValue >= valueEdit->min)   &&
      (newValue <= valueEdit->max)) {
    valueEdit->scaleData->value = newValue;

    gtk_signal_emit_by_name(GTK_OBJECT(valueEdit->scaleData),
			    "value_changed");

    DepthMerge_updatePreview(valueEdit->dm);
  }
}


/* ----- Utility routines ----- */

void util_fillReducedBuffer(guchar *dest, gint destWidth, gint destHeight,
                            gint destBPP, gint destHasAlpha,
                            GDrawable *sourceDrawable,
                            gint x0, gint y0,
                            gint sourceWidth, gint sourceHeight) {
  GPixelRgn rgn;
  guchar    *sourceBuffer, *reducedRowBuffer,
            *sourceBufferRow, *sourceBufferPos, *reducedRowBufferPos;
  int       x, y, i, yPrime, sourceHasAlpha, sourceBpp;
  int       *sourceRowOffsetLookup;

  if ((sourceDrawable == NULL) || (sourceWidth == 0) || (sourceHeight == 0)) {
    for (x = 0; x < destWidth*destHeight*destBPP; x++)
      dest[x] = 0;
    return;
  }

  sourceBpp = sourceDrawable->bpp;

  sourceBuffer          = (guchar *)g_malloc(sourceWidth * sourceHeight *
                                             sourceBpp);
  reducedRowBuffer      = (guchar *)g_malloc(destWidth   * sourceBpp);
  sourceRowOffsetLookup = (int    *)g_malloc(destWidth   * sizeof(int));
  gimp_pixel_rgn_init(&rgn, sourceDrawable, x0, y0, sourceWidth, sourceHeight,
		      FALSE, FALSE);
  sourceHasAlpha = gimp_drawable_has_alpha(sourceDrawable->id);

  for (x = 0; x < destWidth; x++)
    sourceRowOffsetLookup[x] = (x*(sourceWidth-1)/(destWidth-1))*sourceBpp;

  gimp_pixel_rgn_get_rect(&rgn, sourceBuffer,
                          x0, y0, sourceWidth, sourceHeight);

  for (y = 0; y < destHeight; y++) {
    yPrime = y*(sourceHeight-1)/(destHeight-1);
    sourceBufferRow = &(sourceBuffer[yPrime * sourceWidth * sourceBpp]);
    sourceBufferPos = sourceBufferRow;
    reducedRowBufferPos = reducedRowBuffer;
    for (x = 0; x < destWidth; x++) {
      sourceBufferPos = sourceBufferRow + sourceRowOffsetLookup[x];
      for (i = 0; i < sourceBpp; i++) 
        reducedRowBufferPos[i] = sourceBufferPos[i];
      reducedRowBufferPos += sourceBpp;
    }
    util_convertColorspace(&(dest[y*destWidth*destBPP]), destBPP, destHasAlpha,
                           reducedRowBuffer, sourceDrawable->bpp, sourceHasAlpha,
                           destWidth);
  }

  g_free(sourceBuffer);
  g_free(reducedRowBuffer);
  g_free(sourceRowOffsetLookup);
}


/* Utterly pathetic kludge to convert between color spaces;
   likes gray and rgb best, of course.  Others will be creatively mutilated,
   and even rgb->gray is pretty bad */
void util_convertColorspace(guchar *dest,
			    gint destBPP,   gint destHasAlpha,
			    guchar *source,
			    gint sourceBPP, gint sourceHasAlpha,
			    gint length) {
  int i, j, sourcePos, destPos, accum;
  int sourceColorBPP = sourceHasAlpha ? (sourceBPP-1) : sourceBPP;
  int destColorBPP   = destHasAlpha   ? (destBPP  -1) : destBPP;

  if (((sourceColorBPP != 1) && (sourceColorBPP != 3)) ||
      ((destColorBPP   != 1) && (destColorBPP   != 3)))
    fprintf(stderr, _("Warning: I don't _like_ this color space.  This is a suggestion, not a threat.\n"));

  if ((sourceColorBPP == destColorBPP) &&
      (sourceBPP      == destBPP     )) {
    j = length*sourceBPP;
    for (i = 0; i < j; i++) dest[i] = source[i];
    return;
  }

  if (sourceColorBPP == destColorBPP) {
    for (i = destPos = sourcePos = 0; i < length;
         i++, destPos += destBPP, sourcePos += sourceBPP) {
      for (j = 0; j < destColorBPP; j++)
	dest[destPos + j] = source[sourcePos + j];
    }
  } else if (sourceColorBPP == 1) {
    /* Duplicate single "gray" source byte across all dest bytes */
    for (i = destPos = sourcePos = 0; i < length;
         i++, destPos += destBPP, sourcePos += sourceBPP) {
      for (j = 0; j < destColorBPP; j++)
	dest[destPos + j] = source[sourcePos];
    }
  } else if (destColorBPP == 1) {
    /* Average all source bytes into single "gray" dest byte */
    for (i = destPos = sourcePos = 0; i < length;
         i++, destPos += destBPP, sourcePos += sourceBPP) {
      accum = 0;
      for (j = 0; j < sourceColorBPP; j++)
	accum += source[sourcePos + j];
      dest[destPos] = accum/sourceColorBPP;
    }
  } else if (destColorBPP < sourceColorBPP) {
    /* Copy as many corresponding bytes from source to dest as will fit */
    for (i = destPos = sourcePos = 0; i < length;
         i++, destPos += destBPP, sourcePos += sourceBPP) {
      for (j = 0; j < destColorBPP; j++)
	dest[destPos + j] = source[sourcePos + j];
    }
  } else /* destColorBPP > sourceColorBPP */ {
    /* Fill extra dest bytes with zero */
    for (i = destPos = sourcePos = 0; i < length;
         i++, destPos += destBPP, sourcePos += sourceBPP) {
      for (j = 0; j < sourceColorBPP; j++)
	dest[destPos + j] = source[destPos + j];
      for (     ; j < destColorBPP; j++)
	dest[destPos + j] = 0;
    }
  }

  if (destHasAlpha) {
    if (sourceHasAlpha) {
      for (i = 0, destPos = destColorBPP, sourcePos = sourceColorBPP;
           i < length;
           i++, destPos += destBPP, sourcePos += sourceBPP) {
        for (i = 0; i < length; i++)
          dest[destPos] = source[sourcePos];
      }
    } else {
      for (i = 0, destPos = destColorBPP; i < length; i++, destPos += destBPP) {
	dest[destPos] = 255;
      }
    }
  }
}
