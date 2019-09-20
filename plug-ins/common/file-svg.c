/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/* SVG loader plug-in
 * (C) Copyright 2003  Dom Lachowicz <cinamod@hotmail.com>
 *
 * Largely rewritten in September 2003 by Sven Neumann <sven@gimp.org>
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <librsvg/rsvg.h>

#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC               "file-svg-load"
#define LOAD_THUMB_PROC         "file-svg-load-thumb"
#define PLUG_IN_BINARY          "file-svg"
#define PLUG_IN_ROLE            "gimp-file-svg"
#define SVG_VERSION             "2.5.0"
#define SVG_DEFAULT_RESOLUTION  90.0
#define SVG_DEFAULT_SIZE        500
#define SVG_PREVIEW_SIZE        128


typedef struct
{
  gdouble    resolution;
  gint       width;
  gint       height;
  gboolean   import;
  gboolean   merge;
} SvgLoadVals;


typedef struct _Svg      Svg;
typedef struct _SvgClass SvgClass;

struct _Svg
{
  GimpPlugIn      parent_instance;
};

struct _SvgClass
{
  GimpPlugInClass parent_class;
};


#define SVG_TYPE  (svg_get_type ())
#define SVG (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SVG_TYPE, Svg))

GType                   svg_get_type         (void) G_GNUC_CONST;

static GList          * svg_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * svg_create_procedure (GimpPlugIn           *plug_in,
                                              const gchar          *name);

static GimpValueArray * svg_load             (GimpProcedure        *procedure,
                                              GimpRunMode           run_mode,
                                              GFile                *file,
                                              const GimpValueArray *args,
                                              gpointer              run_data);
static GimpValueArray * svg_load_thumb       (GimpProcedure        *procedure,
                                              GFile                *file,
                                              gint                  size,
                                              const GimpValueArray *args,
                                              gpointer              run_data);

static GimpImage         * load_image        (GFile                *file,
                                              GError              **error);
static GdkPixbuf         * load_rsvg_pixbuf  (GFile                *file,
                                              SvgLoadVals          *vals,
                                              GError              **error);
static gboolean            load_rsvg_size    (GFile                *file,
                                              SvgLoadVals          *vals,
                                              GError              **error);
static gboolean            load_dialog       (GFile                *file,
                                              GError              **error);



G_DEFINE_TYPE (Svg, svg, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (SVG_TYPE)


static SvgLoadVals load_vals =
{
  SVG_DEFAULT_RESOLUTION,
  0,
  0,
  FALSE,
  FALSE
};


static void
svg_class_init (SvgClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = svg_query_procedures;
  plug_in_class->create_procedure = svg_create_procedure;
}

static void
svg_init (Svg *svg)
{
}

static GList *
svg_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_THUMB_PROC));
  list = g_list_append (list, g_strdup (LOAD_PROC));

  return list;
}

static GimpProcedure *
svg_create_procedure (GimpPlugIn  *plug_in,
                      const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           svg_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, N_("SVG image"));

      gimp_procedure_set_documentation (procedure,
                                        "Loads files in the SVG file format",
                                        "Renders SVG files to raster graphics "
                                        "using librsvg.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Dom Lachowicz, Sven Neumann",
                                      "Dom Lachowicz <cinamod@hotmail.com>",
                                      SVG_VERSION);

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/svg+xml");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "svg");
      gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                      "0,string,<?xml,0,string,<svg");

      gimp_load_procedure_set_thumbnail_loader (GIMP_LOAD_PROCEDURE (procedure),
                                                LOAD_THUMB_PROC);

      GIMP_PROC_ARG_DOUBLE (procedure, "resolution",
                            "Resolution",
                            "Resolution to use for rendering the SVG",
                            GIMP_MIN_RESOLUTION, GIMP_MAX_RESOLUTION, 90,
                            GIMP_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "width",
                         "Width",
                         "Width (in pixels) to load the SVG in. "
                         "(0 for original width, a negative width to "
                         "specify a maximum width)",
                         -GIMP_MAX_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE, 0,
                         GIMP_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "height",
                         "Height",
                         "Height (in pixels) to load the SVG in. "
                         "(0 for original heght, a negative width to "
                         "specify a maximum height)",
                         -GIMP_MAX_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE, 0,
                         GIMP_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "paths",
                         "Paths",
                         "(0) don't import paths, (1) paths individually, "
                         "(2) paths merged",
                         0, 2, 0,
                         GIMP_PARAM_READWRITE);
    }
  else if (! strcmp (name, LOAD_THUMB_PROC))
    {
      procedure = gimp_thumbnail_procedure_new (plug_in, name,
                                                GIMP_PDB_PROC_TYPE_PLUGIN,
                                                svg_load_thumb, NULL, NULL);

      gimp_procedure_set_documentation (procedure,
                                        "Generates a thumbnail of an SVG image",
                                        "Renders a thumbnail of an SVG file "
                                        "using librsvg.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Dom Lachowicz, Sven Neumann",
                                      "Dom Lachowicz <cinamod@hotmail.com>",
                                      SVG_VERSION);
    }

  return procedure;
}

static GimpValueArray *
svg_load (GimpProcedure        *procedure,
          GimpRunMode           run_mode,
          GFile                *file,
          const GimpValueArray *args,
          gpointer              run_data)
{
  GimpValueArray *return_vals;
  GimpImage      *image;
  GError         *error = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  switch (run_mode)
    {
    case GIMP_RUN_NONINTERACTIVE:
      load_vals.resolution = GIMP_VALUES_GET_DOUBLE (args, 0);
      load_vals.width      = GIMP_VALUES_GET_INT    (args, 1);
      load_vals.height     = GIMP_VALUES_GET_INT    (args, 2);
      load_vals.import     = GIMP_VALUES_GET_INT    (args, 3) != FALSE;
      load_vals.merge      = GIMP_VALUES_GET_INT    (args, 3) == 2;
      break;

    case GIMP_RUN_INTERACTIVE:
      gimp_get_data (LOAD_PROC, &load_vals);
      if (! load_dialog (file, &error))
        return gimp_procedure_new_return_values (procedure,
                                                 GIMP_PDB_CANCEL,
                                                 NULL);
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data (LOAD_PROC, &load_vals);
      break;
    }

  image = load_image (file, &error);

  if (! image)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR,
                                             error);
  if (load_vals.import)
    {
      GimpVectors **vectors;
      gint          num_vectors;

      gimp_vectors_import_from_file (image, file,
                                     load_vals.merge, TRUE,
                                     &num_vectors, &vectors);
      if (num_vectors > 0)
        g_free (vectors);
    }

  if (run_mode != GIMP_RUN_NONINTERACTIVE)
    gimp_set_data (LOAD_PROC, &load_vals, sizeof (load_vals));

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static GimpValueArray *
svg_load_thumb (GimpProcedure        *procedure,
                GFile                *file,
                gint                  size,
                const GimpValueArray *args,
                gpointer              run_data)
{
  GimpValueArray *return_vals;
  gint            width  = 0;
  gint            height = 0;
  GimpImage      *image;
  GError         *error = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  if (load_rsvg_size (file, &load_vals, NULL))
    {
      width  = load_vals.width;
      height = load_vals.height;
    }

  load_vals.resolution = SVG_DEFAULT_RESOLUTION;
  load_vals.width      = - size;
  load_vals.height     = - size;

  image = load_image (file, &error);

  if (! image)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_IMAGE (return_vals, 1, image);
  GIMP_VALUES_SET_INT   (return_vals, 2, width);
  GIMP_VALUES_SET_INT   (return_vals, 3, height);

  gimp_value_array_truncate (return_vals, 4);

  return return_vals;
}

static GimpImage *
load_image (GFile   *file,
            GError **load_error)
{
  GimpImage    *image;
  GimpLayer    *layer;
  GdkPixbuf    *pixbuf;
  gint          width;
  gint          height;
  GError       *error = NULL;

  pixbuf = load_rsvg_pixbuf (file, &load_vals, &error);

  if (! pixbuf)
    {
      /*  Do not rely on librsvg setting GError on failure!  */
      g_set_error (load_error,
                   error ? error->domain : 0, error ? error->code : 0,
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file),
                   error ? error->message : _("Unknown reason"));
      g_clear_error (&error);

      return NULL;
    }

  gimp_progress_init (_("Rendering SVG"));

  width  = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);

  image = gimp_image_new (width, height, GIMP_RGB);
  gimp_image_undo_disable (image);

  gimp_image_set_file (image, file);
  gimp_image_set_resolution (image,
                             load_vals.resolution, load_vals.resolution);

  layer = gimp_layer_new_from_pixbuf (image, _("Rendered SVG"), pixbuf,
                                      100,
                                      gimp_image_get_default_new_layer_mode (image),
                                      0.0, 1.0);
  gimp_image_insert_layer (image, layer, NULL, 0);

  gimp_image_undo_enable (image);

  return image;
}

