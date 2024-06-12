/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Depth Merge -- Combine two image layers via corresponding depth maps
 * Copyright (C) 1997, 1998 Sean Cier (scier@PostHorizon.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define DEBUG

#ifndef LERP
#define LERP(frac,a,b) ((frac)*(b) + (1-(frac))*(a))
#endif

#define MUL255(i) ((i)*256 - (i))
#define DIV255(i) (((i) + (i)/256 + 1) / 256)

#define PLUG_IN_PROC    "plug-in-depth-merge"
#define PLUG_IN_VERSION "August 1998"
#define PLUG_IN_BINARY  "depth-merge"
#define PLUG_IN_ROLE    "gimp-depth-merge"

#define PREVIEW_SIZE    256


typedef struct _Merge      Merge;
typedef struct _MergeClass MergeClass;

struct _Merge
{
  GimpPlugIn parent_instance;

  gint       selectionX;
  gint       selectionY;
  gint       selectionWidth;
  gint       selectionHeight;
  gint       resultHasAlpha;

  GtkWidget *preview;
  gint       previewWidth;
  gint       previewHeight;

  guchar    *previewSource1;
  guchar    *previewSource2;
  guchar    *previewDepthMap1;
  guchar    *previewDepthMap2;

  GimpProcedureConfig *config;
};

struct _MergeClass
{
  GimpPlugInClass parent_class;
};


#define MERGE_TYPE (merge_get_type ())
#define MERGE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), MERGE_TYPE, Merge))

GType                   merge_get_type                     (void) G_GNUC_CONST;

static void             merge_finalize                     (GObject              *object);

static GList          * merge_query_procedures             (GimpPlugIn           *plug_in);
static GimpProcedure  * merge_create_procedure             (GimpPlugIn           *plug_in,
                                                           const gchar          *name);

static GimpValueArray * merge_run                          (GimpProcedure        *procedure,
                                                            GimpRunMode           run_mode,
                                                            GimpImage            *image,
                                                            gint                  n_drawables,
                                                            GimpDrawable        **drawables,
                                                            GimpProcedureConfig  *config,
                                                            gpointer              run_data);

static gint32           DepthMerge_execute                 (Merge                *dm,
                                                            GimpDrawable         *resultDrawable,
                                                            GimpProcedureConfig  *config);
static void             DepthMerge_executeRegion           (GimpProcedureConfig  *config,
                                                            guchar               *source1Row,
                                                            guchar               *source2Row,
                                                            guchar               *depthMap1Row,
                                                            guchar               *depthMap2Row,
                                                            guchar               *resultRow,
                                                            gint                  length,
                                                            gfloat                overlap,
                                                            gfloat                offset,
                                                            gfloat                scale1,
                                                            gfloat                scale2);
static gboolean         DepthMerge_dialog                  (Merge                *merge,
                                                            GimpProcedure        *procedure,
                                                            GimpProcedureConfig  *config);
static void             DepthMerge_buildPreviewSourceImage (Merge                *dm,
                                                            GimpProcedureConfig  *config);
static void             DepthMerge_updatePreview           (Merge                *dm,
                                                            GimpProcedureConfig  *config);


static void             util_fillReducedBuffer             (guchar               *dest,
                                                            const Babl           *dest_format,
                                                            gint                  destWidth,
                                                            gint                  destHeight,
                                                            GimpDrawable         *sourceDrawable,
                                                            gint                  x0,
                                                            gint                  y0,
                                                            gint                  sourceWidth,
                                                            gint                  sourceHeight);

static void              merge_preview_size_allocate       (GtkWidget            *widget,
                                                            GtkAllocation        *allocation,
                                                            Merge                *dm);
static void              merge_preview_config_notify       (GimpProcedureConfig  *config,
                                                            const GParamSpec     *pspec,
                                                            Merge                *dm);


G_DEFINE_TYPE (Merge, merge, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (MERGE_TYPE)
DEFINE_STD_SET_I18N


static void
merge_class_init (MergeClass *klass)
{
  GObjectClass    *object_class  = G_OBJECT_CLASS (klass);
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  object_class->finalize          = merge_finalize;

  plug_in_class->query_procedures = merge_query_procedures;
  plug_in_class->create_procedure = merge_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
merge_init (Merge *merge)
{
  merge->previewSource1   = NULL;
  merge->previewSource2   = NULL;
  merge->previewDepthMap1 = NULL;
  merge->previewDepthMap2 = NULL;
}

static void
merge_finalize (GObject *object)
{
  Merge *merge = MERGE (object);

  g_free (merge->previewSource1);
  g_free (merge->previewSource2);
  g_free (merge->previewDepthMap1);
  g_free (merge->previewDepthMap2);

  G_OBJECT_CLASS (merge_parent_class)->finalize (object);
}

static GList *
merge_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
merge_create_procedure (GimpPlugIn  *plug_in,
                        const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            merge_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "RGB*, GRAY*");
      gimp_procedure_set_sensitivity_mask (procedure,
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLE);

      gimp_procedure_set_menu_label (procedure, _("_Depth Merge..."));
      gimp_procedure_add_menu_path (procedure, "<Image>/Filters/Combine");

      gimp_procedure_set_documentation (procedure,
                                        _("Combine two images using depth "
                                          "maps (z-buffers)"),
                                        "Taking as input two full-color, "
                                        "full-alpha images and two "
                                        "corresponding grayscale depth maps, "
                                        "this plug-in combines the images based "
                                        "on which is closer (has a lower depth "
                                        "map value) at each point.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Sean Cier",
                                      "Sean Cier",
                                      PLUG_IN_VERSION);

      gimp_procedure_add_drawable_argument (procedure, "source-1",
                                            _("Source _1"),
                                            _("Source 1"),
                                            TRUE,
                                            G_PARAM_READWRITE);

      gimp_procedure_add_drawable_argument (procedure, "depth-map-1",
                                            _("_Depth map 1"),
                                            _("Depth map 1"),
                                            TRUE,
                                            G_PARAM_READWRITE);

      gimp_procedure_add_drawable_argument (procedure, "source-2",
                                            _("Source _2"),
                                            _("Source 2"),
                                            TRUE,
                                            G_PARAM_READWRITE);

      gimp_procedure_add_drawable_argument (procedure, "depth-map-2",
                                            _("Depth _map 2"),
                                            _("Depth map 2"),
                                            TRUE,
                                            G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "overlap",
                                          _("O_verlap"),
                                          _("Overlap"),
                                          0, 2, 0,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "offset",
                                          _("O_ffset"),
                                          _("Depth relative offset"),
                                          -1, 1, 0,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "scale-1",
                                          _("Sc_ale 1"),
                                          _("Depth relative scale 1"),
                                          -1, 1, 1,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "scale-2",
                                          _("Scal_e 2"),
                                          _("Depth relative scale 2"),
                                          -1, 1, 1,
                                          G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
merge_run (GimpProcedure        *procedure,
           GimpRunMode           run_mode,
           GimpImage            *image,
           gint                  n_drawables,
           GimpDrawable        **drawables,
           GimpProcedureConfig  *config,
           gpointer              run_data)
{
  GimpDrawable *drawable;
  Merge        *merge = MERGE (gimp_procedure_get_plug_in (procedure));

  gegl_init (NULL, NULL);

  if (n_drawables != 1)
    {
      GError *error = NULL;

      g_set_error (&error, GIMP_PLUG_IN_ERROR, 0,
                   _("Procedure '%s' only works with one drawable."),
                   PLUG_IN_PROC);

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_CALLING_ERROR,
                                               error);
    }
  else
    {
      drawable = drawables[0];
    }

  merge->config = config;
  if (! gimp_drawable_mask_intersect (drawable,
                                      &(merge->selectionX), &(merge->selectionY),
                                      &(merge->selectionWidth),
                                      &(merge->selectionHeight)))
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR,
                                             g_error_new_literal (GIMP_PLUG_IN_ERROR, 0,
                                                                  _("The selection does not intersect with the input drawable.")));

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      g_object_set (config,
                    "source-1",    drawable,
                    "source-2",    drawable,
                    "depth-map-1", drawable,
                    "depth-map-2", drawable,
                    NULL);

      if (! DepthMerge_dialog (merge, procedure, config))
        return gimp_procedure_new_return_values (procedure,
                                                 GIMP_PDB_CANCEL,
                                                 NULL);
    }

  if (! DepthMerge_execute (merge, drawable, config))
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR, NULL);
  else if (run_mode != GIMP_RUN_NONINTERACTIVE)
    gimp_displays_flush ();

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}

