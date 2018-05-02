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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* WMF loading file filter for GIMP
 * -Dom Lachowicz <cinamod@hotmail.com> 2003
 * -Francis James Franklin <fjf@alinameridon.com>
 */

#include "config.h"

#include <libwmf/api.h>
#include <libwmf/gd.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <libgimpmath/gimpmath.h>

#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC               "file-wmf-load"
#define LOAD_THUMB_PROC         "file-wmf-load-thumb"
#define PLUG_IN_BINARY          "file-wmf"
#define PLUG_IN_ROLE            "gimp-file-wmf"

#define WMF_DEFAULT_RESOLUTION  90.0
#define WMF_DEFAULT_SIZE        500
#define WMF_PREVIEW_SIZE        128

typedef struct
{
  gdouble    resolution;
  gint       width;
  gint       height;
} WmfLoadVals;

static WmfLoadVals load_vals =
{
  WMF_DEFAULT_RESOLUTION,
  0,
  0,
};


static void      query          (void);
static void      run            (const gchar       *name,
                                 gint               nparams,
                                 const GimpParam   *param,
                                 gint              *nreturn_vals,
                                 GimpParam        **return_vals);
static gint32    load_image     (const gchar       *filename,
                                 GError           **error);
static gboolean  load_wmf_size  (const gchar       *filename,
                                 WmfLoadVals       *vals);
static gboolean  load_dialog    (const gchar       *filename);
static guchar   *wmf_get_pixbuf (const gchar       *filename,
                                 gint              *width,
                                 gint              *height);
static guchar   *wmf_load_file  (const gchar       *filename,
                                 guint             *width,
                                 guint             *height,
                                 GError           **error);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,   /* init_proc  */
  NULL,   /* quit_proc  */
  query,  /* query_proc */
  run     /* run_proc   */
};

MAIN ()


/*
 * 'query()' - Respond to a plug-in query...
 */
static void
query (void)
{
  static const GimpParamDef load_args[] =
  {
    { GIMP_PDB_INT32,  "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_STRING, "filename",     "The name of the file to load" },
    { GIMP_PDB_STRING, "raw-filename", "The name of the file to load" },
    { GIMP_PDB_FLOAT,  "resolution",   "Resolution to use for rendering the WMF (defaults to 72 dpi"     },
    { GIMP_PDB_INT32,  "width",        "Width (in pixels) to load the WMF in, 0 for original width"      },
    { GIMP_PDB_INT32,  "height",       "Height (in pixels) to load the WMF in, 0 for original height"    }
  };

  static const GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE,   "image",         "Output image"               }
  };

  static const GimpParamDef thumb_args[] =
  {
    { GIMP_PDB_STRING, "filename",     "The name of the file to load"   },
    { GIMP_PDB_INT32,  "thumb-size",   "Preferred thumbnail size"       }
  };
  static const GimpParamDef thumb_return_vals[] =
  {
    { GIMP_PDB_IMAGE,  "image",        "Thumbnail image"                },
    { GIMP_PDB_INT32,  "image-width",  "Width of full-sized image"      },
    { GIMP_PDB_INT32,  "image-height", "Height of full-sized image"     }
  };

  gimp_install_procedure (LOAD_PROC,
                          "Loads files in the WMF file format",
                          "Loads files in the WMF file format",
                          "Dom Lachowicz <cinamod@hotmail.com>",
                          "Dom Lachowicz <cinamod@hotmail.com>",
                          "(c) 2003 - Version 0.3.0",
                          N_("Microsoft WMF file"),
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args, load_return_vals);

  gimp_register_file_handler_mime (LOAD_PROC, "image/x-wmf");
  gimp_register_magic_load_handler (LOAD_PROC,
                                    "wmf,apm", "",
                                    "0,string,\\327\\315\\306\\232,0,string,\\1\\0\\11\\0");

  gimp_install_procedure (LOAD_THUMB_PROC,
                          "Loads a small preview from a WMF image",
                          "",
                          "Dom Lachowicz <cinamod@hotmail.com>",
                          "Dom Lachowicz <cinamod@hotmail.com>",
                          "(c) 2003 - Version 0.3.0",
                          NULL,
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (thumb_args),
                          G_N_ELEMENTS (thumb_return_vals),
                          thumb_args, thumb_return_vals);

  gimp_register_thumbnail_loader (LOAD_PROC, LOAD_THUMB_PROC);
}

/*
 * 'run()' - Run the plug-in...
 */