/*  This is the callback used from load_rsvg_pixbuf().  */
static void
load_set_size_callback (gint     *width,
                        gint     *height,
                        gpointer  data)
{
  SvgLoadVals *vals = data;

  if (*width < 1 || *height < 1)
    {
      *width  = SVG_DEFAULT_SIZE;
      *height = SVG_DEFAULT_SIZE;
    }

  if (!vals->width || !vals->height)
    return;

  /*  either both arguments negative or none  */
  if ((vals->width * vals->height) < 0)
    return;

  if (vals->width > 0)
    {
      *width  = vals->width;
      *height = vals->height;
    }
  else
    {
      gdouble w      = *width;
      gdouble h      = *height;
      gdouble aspect = (gdouble) vals->width / (gdouble) vals->height;

      if (aspect > (w / h))
        {
          *height = abs (vals->height);
          *width  = (gdouble) abs (vals->width) * (w / h) + 0.5;
        }
      else
        {
          *width  = abs (vals->width);
          *height = (gdouble) abs (vals->height) / (w / h) + 0.5;
        }

      vals->width  = *width;
      vals->height = *height;
    }
}

/*  This function renders a pixbuf from an SVG file according to vals.  */
static GdkPixbuf *
load_rsvg_pixbuf (GFile        *file,
                  SvgLoadVals  *vals,
                  GError      **error)
{
  GdkPixbuf  *pixbuf  = NULL;
  RsvgHandle *handle;

  handle = rsvg_handle_new_from_gfile_sync (file, RSVG_HANDLE_FLAGS_NONE, NULL,
                                            error);

  if (! handle)
    return NULL;

  rsvg_handle_set_dpi (handle, vals->resolution);
  rsvg_handle_set_size_callback (handle, load_set_size_callback, vals, NULL);

  pixbuf = rsvg_handle_get_pixbuf (handle);

  g_object_unref (handle);

  return pixbuf;
}