/* ----- DepthMerge ----- */

static gint32
DepthMerge_execute (Merge               *dm,
                    GimpDrawable        *resultDrawable,
                    GimpProcedureConfig *config)
{
  int           x, y;
  GeglBuffer   *source1_buffer   = NULL;
  GeglBuffer   *source2_buffer   = NULL;
  GeglBuffer   *depthMap1_buffer = NULL;
  GeglBuffer   *depthMap2_buffer = NULL;
  GeglBuffer   *result_buffer    = NULL;
  guchar       *source1Row,   *source2Row;
  guchar       *depthMap1Row, *depthMap2Row;
  guchar       *resultRow;
  guchar       *tempRow;

  GimpDrawable *source1Drawable = NULL;
  GimpDrawable *source2Drawable = NULL;
  GimpDrawable *depthMap1Drawable = NULL;
  GimpDrawable *depthMap2Drawable = NULL;
  gdouble       overlap = 0.0;
  gdouble       offset = 0.0;
  gdouble       scale1 = 0.0;
  gdouble       scale2 = 0.0;

  gimp_progress_init (_("Depth-merging"));

  g_object_get (config,
                "source-1",    &source1Drawable,
                "source-2",    &source2Drawable,
                "depth-map-1", &depthMap1Drawable,
                "depth-map-2", &depthMap2Drawable,
                "overlap",     &overlap,
                "offset",      &offset,
                "scale-1",     &scale1,
                "scale-2",     &scale2,
                NULL);

  resultRow    = g_new (guchar, dm->selectionWidth * 4);
  source1Row   = g_new (guchar, dm->selectionWidth * 4);
  source2Row   = g_new (guchar, dm->selectionWidth * 4);
  depthMap1Row = g_new (guchar, dm->selectionWidth    );
  depthMap2Row = g_new (guchar, dm->selectionWidth    );
  tempRow      = g_new (guchar, dm->selectionWidth * 4);

  if (source1Drawable)
    {
      source1_buffer = gimp_drawable_get_buffer (source1Drawable);
    }
  else
    {
      for (x = 0; x < dm->selectionWidth; x++)
        {
          source1Row[4 * x    ] = 0;
          source1Row[4 * x + 1] = 0;
          source1Row[4 * x + 2] = 0;
          source1Row[4 * x + 3] = 255;
        }
    }

  if (source2Drawable)
    {
      source2_buffer = gimp_drawable_get_buffer (source2Drawable);
    }
  else
    {
      for (x = 0; x < dm->selectionWidth; x++)
        {
          source2Row[4 * x    ] = 0;
          source2Row[4 * x + 1] = 0;
          source2Row[4 * x + 2] = 0;
          source2Row[4 * x + 3] = 255;
        }
    }

  if (depthMap1Drawable)
    {
      depthMap1_buffer = gimp_drawable_get_buffer (depthMap1Drawable);
    }
  else
    {
      for (x = 0; x < dm->selectionWidth; x++)
        depthMap1Row[x] = 0;
    }

  if (depthMap2Drawable)
    {
      depthMap2_buffer = gimp_drawable_get_buffer (depthMap2Drawable);
    }
  else
    {
      for (x = 0; x < dm->selectionWidth; x++)
        depthMap2Row[x] = 0;
    }

  result_buffer = gimp_drawable_get_shadow_buffer (resultDrawable);

  for (y = dm->selectionY; y < (dm->selectionY + dm->selectionHeight); y++)
    {
      if (source1Drawable)
        gegl_buffer_get (source1_buffer,
                         GEGL_RECTANGLE (dm->selectionX, y,
                                         dm->selectionWidth, 1), 1.0,
                         babl_format ("R'G'B'A u8"), source1Row,
                         GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      if (source2Drawable)
        gegl_buffer_get (source2_buffer,
                         GEGL_RECTANGLE (dm->selectionX, y,
                                         dm->selectionWidth, 1), 1.0,
                         babl_format ("R'G'B'A u8"), source2Row,
                         GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      if (depthMap1Drawable)
        gegl_buffer_get (depthMap1_buffer,
                         GEGL_RECTANGLE (dm->selectionX, y,
                                         dm->selectionWidth, 1), 1.0,
                         babl_format ("Y' u8"), depthMap1Row,
                         GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      if (depthMap2Drawable)
        gegl_buffer_get (depthMap2_buffer,
                         GEGL_RECTANGLE (dm->selectionX, y,
                                         dm->selectionWidth, 1), 1.0,
                         babl_format ("Y' u8"), depthMap2Row,
                         GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      DepthMerge_executeRegion (config,
                                source1Row, source2Row, depthMap1Row, depthMap2Row,
                                resultRow,
                                dm->selectionWidth, overlap, offset, scale1, scale2);

      gegl_buffer_set (result_buffer,
                       GEGL_RECTANGLE (dm->selectionX, y,
                                       dm->selectionWidth, 1), 0,
                       babl_format ("R'G'B'A u8"), resultRow,
                       GEGL_AUTO_ROWSTRIDE);

      gimp_progress_update ((double)(y-dm->selectionY) /
                            (double)(dm->selectionHeight - 1));
    }

  g_clear_object (&source1Drawable);
  g_clear_object (&source2Drawable);
  g_clear_object (&depthMap1Drawable);
  g_clear_object (&depthMap2Drawable);
  g_free (resultRow);
  g_free (source1Row);
  g_free (source2Row);
  g_free (depthMap1Row);
  g_free (depthMap2Row);
  g_free (tempRow);

  gimp_progress_update (1.0);

  g_clear_object (&source1_buffer);
  g_clear_object (&source2_buffer);
  g_clear_object (&depthMap1_buffer);
  g_clear_object (&depthMap2_buffer);
  g_clear_object (&result_buffer);

  gimp_drawable_merge_shadow (resultDrawable, TRUE);
  gimp_drawable_update (resultDrawable,
                        dm->selectionX, dm->selectionY,
                        dm->selectionWidth, dm->selectionHeight);

  return TRUE;
}

static void
DepthMerge_executeRegion (GimpProcedureConfig *config,
                          guchar              *source1Row,
                          guchar              *source2Row,
                          guchar              *depthMap1Row,
                          guchar              *depthMap2Row,
                          guchar              *resultRow,
                          gint                 length,
                          gfloat               overlap,
                          gfloat               offset,
                          gfloat               scale1,
                          gfloat               scale2)
{
  gfloat  offset255, invOverlap255;
  gfloat  frac, depth1, depth2;
  gushort c1[4], c2[4];
  gushort cR1[4] = { 0, 0, 0, 0 }, cR2[4] = { 0, 0, 0, 0 };
  gushort cR[4], temp;
  gint    i, tempInt;

  invOverlap255 = 1.0 / (MAX (overlap, 0.001) * 255);
  offset255     = offset * 255;

  for (i = 0; i < length; i++)
    {
      depth1 = (gfloat) depthMap1Row[i];
      depth2 = (gfloat) depthMap2Row[i];

      frac = (depth2 * scale2 - (depth1 * scale1 + offset255)) * invOverlap255;
      frac = 0.5 * (frac + 1.0);
      frac = CLAMP(frac, 0.0, 1.0);

      /* c1 -> color corresponding to source1 */
      c1[0] = source1Row[4 * i    ];
      c1[1] = source1Row[4 * i + 1];
      c1[2] = source1Row[4 * i + 2];
      c1[3] = source1Row[4 * i + 3];

      /* c2 -> color corresponding to source2 */
      c2[0] = source2Row[4 * i    ];
      c2[1] = source2Row[4 * i + 1];
      c2[2] = source2Row[4 * i + 2];
      c2[3] = source2Row[4 * i + 3];

      if (frac != 0)
        {
          /* cR1 -> result if c1 is completely on top */
          cR1[0] = c1[3] * c1[0]  + (255 - c1[3]) * c2[0];
          cR1[1] = c1[3] * c1[1]  + (255 - c1[3]) * c2[1];
          cR1[2] = c1[3] * c1[2]  + (255 - c1[3]) * c2[2];
          cR1[3] = MUL255 (c1[3]) + (255 - c1[3]) * c2[3];
        }

      if (frac != 1)
        {
          /* cR2 -> result if c2 is completely on top */
          cR2[0] = c2[3] * c2[0]  + (255 - c2[3]) * c1[0];
          cR2[1] = c2[3] * c2[1]  + (255 - c2[3]) * c1[1];
          cR2[2] = c2[3] * c2[2]  + (255 - c2[3]) * c1[2];
          cR2[3] = MUL255 (c2[3]) + (255 - c2[3]) * c1[3];
        }

      if (frac == 1)
        {
          cR[0] = cR1[0];
          cR[1] = cR1[1];
          cR[2] = cR1[2];
          cR[3] = cR1[3];
        }
      else if (frac == 0)
        {
          cR[0] = cR2[0];
          cR[1] = cR2[1];
          cR[2] = cR2[2];
          cR[3] = cR2[3];
        }
      else
        {
          tempInt = LERP (frac, cR2[0], cR1[0]);
          cR[0] = CLAMP (tempInt,0,255 * 255);
          tempInt = LERP (frac, cR2[1], cR1[1]);
          cR[1] = CLAMP (tempInt,0,255 * 255);
          tempInt = LERP (frac, cR2[2], cR1[2]);
          cR[2] = CLAMP (tempInt,0,255 * 255);
          tempInt = LERP (frac, cR2[3], cR1[3]);
          cR[3] = CLAMP (tempInt,0,255 * 255);
        }

      temp = DIV255 (cR[0]); resultRow[4 * i    ] = MIN (temp, 255);
      temp = DIV255 (cR[1]); resultRow[4 * i + 1] = MIN (temp, 255);
      temp = DIV255 (cR[2]); resultRow[4 * i + 2] = MIN (temp, 255);
      temp = DIV255 (cR[3]); resultRow[4 * i + 3] = MIN (temp, 255);
    }
}

static gboolean
DepthMerge_dialog (Merge               *merge,
                   GimpProcedure       *procedure,
                   GimpProcedureConfig *config)
{
  GtkWidget *dialog;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY);

  dialog = gimp_procedure_dialog_new (procedure,
                                      GIMP_PROCEDURE_CONFIG (config),
                                      _("Depth Merge"));
  /* Preview */
  merge->previewWidth  = MIN (merge->selectionWidth,  PREVIEW_SIZE);
  merge->previewHeight = MIN (merge->selectionHeight, PREVIEW_SIZE);
  merge->preview = gimp_preview_area_new ();
  DepthMerge_buildPreviewSourceImage (merge, config);
  g_signal_connect (merge->preview, "size-allocate",
                    G_CALLBACK (merge_preview_size_allocate),
                    merge);
  gtk_widget_set_size_request (merge->preview,
                               merge->previewWidth,
                               merge->previewHeight);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      merge->preview, TRUE, TRUE, 0);
  gtk_widget_show (merge->preview);

  g_signal_connect (config, "notify",
                    G_CALLBACK (merge_preview_config_notify),
                    merge);

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog), NULL);
  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}