static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[4];
  GimpRunMode        run_mode;
  GimpPDBStatusType  status   = GIMP_PDB_SUCCESS;
  const gchar       *filename = NULL;
  GError            *error    = NULL;
  gint32             image_ID = -1;
  gint               width    = 0;
  gint               height   = 0;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (strcmp (name, LOAD_PROC) == 0)
    {
      filename = param[1].data.d_string;

      gimp_get_data (LOAD_PROC, &load_vals);

      switch (run_mode)
        {
        case GIMP_RUN_NONINTERACTIVE:
          if (nparams > 3)  load_vals.resolution = param[3].data.d_float;
          if (nparams > 4)  load_vals.width      = param[4].data.d_int32;
          if (nparams > 5)  load_vals.height     = param[5].data.d_int32;
          break;

        case GIMP_RUN_INTERACTIVE:
          if (!load_dialog (param[1].data.d_string))
            status = GIMP_PDB_CANCEL;
          break;

        case GIMP_RUN_WITH_LAST_VALS:
          break;
        }
    }
  else if (strcmp (name, LOAD_THUMB_PROC) == 0)
    {
      gint size = param[1].data.d_int32;

      filename = param[0].data.d_string;

      if (size > 0                             &&
          load_wmf_size (filename, &load_vals) &&
          load_vals.width  > 0                 &&
          load_vals.height > 0)
        {
          width  = load_vals.width;
          height = load_vals.height;

          if ((gdouble) load_vals.width > (gdouble) load_vals.height)
            {
              load_vals.width   = size;
              load_vals.height *= size / (gdouble) load_vals.width;
            }
          else
            {
              load_vals.width  *= size / (gdouble) load_vals.height;
              load_vals.height  = size;
            }
        }
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      if (load_vals.resolution < GIMP_MIN_RESOLUTION ||
          load_vals.resolution > GIMP_MAX_RESOLUTION)
        {
          load_vals.resolution = WMF_DEFAULT_RESOLUTION;
        }

      image_ID = load_image (filename, &error);

      if (image_ID != -1)
        {
          *nreturn_vals = 2;
          values[1].type         = GIMP_PDB_IMAGE;
          values[1].data.d_image = image_ID;
        }
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      if (strcmp (name, LOAD_THUMB_PROC) == 0)
        {
          *nreturn_vals = 4;
          values[2].type         = GIMP_PDB_INT32;
          values[2].data.d_int32 = width;
          values[3].type         = GIMP_PDB_INT32;
          values[3].data.d_int32 = height;
        }
      else
        {
          gimp_set_data (LOAD_PROC, &load_vals, sizeof (load_vals));
        }
    }

  if (status != GIMP_PDB_SUCCESS && error)
    {
      *nreturn_vals = 2;
      values[1].type          = GIMP_PDB_STRING;
      values[1].data.d_string = error->message;
    }

  values[0].data.d_status = status;
}


static GtkWidget *size_label = NULL;


/*  This function retrieves the pixel size from a WMF file. */
static gboolean
load_wmf_size (const gchar *filename,
               WmfLoadVals *vals)
{
  GMappedFile    *file;
  /* the bits we need to decode the WMF via libwmf2's GD layer  */
  wmf_error_t     err;
  gulong          flags;
  wmf_gd_t       *ddata = NULL;
  wmfAPI         *API   = NULL;
  wmfAPI_Options  api_options;
  wmfD_Rect       bbox;
  guint           width   = -1;
  guint           height  = -1;
  gboolean        success = TRUE;

  file = g_mapped_file_new (filename, FALSE, NULL);
  if (! file)
    return FALSE;

  flags = WMF_OPT_IGNORE_NONFATAL | WMF_OPT_FUNCTION;
  api_options.function = wmf_gd_function;

  err = wmf_api_create (&API, flags, &api_options);
  if (err != wmf_E_None)
    success = FALSE;

  ddata = WMF_GD_GetData (API);
  ddata->type = wmf_gd_image;

  err = wmf_mem_open (API,
                      (guchar *) g_mapped_file_get_contents (file),
                      g_mapped_file_get_length (file));
  if (err != wmf_E_None)
    success = FALSE;

  err = wmf_scan (API, 0, &bbox);
  if (err != wmf_E_None)
    success = FALSE;

  err = wmf_display_size (API, &width, &height,
                          vals->resolution, vals->resolution);
  if (err != wmf_E_None || width <= 0 || height <= 0)
    success = FALSE;

  wmf_mem_close (API);
  g_mapped_file_unref (file);

  if (width < 1 || height < 1)
    {
      width  = WMF_DEFAULT_SIZE;
      height = WMF_DEFAULT_SIZE;

      if (size_label)
        gtk_label_set_text (GTK_LABEL (size_label),
                            _("WMF file does not\nspecify a size!"));
    }
  else
    {
      if (size_label)
        {
          gchar *text = g_strdup_printf (_("%d × %d"), width, height);

          gtk_label_set_text (GTK_LABEL (size_label), text);
          g_free (text);
        }
    }

  vals->width  = width;
  vals->height = height;

  return success;
}