static GtkWidget *size_label = NULL;

/*  This function retrieves the pixel size from an SVG file.  */
static gboolean
load_rsvg_size (GFile        *file,
                SvgLoadVals  *vals,
                GError      **error)
{
  RsvgHandle        *handle;
  RsvgDimensionData  dim;
  gboolean           has_size;

  handle = rsvg_handle_new_from_gfile_sync (file, RSVG_HANDLE_FLAGS_NONE, NULL,
                                            error);

  if (! handle)
    return FALSE;

  rsvg_handle_set_dpi (handle, vals->resolution);

  rsvg_handle_get_dimensions (handle, &dim);

  if (dim.width > 0 && dim.height > 0)
    {
      vals->width  = dim.width;
      vals->height = dim.height;
      has_size = TRUE;
    }
  else
    {
      vals->width  = SVG_DEFAULT_SIZE;
      vals->height = SVG_DEFAULT_SIZE;
      has_size = FALSE;
    }

    if (size_label)
      {
        if (has_size)
          {
            gchar *text = g_strdup_printf (_("%d × %d"),
                                           vals->width, vals->height);
            gtk_label_set_text (GTK_LABEL (size_label), text);
            g_free (text);
          }
        else
          {
            gtk_label_set_text (GTK_LABEL (size_label),
                                _("SVG file does not\nspecify a size!"));
          }
      }

  g_object_unref (handle);

  if (vals->width  < 1)  vals->width  = 1;
  if (vals->height < 1)  vals->height = 1;

  return TRUE;
}