static void
DepthMerge_buildPreviewSourceImage (Merge               *dm,
                                    GimpProcedureConfig *config)
{
  GimpDrawable *source1Drawable;
  GimpDrawable *source2Drawable;
  GimpDrawable *depthMap1Drawable;
  GimpDrawable *depthMap2Drawable;

  g_object_get (config,
                "source-1",    &source1Drawable,
                "source-2",    &source2Drawable,
                "depth-map-1", &depthMap1Drawable,
                "depth-map-2", &depthMap2Drawable,
                NULL);

  dm->previewSource1 = g_new (guchar, dm->previewWidth * dm->previewHeight * 4);
  util_fillReducedBuffer (dm->previewSource1,
                          babl_format ("R'G'B'A u8"),
                          dm->previewWidth,
                          dm->previewHeight,
                          source1Drawable,
                          dm->selectionX, dm->selectionY,
                          dm->selectionWidth, dm->selectionHeight);

  dm->previewSource2 = g_new (guchar, dm->previewWidth * dm->previewHeight * 4);
  util_fillReducedBuffer (dm->previewSource2,
                          babl_format ("R'G'B'A u8"),
                          dm->previewWidth,
                          dm->previewHeight,
                          source2Drawable,
                          dm->selectionX, dm->selectionY,
                          dm->selectionWidth, dm->selectionHeight);

  dm->previewDepthMap1 = g_new (guchar, dm->previewWidth * dm->previewHeight * 1);
  util_fillReducedBuffer (dm->previewDepthMap1,
                          babl_format ("Y' u8"),
                          dm->previewWidth,
                          dm->previewHeight,
                          depthMap1Drawable,
                          dm->selectionX, dm->selectionY,
                          dm->selectionWidth, dm->selectionHeight);

  dm->previewDepthMap2 = g_new (guchar, dm->previewWidth * dm->previewHeight * 1);
  util_fillReducedBuffer (dm->previewDepthMap2,
                          babl_format ("Y' u8"),
                          dm->previewWidth,
                          dm->previewHeight,
                          depthMap2Drawable,
                          dm->selectionX, dm->selectionY,
                          dm->selectionWidth, dm->selectionHeight);

  g_clear_object (&source1Drawable);
  g_clear_object (&source2Drawable);
  g_clear_object (&depthMap1Drawable);
  g_clear_object (&depthMap2Drawable);
}