/*  User interface  */

static GimpSizeEntry *size       = NULL;
static GtkAdjustment *xadj       = NULL;
static GtkAdjustment *yadj       = NULL;
static GtkWidget     *constrain  = NULL;
static gdouble        ratio_x    = 1.0;
static gdouble        ratio_y    = 1.0;
static gint           wmf_width  = 0;
static gint           wmf_height = 0;

static void  load_dialog_set_ratio (gdouble x,
                                    gdouble y);


static void
load_dialog_size_callback (GtkWidget *widget,
                           gpointer   data)
{
  if (gimp_chain_button_get_active (GIMP_CHAIN_BUTTON (constrain)))
    {
      gdouble x = gimp_size_entry_get_refval (size, 0) / (gdouble) wmf_width;
      gdouble y = gimp_size_entry_get_refval (size, 1) / (gdouble) wmf_height;

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
                                 const gchar   *filename)
{
  WmfLoadVals  vals = { 0.0, 0, 0 };

  load_vals.resolution = vals.resolution = gimp_size_entry_get_refval (res, 0);

  if (!load_wmf_size (filename, &vals))
    return;

  wmf_width  = vals.width;
  wmf_height = vals.height;

  load_dialog_set_ratio (ratio_x, ratio_y);
}

static void
wmf_preview_callback (GtkWidget     *widget,
                      GtkAllocation *allocation,
                      guchar        *pixels)
{
  gimp_preview_area_draw (GIMP_PREVIEW_AREA (widget),
                          0, 0, allocation->width, allocation->height,
                          GIMP_RGBA_IMAGE,
                          pixels, allocation->width * 4);
}

static void
load_dialog_set_ratio (gdouble x,
                       gdouble y)
{
  ratio_x = x;
  ratio_y = y;

  g_signal_handlers_block_by_func (size, load_dialog_size_callback, NULL);

  gimp_size_entry_set_refval (size, 0, wmf_width  * x);
  gimp_size_entry_set_refval (size, 1, wmf_height * y);

  g_signal_handlers_unblock_by_func (size, load_dialog_size_callback, NULL);

  g_signal_handlers_block_by_func (xadj, load_dialog_ratio_callback, NULL);
  g_signal_handlers_block_by_func (yadj, load_dialog_ratio_callback, NULL);

  gtk_adjustment_set_value (xadj, x);
  gtk_adjustment_set_value (yadj, y);

  g_signal_handlers_unblock_by_func (xadj, load_dialog_ratio_callback, NULL);
  g_signal_handlers_unblock_by_func (yadj, load_dialog_ratio_callback, NULL);
}

static gboolean
load_dialog (const gchar *filename)
{
  GtkWidget     *dialog;
  GtkWidget     *frame;
  GtkWidget     *hbox;
  GtkWidget     *vbox;
  GtkWidget     *image;
  GtkWidget     *table;
  GtkWidget     *table2;
  GtkWidget     *abox;
  GtkWidget     *res;
  GtkWidget     *label;
  GtkWidget     *spinbutton;
  GtkAdjustment *adj;
  guchar        *pixels;
  gboolean       run = FALSE;

  WmfLoadVals  vals = { WMF_DEFAULT_RESOLUTION,
                        - WMF_PREVIEW_SIZE, - WMF_PREVIEW_SIZE };

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (_("Render Windows Metafile"), PLUG_IN_ROLE,
                            NULL, 0,
                            gimp_standard_help_func, LOAD_PROC,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_OK"),     GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
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

  /*  The WMF preview  */
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), abox, FALSE, FALSE, 0);
  gtk_widget_show (abox);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (abox), frame);
  gtk_widget_show (frame);

  pixels = wmf_get_pixbuf (filename, &vals.width, &vals.height);
  image = gimp_preview_area_new ();
  gtk_widget_set_size_request (image, vals.width, vals.height);
  gtk_container_add (GTK_CONTAINER (frame), image);
  gtk_widget_show (image);

  g_signal_connect (image, "size-allocate",
                    G_CALLBACK (wmf_preview_callback),
                    pixels);

  size_label = gtk_label_new (NULL);
  gtk_label_set_justify (GTK_LABEL (size_label), GTK_JUSTIFY_CENTER);
  gtk_box_pack_start (GTK_BOX (vbox), size_label, TRUE, TRUE, 4);
  gtk_widget_show (size_label);

  /*  query the initial size after the size label is created  */
  vals.resolution = load_vals.resolution;

  load_wmf_size (filename, &vals);

  wmf_width  = vals.width;
  wmf_height = vals.height;

  table = gtk_table_new (7, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
  gtk_table_set_row_spacing (GTK_TABLE (table), 2, 2);
  gtk_box_pack_start (GTK_BOX (hbox), table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  /*  Width and Height  */
  label = gtk_label_new (_("Width:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new (_("Height:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, 0, 1,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (hbox);

  adj = (GtkAdjustment *) gtk_adjustment_new (1, 1, 1, 1, 10, 0);
  spinbutton = gtk_spin_button_new (adj, 1.0, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 10);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, 1, 2,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
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

  gimp_size_entry_set_refval (size, 0, wmf_width);
  gimp_size_entry_set_refval (size, 1, wmf_height);

  gimp_size_entry_set_resolution (size, 0, load_vals.resolution, FALSE);
  gimp_size_entry_set_resolution (size, 1, load_vals.resolution, FALSE);

  g_signal_connect (size, "value-changed",
                    G_CALLBACK (load_dialog_size_callback),
                    NULL);

  /*  Scale ratio  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, 2, 4,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (hbox);

  table2 = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table2), 0, 2);
  gtk_table_set_row_spacing (GTK_TABLE (table2), 0, 4);
  gtk_box_pack_start (GTK_BOX (hbox), table2, FALSE, FALSE, 0);

  xadj = (GtkAdjustment *)
    gtk_adjustment_new (ratio_x,
                        (gdouble) GIMP_MIN_IMAGE_SIZE / (gdouble) wmf_width,
                        (gdouble) GIMP_MAX_IMAGE_SIZE / (gdouble) wmf_width,
                        0.01, 0.1, 0);
  spinbutton = gtk_spin_button_new (xadj, 0.01, 4);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 10);
  gtk_table_attach_defaults (GTK_TABLE (table2), spinbutton, 0, 1, 0, 1);
  gtk_widget_show (spinbutton);

  g_signal_connect (xadj, "value-changed",
                    G_CALLBACK (load_dialog_ratio_callback),
                    NULL);

  label = gtk_label_new_with_mnemonic (_("_X ratio:"));
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), spinbutton);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  yadj = (GtkAdjustment *)
    gtk_adjustment_new (ratio_y,
                        (gdouble) GIMP_MIN_IMAGE_SIZE / (gdouble) wmf_height,
                        (gdouble) GIMP_MAX_IMAGE_SIZE / (gdouble) wmf_height,
                        0.01, 0.1, 0);
  spinbutton = gtk_spin_button_new (yadj, 0.01, 4);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 10);
  gtk_table_attach_defaults (GTK_TABLE (table2), spinbutton, 0, 1, 1, 2);
  gtk_widget_show (spinbutton);

  g_signal_connect (yadj, "value-changed",
                    G_CALLBACK (load_dialog_ratio_callback),
                    NULL);

  label = gtk_label_new_with_mnemonic (_("_Y ratio:"));
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), spinbutton);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  /*  the constrain ratio chainbutton  */
  constrain = gimp_chain_button_new (GIMP_CHAIN_RIGHT);
  gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (constrain), TRUE);
  gtk_table_attach_defaults (GTK_TABLE (table2), constrain, 1, 2, 0, 2);
  gtk_widget_show (constrain);

  gimp_help_set_help_data (gimp_chain_button_get_button (GIMP_CHAIN_BUTTON (constrain)),
                           _("Constrain aspect ratio"), NULL);

  gtk_widget_show (table2);

  /*  Resolution   */
  label = gtk_label_new (_("Resolution:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 4, 5,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  res = gimp_size_entry_new (1, GIMP_UNIT_INCH, _("pixels/%a"),
                             FALSE, FALSE, FALSE, 10,
                             GIMP_SIZE_ENTRY_UPDATE_RESOLUTION);

  gtk_table_attach (GTK_TABLE (table), res, 1, 2, 4, 5,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (res);

  /* don't let the resolution become too small ? does libwmf tend to
     crash with very small resolutions */
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (res), 0,
                                         5.0, GIMP_MAX_RESOLUTION);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (res), 0, load_vals.resolution);

  g_signal_connect (res, "value-changed",
                    G_CALLBACK (load_dialog_resolution_callback),
                    (gpointer) filename);

  gtk_widget_show (dialog);

  if (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK)
    {
      load_vals.width  = ROUND (gimp_size_entry_get_refval (size, 0));
      load_vals.height = ROUND (gimp_size_entry_get_refval (size, 1));
      run = TRUE;
    }

  gtk_widget_destroy (GTK_WIDGET (dialog));

  return run;
}

static guchar *
pixbuf_gd_convert (const gint *gd_pixels,
                   gint        width,
                   gint        height)
{
  const gint *gd_ptr = gd_pixels;
  guchar     *pixels;
  guchar     *px_ptr;
  gint        i, j;

  pixels = (guchar *) g_try_malloc (width * height * sizeof (guchar) * 4);
  if (! pixels)
    return NULL;

  px_ptr = pixels;

  for (i = 0; i < height; i++)
    for (j = 0; j < width; j++)
      {
        guchar r, g, b, a;
        guint  pixel = (guint) (*gd_ptr++);

        b = (guchar) (pixel & 0xff);
        pixel >>= 8;
        g = (guchar) (pixel & 0xff);
        pixel >>= 8;
        r = (guchar) (pixel & 0xff);
        pixel >>= 7;
        a = (guchar) (pixel & 0xfe);
        a ^= 0xff;

        *px_ptr++ = r;
        *px_ptr++ = g;
        *px_ptr++ = b;
        *px_ptr++ = a;
      }

  return pixels;
}

static guchar *
wmf_get_pixbuf (const gchar *filename,
                gint        *width,
                gint        *height)
{
  GMappedFile    *file;
  guchar         *pixels   = NULL;

  /* the bits we need to decode the WMF via libwmf2's GD layer  */
  wmf_error_t     err;
  gulong          flags;
  wmf_gd_t       *ddata = NULL;
  wmfAPI         *API   = NULL;
  wmfAPI_Options  api_options;
  guint           file_width;
  guint           file_height;
  wmfD_Rect       bbox;
  gint           *gd_pixels = NULL;

  file = g_mapped_file_new (filename, FALSE, NULL);
  if (! file)
    return NULL;

  flags = WMF_OPT_IGNORE_NONFATAL | WMF_OPT_FUNCTION;
  api_options.function = wmf_gd_function;

  err = wmf_api_create (&API, flags, &api_options);
  if (err != wmf_E_None)
    goto _wmf_error;

  ddata = WMF_GD_GetData (API);
  ddata->type = wmf_gd_image;

  err = wmf_mem_open (API,
                      (guchar *) g_mapped_file_get_contents (file),
                      g_mapped_file_get_length (file));
  if (err != wmf_E_None)
    goto _wmf_error;

  err = wmf_scan (API, 0, &bbox);
  if (err != wmf_E_None)
    goto _wmf_error;

  err = wmf_display_size (API, &file_width, &file_height,
                          WMF_DEFAULT_RESOLUTION, WMF_DEFAULT_RESOLUTION);
  if (err != wmf_E_None || file_width <= 0 || file_height <= 0)
    goto _wmf_error;

  if (!*width || !*height)
    goto _wmf_error;

  /*  either both arguments negative or none  */
  if ((*width * *height) < 0)
    goto _wmf_error;

  ddata->bbox   = bbox;

  if (*width > 0)
    {
      ddata->width  = *width;
      ddata->height = *height;
    }
  else
    {
      gdouble w      = file_width;
      gdouble h      = file_height;
      gdouble aspect = ((gdouble) *width) / (gdouble) *height;

      if (aspect > (w / h))
        {
          ddata->height = abs (*height);
          ddata->width  = (gdouble) abs (*width) * (w / h) + 0.5;
        }
      else
        {
          ddata->width  = abs (*width);
          ddata->height = (gdouble) abs (*height) / (w / h) + 0.5;
        }
    }

  err = wmf_play (API, 0, &bbox);
  if (err != wmf_E_None)
    goto _wmf_error;

  if (ddata->gd_image != NULL)
    gd_pixels = wmf_gd_image_pixels (ddata->gd_image);
  if (gd_pixels == NULL)
    goto _wmf_error;

  pixels = pixbuf_gd_convert (gd_pixels, ddata->width, ddata->height);
  if (pixels == NULL)
    goto _wmf_error;

  *width = ddata->width;
  *height = ddata->height;

 _wmf_error:
  if (API)
    {
      wmf_mem_close (API);
      wmf_api_destroy (API);
    }

  g_mapped_file_unref (file);

  return pixels;
}

static guchar *
wmf_load_file (const gchar  *filename,
               guint        *width,
               guint        *height,
               GError      **error)
{
  GMappedFile    *file;
  guchar         *pixels   = NULL;

  /* the bits we need to decode the WMF via libwmf2's GD layer  */
  wmf_error_t     err;
  gulong          flags;
  wmf_gd_t       *ddata = NULL;
  wmfAPI         *API   = NULL;
  wmfAPI_Options  api_options;
  wmfD_Rect       bbox;
  gint           *gd_pixels = NULL;

  *width = *height = -1;

  file = g_mapped_file_new (filename, FALSE, error);
  if (! file)
    return NULL;

  flags = WMF_OPT_IGNORE_NONFATAL | WMF_OPT_FUNCTION;
  api_options.function = wmf_gd_function;

  err = wmf_api_create (&API, flags, &api_options);
  if (err != wmf_E_None)
    goto _wmf_error;

  ddata = WMF_GD_GetData (API);
  ddata->type = wmf_gd_image;

  err = wmf_mem_open (API,
                      (guchar *) g_mapped_file_get_contents (file),
                      g_mapped_file_get_length (file));
  if (err != wmf_E_None)
    goto _wmf_error;

  err = wmf_scan (API, 0, &bbox);
  if (err != wmf_E_None)
    goto _wmf_error;

  err = wmf_display_size (API,
                          width, height,
                          load_vals.resolution, load_vals.resolution);
  if (err != wmf_E_None || *width <= 0 || *height <= 0)
    goto _wmf_error;

  if (load_vals.width > 0 && load_vals.height > 0)
    {
      *width  = load_vals.width;
      *height = load_vals.height;
    }

  ddata->bbox   = bbox;
  ddata->width  = *width;
  ddata->height = *height;

  err = wmf_play (API, 0, &bbox);
  if (err != wmf_E_None)
    goto _wmf_error;

  if (ddata->gd_image != NULL)
    gd_pixels = wmf_gd_image_pixels (ddata->gd_image);
  if (gd_pixels == NULL)
    goto _wmf_error;

  pixels = pixbuf_gd_convert (gd_pixels, *width, *height);
  if (pixels == NULL)
    goto _wmf_error;

 _wmf_error:
  if (API)
    {
      wmf_mem_close (API);
      wmf_api_destroy (API);
    }

  g_mapped_file_unref (file);

  /* FIXME: improve error message */
  g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
               _("Could not open '%s' for reading"),
               gimp_filename_to_utf8 (filename));

  return pixels;
}

/*
 * 'load_image()' - Load a WMF image into a new image window.
 */
static gint32
load_image (const gchar  *filename,
            GError      **error)
{
  gint32        image;
  gint32        layer;
  GeglBuffer   *buffer;
  guchar       *pixels;
  guint         width, height;

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_filename_to_utf8 (filename));

  pixels = wmf_load_file (filename, &width, &height, error);

  if (! pixels)
    return -1;

  image = gimp_image_new (width, height, GIMP_RGB);
  gimp_image_set_filename (image, filename);
  gimp_image_set_resolution (image,
                             load_vals.resolution, load_vals.resolution);

  layer = gimp_layer_new (image,
                          _("Rendered WMF"),
                          width, height,
                          GIMP_RGBA_IMAGE,
                          100,
                          gimp_image_get_default_new_layer_mode (image));

  buffer = gimp_drawable_get_buffer (layer);

  gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, width, height), 0,
                   babl_format ("R'G'B'A u8"),
                   pixels, GEGL_AUTO_ROWSTRIDE);

  g_object_unref (buffer);

  g_free (pixels);

  gimp_image_insert_layer (image, layer, -1, 0);

  gimp_progress_update (1.0);

  return image;
}