/*  User interface  */

static GimpSizeEntry *size       = NULL;
static GtkAdjustment *xadj       = NULL;
static GtkAdjustment *yadj       = NULL;
static GtkWidget     *constrain  = NULL;
static gdouble        ratio_x    = 1.0;
static gdouble        ratio_y    = 1.0;
static gint           svg_width  = 0;
static gint           svg_height = 0;

static void  load_dialog_set_ratio (gdouble x,
                                    gdouble y);


static void
load_dialog_size_callback (GtkWidget *widget,
                           gpointer   data)
{
  if (gimp_chain_button_get_active (GIMP_CHAIN_BUTTON (constrain)))
    {
      gdouble x = gimp_size_entry_get_refval (size, 0) / (gdouble) svg_width;
      gdouble y = gimp_size_entry_get_refval (size, 1) / (gdouble) svg_height;

      if (x != ratio_x)
        {
          load_dialog_set_ratio (x, x);
        }
      else if (y != ratio_y)
        {
          load_dialog_set_ratio (y, y);
        }
    }
}

static void
load_dialog_ratio_callback (GtkAdjustment *adj,
                            gpointer       data)
{
  gdouble x = gtk_adjustment_get_value (xadj);
  gdouble y = gtk_adjustment_get_value (yadj);

  if (gimp_chain_button_get_active (GIMP_CHAIN_BUTTON (constrain)))
    {
      if (x != ratio_x)
        y = x;
      else
        x = y;
    }

  load_dialog_set_ratio (x, y);
}

static void
load_dialog_resolution_callback (GimpSizeEntry *res,
                                 GFile         *file)
{
  SvgLoadVals  vals = { 0.0, 0, 0 };

  load_vals.resolution = vals.resolution = gimp_size_entry_get_refval (res, 0);

  if (! load_rsvg_size (file, &vals, NULL))
    return;

  g_signal_handlers_block_by_func (size, load_dialog_size_callback, NULL);

  gimp_size_entry_set_resolution (size, 0, load_vals.resolution, FALSE);
  gimp_size_entry_set_resolution (size, 1, load_vals.resolution, FALSE);

  g_signal_handlers_unblock_by_func (size, load_dialog_size_callback, NULL);

  if (gimp_size_entry_get_unit (size) != GIMP_UNIT_PIXEL)
    {
      ratio_x = gimp_size_entry_get_refval (size, 0) / vals.width;
      ratio_y = gimp_size_entry_get_refval (size, 1) / vals.height;
    }

  svg_width  = vals.width;
  svg_height = vals.height;

  load_dialog_set_ratio (ratio_x, ratio_y);
}

static void
load_dialog_set_ratio (gdouble x,
                       gdouble y)
{
  ratio_x = x;
  ratio_y = y;

  g_signal_handlers_block_by_func (size, load_dialog_size_callback, NULL);

  gimp_size_entry_set_refval (size, 0, svg_width  * x);
  gimp_size_entry_set_refval (size, 1, svg_height * y);

  g_signal_handlers_unblock_by_func (size, load_dialog_size_callback, NULL);

  g_signal_handlers_block_by_func (xadj, load_dialog_ratio_callback, NULL);
  g_signal_handlers_block_by_func (yadj, load_dialog_ratio_callback, NULL);

  gtk_adjustment_set_value (xadj, x);
  gtk_adjustment_set_value (yadj, y);

  g_signal_handlers_unblock_by_func (xadj, load_dialog_ratio_callback, NULL);
  g_signal_handlers_unblock_by_func (yadj, load_dialog_ratio_callback, NULL);
}