static void
DepthMerge_updatePreview (Merge               *dm,
                          GimpProcedureConfig *config)
{
  gint    y;
  guchar *source1Row, *source2Row;
  guchar *depthMap1Row, *depthMap2Row;
  guchar *resultRGBA;
  gdouble overlap = 0.0;
  gdouble offset = 0.0;
  gdouble scale1 = 0.0;
  gdouble scale2 = 0.0;

  g_object_get (config,
                "overlap", &overlap,
                "offset",  &offset,
                "scale-1", &scale1,
                "scale-2", &scale2,
                NULL);

  resultRGBA = g_new (guchar, 4 * dm->previewWidth *
                                  dm->previewHeight);

  for (y = 0; y < dm->previewHeight; y++)
    {
      source1Row =
        &(dm->previewSource1[  y * dm->previewWidth * 4]);
      source2Row =
        &(dm->previewSource2[  y * dm->previewWidth * 4]);
      depthMap1Row =
        &(dm->previewDepthMap1[y * dm->previewWidth    ]);
      depthMap2Row =
        &(dm->previewDepthMap2[y * dm->previewWidth    ]);

      DepthMerge_executeRegion (config,
                                source1Row, source2Row, depthMap1Row, depthMap2Row,
                                resultRGBA + 4 * y * dm->previewWidth,
                                dm->previewWidth, overlap, offset, scale1, scale2);
    }

  gimp_preview_area_draw (GIMP_PREVIEW_AREA (dm->preview),
                          0, 0,
                          dm->previewWidth,
                          dm->previewHeight,
                          GIMP_RGBA_IMAGE,
                          resultRGBA,
                          dm->previewWidth * 4);
  gtk_widget_show (dm->preview);
  g_free (resultRGBA);
}


/* ----- Utility routines ----- */

static void
util_fillReducedBuffer (guchar       *dest,
                        const Babl   *dest_format,
                        gint          destWidth,
                        gint          destHeight,
                        GimpDrawable *sourceDrawable,
                        gint          x0,
                        gint          y0,
                        gint          sourceWidth,
                        gint          sourceHeight)
{
  GeglBuffer *buffer;
  guchar     *sourceBuffer,    *reducedRowBuffer;
  guchar     *sourceBufferPos, *reducedRowBufferPos;
  guchar     *sourceBufferRow;
  gint        x, y, i, yPrime;
  gint        destBPP;
  gint       *sourceRowOffsetLookup;

  destBPP = babl_format_get_bytes_per_pixel (dest_format);

  if (! sourceDrawable || (sourceWidth == 0) || (sourceHeight == 0))
    {
      for (x = 0; x < destWidth * destHeight * destBPP; x++)
        dest[x] = 0;

      return;
    }

  sourceBuffer          = g_new (guchar, sourceWidth * sourceHeight * destBPP);
  reducedRowBuffer      = g_new (guchar, destWidth   * destBPP);
  sourceRowOffsetLookup = g_new (int, destWidth);

  buffer = gimp_drawable_get_buffer (sourceDrawable);

  for (x = 0; x < destWidth; x++)
    sourceRowOffsetLookup[x] = (x * (sourceWidth - 1) / (destWidth - 1)) * destBPP;

  gegl_buffer_get (buffer,
                   GEGL_RECTANGLE (x0, y0, sourceWidth, sourceHeight), 1.0,
                   dest_format, sourceBuffer,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  for (y = 0; y < destHeight; y++)
    {
      yPrime = y * (sourceHeight - 1) / (destHeight - 1);
      sourceBufferRow = &(sourceBuffer[yPrime * sourceWidth * destBPP]);
      reducedRowBufferPos = reducedRowBuffer;

      for (x = 0; x < destWidth; x++)
        {
          sourceBufferPos = sourceBufferRow + sourceRowOffsetLookup[x];
          for (i = 0; i < destBPP; i++)
            reducedRowBufferPos[i] = sourceBufferPos[i];
          reducedRowBufferPos += destBPP;
        }

      memcpy (&(dest[y * destWidth * destBPP]), reducedRowBuffer,
              destWidth * destBPP);
    }

  g_object_unref (buffer);

  g_free (sourceBuffer);
  g_free (reducedRowBuffer);
  g_free (sourceRowOffsetLookup);
}


/* Signal handlers */

static void
merge_preview_size_allocate (GtkWidget     *widget,
                             GtkAllocation *allocation,
                             Merge         *dm)
{
  DepthMerge_updatePreview (dm, dm->config);
}

static void
merge_preview_config_notify (GimpProcedureConfig *config,
                             const GParamSpec    *pspec,
                             Merge               *dm)
{
  GimpDrawable *drawable = NULL;
  guchar       *dest     = NULL;
  const Babl   *format   = NULL;

  if (g_strcmp0 (pspec->name, "source-1") == 0)
    {
      g_object_get (config, "source-1", &drawable, NULL);
      dest   = dm->previewSource1;
      format = babl_format ("R'G'B'A u8");
    }
  else if (g_strcmp0 (pspec->name, "source-2") == 0)
    {
      g_object_get (config, "source-2", &drawable, NULL);
      dest   = dm->previewSource2;
      format = babl_format ("R'G'B'A u8");
    }
  else if (g_strcmp0 (pspec->name, "depth-map-1") == 0)
    {
      g_object_get (config, "depth-map-1", &drawable, NULL);
      dest   = dm->previewDepthMap1,
      format = babl_format ("Y' u8");
    }
  else if (g_strcmp0 (pspec->name, "depth-map-2") == 0)
    {
      g_object_get (config, "depth-map-2", &drawable, NULL);
      dest   = dm->previewDepthMap2,
      format = babl_format ("Y' u8");
    }

  if (drawable != NULL)
    util_fillReducedBuffer (dest, format,
                            dm->previewWidth,
                            dm->previewHeight,
                            drawable,
                            dm->selectionX, dm->selectionY,
                            dm->selectionWidth, dm->selectionHeight);
  g_clear_object (&drawable);

  DepthMerge_updatePreview (dm, dm->config);
}