static gboolean
load_dialog (GFile   *file,
             GError **load_error)
{
  GtkWidget     *dialog;
  GtkWidget     *frame;
  GtkWidget     *hbox;
  GtkWidget     *vbox;
  GtkWidget     *image;
  GdkPixbuf     *preview;
  GtkWidget     *grid;
  GtkWidget     *grid2;
  GtkWidget     *res;
  GtkWidget     *label;
  GtkWidget     *spinbutton;
  GtkWidget     *toggle;
  GtkWidget     *toggle2;
  GtkAdjustment *adj;
  gboolean       run;
  GError        *error = NULL;
  SvgLoadVals    vals  =
  {
    SVG_DEFAULT_RESOLUTION,
    - SVG_PREVIEW_SIZE,
    - SVG_PREVIEW_SIZE
  };

  preview = load_rsvg_pixbuf (file, &vals, &error);

  if (! preview)
    {
      /*  Do not rely on librsvg setting GError on failure!  */
      g_set_error (load_error,
                   error ? error->domain : 0, error ? error->code : 0,
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file),
                   error ? error->message : _("Unknown reason"));
      g_clear_error (&error);

      return GIMP_PDB_EXECUTION_ERROR;
    }

  gimp_ui_init (PLUG_IN_BINARY);

  /* Scalable Vector Graphics is SVG, should perhaps not be translated */
  dialog = gimp_dialog_new (_("Render Scalable Vector Graphics"),
                            PLUG_IN_ROLE,
                            NULL, 0,
                            gimp_standard_help_func, LOAD_PROC,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_OK"),     GTK_RESPONSE_OK,

                            NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  /*  The SVG preview  */
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  image = gtk_image_new_from_pixbuf (preview);
  gtk_container_add (GTK_CONTAINER (frame), image);
  gtk_widget_show (image);

  size_label = gtk_label_new (NULL);
  gtk_label_set_justify (GTK_LABEL (size_label), GTK_JUSTIFY_CENTER);
  gtk_box_pack_start (GTK_BOX (vbox), size_label, TRUE, TRUE, 4);
  gtk_widget_show (size_label);

  /*  query the initial size after the size label is created  */
  vals.resolution = load_vals.resolution;

  load_rsvg_size (file, &vals, NULL);

  svg_width  = vals.width;
  svg_height = vals.height;

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (hbox), grid, TRUE, TRUE, 0);
  gtk_widget_show (grid);

  /*  Width and Height  */
  label = gtk_label_new (_("Width:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);
                    // GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new (_("Height:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 1, 1);
                    // GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_grid_attach (GTK_GRID (grid), hbox, 1, 0, 1, 1);
                    // GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (hbox);

  adj = gtk_adjustment_new (1, 1, 1, 1, 10, 0);
  spinbutton = gimp_spin_button_new (adj, 1.0, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 10);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_grid_attach (GTK_GRID (grid), hbox, 1, 1, 1, 1);
                    // GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (hbox);

  size = GIMP_SIZE_ENTRY (gimp_size_entry_new (1, GIMP_UNIT_PIXEL, "%a",
                                               TRUE, FALSE, FALSE, 10,
                                               GIMP_SIZE_ENTRY_UPDATE_SIZE));

  gimp_size_entry_add_field (size, GTK_SPIN_BUTTON (spinbutton), NULL);

  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (size), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (size));

  gimp_size_entry_set_refval_boundaries (size, 0,
                                         GIMP_MIN_IMAGE_SIZE,
                                         GIMP_MAX_IMAGE_SIZE);
  gimp_size_entry_set_refval_boundaries (size, 1,
                                         GIMP_MIN_IMAGE_SIZE,
                                         GIMP_MAX_IMAGE_SIZE);

  gimp_size_entry_set_refval (size, 0, svg_width);
  gimp_size_entry_set_refval (size, 1, svg_height);

  gimp_size_entry_set_resolution (size, 0, load_vals.resolution, FALSE);
  gimp_size_entry_set_resolution (size, 1, load_vals.resolution, FALSE);

  g_signal_connect (size, "value-changed",
                    G_CALLBACK (load_dialog_size_callback),
                    NULL);

  /*  Scale ratio  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_grid_attach (GTK_GRID (grid), hbox, 1, 2, 1, 2);
                    // GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (hbox);

  grid2 = gtk_grid_new ();
  gtk_box_pack_start (GTK_BOX (hbox), grid2, FALSE, FALSE, 0);

  xadj = gtk_adjustment_new (ratio_x,
                             (gdouble) GIMP_MIN_IMAGE_SIZE / (gdouble) svg_width,
                             (gdouble) GIMP_MAX_IMAGE_SIZE / (gdouble) svg_width,
                             0.01, 0.1, 0);
  spinbutton = gimp_spin_button_new (xadj, 0.01, 4);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 10);
  gtk_grid_attach (GTK_GRID (grid2), spinbutton, 0, 0, 1, 1);
  gtk_widget_show (spinbutton);

  g_signal_connect (xadj, "value-changed",
                    G_CALLBACK (load_dialog_ratio_callback),
                    NULL);

  label = gtk_label_new_with_mnemonic (_("_X ratio:"));
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), spinbutton);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 2, 1, 1);
                    // GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  yadj = gtk_adjustment_new (ratio_y,
                             (gdouble) GIMP_MIN_IMAGE_SIZE / (gdouble) svg_height,
                             (gdouble) GIMP_MAX_IMAGE_SIZE / (gdouble) svg_height,
                             0.01, 0.1, 0);
  spinbutton = gimp_spin_button_new (yadj, 0.01, 4);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 10);
  gtk_grid_attach (GTK_GRID (grid2), spinbutton, 0, 1, 1, 1);
  gtk_widget_show (spinbutton);

  g_signal_connect (yadj, "value-changed",
                    G_CALLBACK (load_dialog_ratio_callback),
                    NULL);

  label = gtk_label_new_with_mnemonic (_("_Y ratio:"));
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), spinbutton);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 3, 1, 1);
                    // GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  /*  the constrain ratio chainbutton  */
  constrain = gimp_chain_button_new (GIMP_CHAIN_RIGHT);
  gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (constrain), TRUE);
  gtk_grid_attach (GTK_GRID (grid2), constrain, 1, 0, 1, 2);
  gtk_widget_show (constrain);

  gimp_help_set_help_data (gimp_chain_button_get_button (GIMP_CHAIN_BUTTON (constrain)),
                           _("Constrain aspect ratio"), NULL);

  gtk_widget_show (grid2);

  /*  Resolution   */
  label = gtk_label_new (_("Resolution:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 4, 1, 1);
                    // GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  res = gimp_size_entry_new (1, GIMP_UNIT_INCH, _("pixels/%a"),
                             FALSE, FALSE, FALSE, 10,
                             GIMP_SIZE_ENTRY_UPDATE_RESOLUTION);

  gtk_grid_attach (GTK_GRID (grid), res, 1, 4, 1, 1);
                    // GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (res);

  /* don't let the resolution become too small, librsvg tends to
     crash with very small resolutions */
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (res), 0,
                                         5.0, GIMP_MAX_RESOLUTION);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (res), 0, load_vals.resolution);

  g_signal_connect (res, "value-changed",
                    G_CALLBACK (load_dialog_resolution_callback),
                    file);

  /*  Path Import  */
  toggle = gtk_check_button_new_with_mnemonic (_("Import _paths"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), load_vals.import);
  gtk_grid_attach (GTK_GRID (grid), toggle, 0, 5, 2, 1);
                    // GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (toggle);

  gimp_help_set_help_data (toggle,
                           _("Import path elements of the SVG so they "
                             "can be used with the GIMP path tool"),
                           NULL);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &load_vals.import);

  toggle2 = gtk_check_button_new_with_mnemonic (_("Merge imported paths"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle2), load_vals.merge);
  gtk_grid_attach (GTK_GRID (grid), toggle2, 0, 6, 2, 1);
                    // GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (toggle2);

  g_signal_connect (toggle2, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &load_vals.merge);

  g_object_bind_property (toggle,  "active",
                          toggle2, "sensitive",
                          G_BINDING_SYNC_CREATE);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  if (run)
    {
      load_vals.width  = ROUND (gimp_size_entry_get_refval (size, 0));
      load_vals.height = ROUND (gimp_size_entry_get_refval (size, 1));
    }

  gtk_widget_destroy (dialog);

  return run;
}
