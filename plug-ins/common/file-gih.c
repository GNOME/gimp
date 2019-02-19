/* Plug-in to load and export .gih (GIMP Brush Pipe) files.
 *
 * Copyright (C) 1999 Tor Lillqvist
 * Copyright (C) 2000 Jens Lautenbacher, Sven Neumann
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

  /* Example of how to call file_gih_save from script-fu:

  (let ((ranks (cons-array 1 'byte)))
    (aset ranks 0 12)
    (file-gih-save 1
                   img
                   drawable
                   "foo.gih"
                   "foo.gih"
                   100
                   "test brush"
                   125
                   125
                   3
                   4
                   1
                   ranks
                   1
                   '("random")))
  */


#include "config.h"

#include <stdlib.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <libgimpbase/gimpparasiteio.h>

#include "app/core/gimpbrush-header.h"
#include "app/core/gimppattern-header.h"

#include "libgimp/stdplugins-intl.h"


#define SAVE_PROC      "file-gih-save"
#define PLUG_IN_BINARY "file-gih"
#define PLUG_IN_ROLE   "gimp-file-gih"


/* Parameters applicable each time we export a gih, exported in the
 * main gimp application between invocations of this plug-in.
 */
typedef struct
{
  gchar description[256];
  gint  spacing;
} BrushInfo;

typedef struct
{
  GimpOrientationType  orientation;
  gint32               image;
  gint32               toplayer;
  gint                 nguides;
  gint32              *guides;
  gint                *value;
  GtkWidget           *count_label;    /* Corresponding count adjustment, */
  gint                *count;          /* cols or rows                    */
  gint                *other_count;    /* and the other one               */
  GtkAdjustment       *ncells;
  GtkAdjustment       *rank0;
  GtkWidget           *warning_label;
  GtkWidget           *rank_entry[GIMP_PIXPIPE_MAXDIM];
  GtkWidget           *mode_entry[GIMP_PIXPIPE_MAXDIM];
} SizeAdjustmentData;


/*  local function prototypes  */

static void      query               (void);
static void      run                 (const gchar      *name,
                                      gint              nparams,
                                      const GimpParam  *param,
                                      gint             *nreturn_vals,
                                      GimpParam       **return_vals);

static gboolean  gih_save_dialog     (gint32            image_ID);
static gboolean  gih_save_one_brush  (GOutputStream    *output,
                                      gint32            drawable_ID,
                                      GeglRectangle    *rect,
                                      const gchar      *name,
                                      GError          **error);
static gboolean  gih_save_image      (GFile            *file,
                                      gint32            image_ID,
                                      gint32            orig_image_ID,
                                      gint32            drawable_ID,
                                      GError          **error);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};


/*  private variables  */

static BrushInfo info =
{
  "GIMP Brush Pipe",
  20
};

static gint              num_layers = 0;
static GimpPixPipeParams gihparams  = { 0, };

static const gchar * const selection_modes[] = { "incremental",
                                                 "angular",
                                                 "random",
                                                 "velocity",
                                                 "pressure",
                                                 "xtilt",
                                                 "ytilt" };


MAIN ()

static void
query (void)
{
  static const GimpParamDef gih_save_args[] =
  {
    { GIMP_PDB_INT32,       "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,       "image",        "Input image" },
    { GIMP_PDB_DRAWABLE,    "drawable",     "Drawable to export" },
    { GIMP_PDB_STRING,      "uri",          "The URI of the file to export the brush pipe in" },
    { GIMP_PDB_STRING,      "raw-uri",      "The URI of the file to export the brush pipe in" },
    { GIMP_PDB_INT32,       "spacing",      "Spacing of the brush" },
    { GIMP_PDB_STRING,      "description",  "Short description of the brush pipe" },
    { GIMP_PDB_INT32,       "cell-width",   "Width of the brush cells" },
    { GIMP_PDB_INT32,       "cell-height",  "Width of the brush cells" },
    { GIMP_PDB_INT8,        "display-cols", "Display column number" },
    { GIMP_PDB_INT8,        "display-rows", "Display row number" },
    { GIMP_PDB_INT32,       "dimension",    "Dimension of the brush pipe" },
    /* The number of rank and sel args depend on the dimension */
    { GIMP_PDB_INT8ARRAY,   "rank",         "Ranks of the dimensions" },
    { GIMP_PDB_INT32,       "dimension",    "Dimension (again)" },
    { GIMP_PDB_STRINGARRAY, "sel",          "Selection modes" }
  };

  gimp_install_procedure (SAVE_PROC,
                          "exports images in GIMP brush pipe format",
                          "This plug-in exports an image in the GIMP brush pipe "
                          "format. For a colored brush pipe, RGBA layers are "
                          "used, otherwise the layers should be grayscale "
                          "masks. The image can be multi-layered, and "
                          "additionally the layers can be divided into a "
                          "rectangular array of brushes.",
                          "Tor Lillqvist",
                          "Tor Lillqvist",
                          "1999",
                          N_("GIMP brush (animated)"),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (gih_save_args), 0,
                          gih_save_args, NULL);

  gimp_plugin_icon_register (SAVE_PROC, GIMP_ICON_TYPE_ICON_NAME,
                             (const guint8 *) GIMP_ICON_BRUSH);
  gimp_register_file_handler_mime (SAVE_PROC, "image/x-gimp-gih");
  gimp_register_file_handler_uri (SAVE_PROC);
  gimp_register_save_handler (SAVE_PROC, "gih", "");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[2];
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  gint32             image_ID;
  gint32             drawable_ID;
  gint               i;
  GimpExportReturn   export = GIMP_EXPORT_CANCEL;
  GError            *error  = NULL;

  INIT_I18N();
  gegl_init (NULL, NULL);

  run_mode = param[0].data.d_int32;

  *return_vals  = values;
  *nreturn_vals = 1;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (strcmp (name, SAVE_PROC) == 0)
    {
      GFile        *file;
      GimpParasite *name_parasite;
      GimpParasite *pipe_parasite;
      gint32        orig_image_ID;

      image_ID    = param[1].data.d_int32;
      drawable_ID = param[2].data.d_int32;
      file        = g_file_new_for_uri (param[3].data.d_string);

      orig_image_ID = image_ID;

      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
        case GIMP_RUN_WITH_LAST_VALS:
          gimp_ui_init (PLUG_IN_BINARY, FALSE);

          export = gimp_export_image (&image_ID, &drawable_ID, "GIH",
                                      GIMP_EXPORT_CAN_HANDLE_RGB   |
                                      GIMP_EXPORT_CAN_HANDLE_GRAY  |
                                      GIMP_EXPORT_CAN_HANDLE_ALPHA |
                                      GIMP_EXPORT_CAN_HANDLE_LAYERS);

          if (export == GIMP_EXPORT_CANCEL)
            {
              values[0].data.d_status = GIMP_PDB_CANCEL;
              return;
            }

          /*  Possibly retrieve data  */
          gimp_get_data (SAVE_PROC, &info);

          name_parasite = gimp_image_get_parasite (orig_image_ID,
                                                   "gimp-brush-pipe-name");
          if (name_parasite)
            {
              strncpy (info.description,
                       gimp_parasite_data (name_parasite),
                       MIN (sizeof (info.description),
                            gimp_parasite_data_size (name_parasite)));
              info.description[sizeof (info.description) - 1] = '\0';

              gimp_parasite_free (name_parasite);
            }
          else
            {
              gchar *name = g_path_get_basename (gimp_file_get_utf8_name (file));

              if (g_str_has_suffix (name, ".gih"))
                name[strlen (name) - 4] = '\0';

              if (strlen (name))
                {
                  strncpy (info.description, name, sizeof (info.description));
                  info.description[sizeof (info.description) - 1] = '\0';
                }

              g_free (name);
            }
          break;

        default:
          break;
        }

      g_free (gimp_image_get_layers (image_ID, &num_layers));

      gimp_pixpipe_params_init (&gihparams);

      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
          gihparams.ncells = (num_layers * gihparams.rows * gihparams.cols);

          gihparams.cellwidth  = gimp_image_width (image_ID)  / gihparams.cols;
          gihparams.cellheight = gimp_image_height (image_ID) / gihparams.rows;

          pipe_parasite = gimp_image_get_parasite (orig_image_ID,
                                                   "gimp-brush-pipe-parameters");
          if (pipe_parasite)
            {
              gimp_pixpipe_params_parse (gimp_parasite_data (pipe_parasite),
                                         &gihparams);
              gimp_parasite_free (pipe_parasite);
            }

          /* Force default rank to same as number of cells if there is
           * just one dim
           */
          if (gihparams.dim == 1)
            gihparams.rank[0] = gihparams.ncells;

          if (! gih_save_dialog (image_ID))
            status = GIMP_PDB_CANCEL;
          break;

        case GIMP_RUN_NONINTERACTIVE:
          if (nparams != 15)
            {
              status = GIMP_PDB_CALLING_ERROR;
            }
          else
            {
              info.spacing = param[5].data.d_int32;
              strncpy (info.description, param[6].data.d_string,
                       sizeof (info.description));
              info.description[sizeof (info.description) - 1] = '\0';

              gihparams.cellwidth  = param[7].data.d_int32;
              gihparams.cellheight = param[8].data.d_int32;
              gihparams.cols       = param[9].data.d_int8;
              gihparams.rows       = param[10].data.d_int8;
              gihparams.dim        = param[11].data.d_int32;
              gihparams.ncells     = 1;

              if (param[13].data.d_int32 != gihparams.dim)
                {
                  status = GIMP_PDB_CALLING_ERROR;
                }
              else
                {
                  for (i = 0; i < gihparams.dim; i++)
                    {
                      gihparams.rank[i]      = param[12].data.d_int8array[i];
                      gihparams.selection[i] = g_strdup (param[14].data.d_stringarray[i]);
                      gihparams.ncells       *= gihparams.rank[i];
                    }
                }
            }
          break;

        case GIMP_RUN_WITH_LAST_VALS:
          pipe_parasite = gimp_image_get_parasite (orig_image_ID,
                                                   "gimp-brush-pipe-parameters");
          if (pipe_parasite)
            {
              gimp_pixpipe_params_parse (gimp_parasite_data (pipe_parasite),
                                         &gihparams);
              gimp_parasite_free (pipe_parasite);
            }
          break;
        }

      if (status == GIMP_PDB_SUCCESS)
        {
          if (gih_save_image (file, image_ID, orig_image_ID, drawable_ID,
                              &error))
            {
              gimp_set_data (SAVE_PROC, &info, sizeof (info));
            }
          else
            {
              status = GIMP_PDB_EXECUTION_ERROR;
            }
        }

      gimp_pixpipe_params_free (&gihparams);

      if (export == GIMP_EXPORT_EXPORT)
        gimp_image_delete (image_ID);
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  if (status != GIMP_PDB_SUCCESS && error)
    {
      *nreturn_vals = 2;
      values[1].type          = GIMP_PDB_STRING;
      values[1].data.d_string = error->message;
    }

  values[0].data.d_status = status;
}


/*  save routines */

static void
size_adjustment_callback (GtkWidget          *widget,
                          SizeAdjustmentData *adj)
{
  gint  i;
  gint  size;
  gint  newn;
  gchar buf[10];

  for (i = 0; i < adj->nguides; i++)
    gimp_image_delete_guide (adj->image, adj->guides[i]);

  g_free (adj->guides);
  adj->guides = NULL;
  gimp_displays_flush ();

  *(adj->value) = gtk_adjustment_get_value (GTK_ADJUSTMENT (widget));

  if (adj->orientation == GIMP_ORIENTATION_VERTICAL)
    {
      size = gimp_image_width (adj->image);
      newn = size / *(adj->value);
      adj->nguides = newn - 1;
      adj->guides = g_new (gint32, adj->nguides);
      for (i = 0; i < adj->nguides; i++)
        adj->guides[i] = gimp_image_add_vguide (adj->image,
                                                *(adj->value) * (i+1));
    }
  else
    {
      size = gimp_image_height (adj->image);
      newn = size / *(adj->value);
      adj->nguides = newn - 1;
      adj->guides = g_new (gint32, adj->nguides);
      for (i = 0; i < adj->nguides; i++)
        adj->guides[i] = gimp_image_add_hguide (adj->image,
                                                *(adj->value) * (i+1));
    }
  gimp_displays_flush ();
  g_snprintf (buf, sizeof (buf), "%2d", newn);
  gtk_label_set_text (GTK_LABEL (adj->count_label), buf);

  *(adj->count) = newn;

  gtk_widget_set_visible (GTK_WIDGET (adj->warning_label),
                          newn * *(adj->value) != size);

  if (adj->ncells != NULL)
    gtk_adjustment_set_value (GTK_ADJUSTMENT (adj->ncells),
                              *(adj->other_count) * *(adj->count));
  if (adj->rank0 != NULL)
    gtk_adjustment_set_value (GTK_ADJUSTMENT (adj->rank0),
                              *(adj->other_count) * *(adj->count));
}

static void
entry_callback (GtkWidget *widget,
                gpointer   data)
{
  if (data == info.description)
    {
      strncpy (info.description, gtk_entry_get_text (GTK_ENTRY (widget)),
               sizeof (info.description));
      info.description[sizeof (info.description) - 1]  = 0;
    }
}

static void
cb_callback (GtkWidget *widget,
             gpointer   data)
{
  gint index;

  index = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));

  *((const gchar **) data) = selection_modes [index];
}

static void
dim_callback (GtkAdjustment      *adjustment,
              SizeAdjustmentData *data)
{
  gint i;

  gihparams.dim = RINT (gtk_adjustment_get_value (adjustment));

  for (i = 0; i < GIMP_PIXPIPE_MAXDIM; i++)
    {
      gtk_widget_set_sensitive (data->rank_entry[i], i < gihparams.dim);
      gtk_widget_set_sensitive (data->mode_entry[i], i < gihparams.dim);
    }
}

static gboolean
gih_save_dialog (gint32 image_ID)
{
  GtkWidget          *dialog;
  GtkWidget          *table;
  GtkWidget          *dimtable;
  GtkWidget          *label;
  GtkAdjustment      *adjustment;
  GtkWidget          *spinbutton;
  GtkWidget          *entry;
  GtkWidget          *box;
  GtkWidget          *cb;
  gint                i;
  gchar               buffer[100];
  SizeAdjustmentData  cellw_adjust;
  SizeAdjustmentData  cellh_adjust;
  gint32             *layer_ID;
  gint32              nlayers;
  gboolean            run;

  dialog = gimp_export_dialog_new (_("Brush Pipe"), PLUG_IN_BINARY, SAVE_PROC);

  /* The main table */
  table = gtk_table_new (8, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 12);
  gtk_box_pack_start (GTK_BOX (gimp_export_dialog_get_content_area (dialog)),
                      table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  /*
   * Description: ___________
   */
  entry = gtk_entry_new ();
  gtk_widget_set_size_request (entry, 200, -1);
  gtk_entry_set_text (GTK_ENTRY (entry), info.description);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("Description:"), 0.0, 0.5,
                             entry, 1, FALSE);

  g_signal_connect (entry, "changed",
                    G_CALLBACK (entry_callback),
                    info.description);

  /*
   * Spacing: __
   */
  adjustment = (GtkAdjustment *) gtk_adjustment_new (info.spacing,
                                                     1, 1000, 1, 10, 0);
  spinbutton = gtk_spin_button_new (adjustment, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                             _("Spacing (percent):"), 0.0, 0.5,
                             spinbutton, 1, TRUE);

  g_signal_connect (adjustment, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &info.spacing);

  /*
   * Cell size: __ x __ pixels
   */
  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);

  adjustment = (GtkAdjustment *)
    gtk_adjustment_new (gihparams.cellwidth,
                        2, gimp_image_width (image_ID), 1, 10, 0);
  spinbutton = gtk_spin_button_new (adjustment, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_box_pack_start (GTK_BOX (box), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  layer_ID = gimp_image_get_layers (image_ID, &nlayers);
  cellw_adjust.orientation = GIMP_ORIENTATION_VERTICAL;
  cellw_adjust.image       = image_ID;
  cellw_adjust.toplayer    = layer_ID[nlayers-1];
  cellw_adjust.nguides     = 0;
  cellw_adjust.guides      = NULL;
  cellw_adjust.value       = &gihparams.cellwidth;

  g_signal_connect (adjustment, "value-changed",
                    G_CALLBACK (size_adjustment_callback),
                    &cellw_adjust);

  label = gtk_label_new ("x");
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  adjustment = (GtkAdjustment *)
    gtk_adjustment_new (gihparams.cellheight,
                        2, gimp_image_height (image_ID), 1, 10, 0);
  spinbutton = gtk_spin_button_new (adjustment, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_box_pack_start (GTK_BOX (box), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  cellh_adjust.orientation = GIMP_ORIENTATION_HORIZONTAL;
  cellh_adjust.image       = image_ID;
  cellh_adjust.toplayer    = layer_ID[nlayers-1];
  cellh_adjust.nguides     = 0;
  cellh_adjust.guides      = NULL;
  cellh_adjust.value       = &gihparams.cellheight;

  g_signal_connect (adjustment, "value-changed",
                    G_CALLBACK (size_adjustment_callback),
                    &cellh_adjust);

  label = gtk_label_new ( _("Pixels"));
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
                             _("Cell size:"), 0.0, 0.5,
                             box, 1, FALSE);

  g_free (layer_ID);

  /*
   * Number of cells: ___
   */
  adjustment = (GtkAdjustment *)
    gtk_adjustment_new (gihparams.ncells, 1, 1000, 1, 10, 0);
  spinbutton = gtk_spin_button_new (adjustment, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 3,
                             _("Number of cells:"), 0.0, 0.5,
                             spinbutton, 1, TRUE);

  g_signal_connect (adjustment, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &gihparams.ncells);

  if (gihparams.dim == 1)
    cellw_adjust.ncells = cellh_adjust.ncells = adjustment;
  else
    cellw_adjust.ncells = cellh_adjust.ncells = NULL;

  /*
   * Display as: __ rows x __ cols
   */
  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

  g_snprintf (buffer, sizeof (buffer), "%2d", gihparams.rows);
  label = gtk_label_new (buffer);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  cellh_adjust.count_label = label;
  cellh_adjust.count       = &gihparams.rows;
  cellh_adjust.other_count = &gihparams.cols;
  gtk_widget_show (label);

  label = gtk_label_new (_(" Rows of "));
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  g_snprintf (buffer, sizeof (buffer), "%2d", gihparams.cols);
  label = gtk_label_new (buffer);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  cellw_adjust.count_label = label;
  cellw_adjust.count       = &gihparams.cols;
  cellw_adjust.other_count = &gihparams.rows;
  gtk_widget_show (label);

  label = gtk_label_new (_(" Columns on each layer"));
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  label = gtk_label_new (_(" (Width Mismatch!) "));
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  cellw_adjust.warning_label = label;

  label = gtk_label_new (_(" (Height Mismatch!) "));
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  cellh_adjust.warning_label = label;

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 4,
                             _("Display as:"), 0.0, 0.5,
                             box, 1, FALSE);

  /*
   * Dimension: ___
   */
  adjustment = (GtkAdjustment *)
    gtk_adjustment_new (gihparams.dim, 1, GIMP_PIXPIPE_MAXDIM, 1, 1, 0);
  spinbutton = gtk_spin_button_new (adjustment, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 5,
                             _("Dimension:"), 0.0, 0.5,
                             spinbutton, 1, TRUE);

  g_signal_connect (adjustment, "value-changed",
                    G_CALLBACK (dim_callback),
                    &cellw_adjust);

  /*
   * Ranks / Selection: ______ ______ ______ ______ ______
   */

  dimtable = gtk_table_new (2, GIMP_PIXPIPE_MAXDIM, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (dimtable), 4);
  for (i = 0; i < GIMP_PIXPIPE_MAXDIM; i++)
    {
      gint j;

      adjustment = (GtkAdjustment *)
        gtk_adjustment_new (gihparams.rank[i], 1, 100, 1, 1, 0);
      spinbutton = gtk_spin_button_new (adjustment, 1.0, 0);
      gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
      gtk_table_attach (GTK_TABLE (dimtable), spinbutton, 0, 1, i, i + 1,
                        GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

      gtk_widget_show (spinbutton);

      if (i >= gihparams.dim)
        gtk_widget_set_sensitive (spinbutton, FALSE);

      g_signal_connect (adjustment, "value-changed",
                        G_CALLBACK (gimp_int_adjustment_update),
                        &gihparams.rank[i]);

      cellw_adjust.rank_entry[i] = cellh_adjust.rank_entry[i] = spinbutton;

      if (i == 0)
        {
          if (gihparams.dim == 1)
            cellw_adjust.rank0 = cellh_adjust.rank0 = adjustment;
          else
            cellw_adjust.rank0 = cellh_adjust.rank0 = NULL;
        }

      cb = gtk_combo_box_text_new ();

      for (j = 0; j < G_N_ELEMENTS (selection_modes); j++)
        gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (cb),
                                        selection_modes[j]);
      gtk_combo_box_set_active (GTK_COMBO_BOX (cb), 2);  /* random */

      if (gihparams.selection[i])
        for (j = 0; j < G_N_ELEMENTS (selection_modes); j++)
          if (!strcmp (gihparams.selection[i], selection_modes[j]))
            {
              gtk_combo_box_set_active (GTK_COMBO_BOX (cb), j);
              break;
            }

      gtk_table_attach (GTK_TABLE (dimtable), cb, 1, 2, i, i + 1,
                        GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

      gtk_widget_show (cb);

      if (i >= gihparams.dim)
        gtk_widget_set_sensitive (cb, FALSE);

      g_signal_connect (GTK_COMBO_BOX (cb), "changed",
                        G_CALLBACK (cb_callback),
                        &gihparams.selection[i]);

      cellw_adjust.mode_entry[i] = cellh_adjust.mode_entry[i] = cb;


    }

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 6,
                             _("Ranks:"), 0.0, 0.0,
                             dimtable, 1, FALSE);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  if (run)
    {
      gint i;

      for (i = 0; i < GIMP_PIXPIPE_MAXDIM; i++)
        gihparams.selection[i] = g_strdup (gihparams.selection[i]);

      /* Fix up bogus values */
      gihparams.ncells = MIN (gihparams.ncells,
                              num_layers * gihparams.rows * gihparams.cols);
    }

  gtk_widget_destroy (dialog);

  for (i = 0; i < cellw_adjust.nguides; i++)
    gimp_image_delete_guide (image_ID, cellw_adjust.guides[i]);
  for (i = 0; i < cellh_adjust.nguides; i++)
    gimp_image_delete_guide (image_ID, cellh_adjust.guides[i]);

  return run;
}

static gboolean
gih_save_one_brush (GOutputStream  *output,
                    gint32          drawable_ID,
                    GeglRectangle  *rect,
                    const gchar    *name,
                    GError        **error)
{
  GeglBuffer      *buffer;
  const Babl      *format;
  GimpBrushHeader  bh;
  guchar          *data;
  GimpImageType    drawable_type;
  gint             bpp;
  gint             y;

  buffer = gimp_drawable_get_buffer (drawable_ID);

  drawable_type = gimp_drawable_type (drawable_ID);

  if (! name)
    name = _("Unnamed");

  switch (drawable_type)
    {
    case GIMP_GRAY_IMAGE:
    case GIMP_GRAYA_IMAGE: /* alpha channel is ignored */
      format = babl_format ("Y' u8");
      break;

    case GIMP_RGB_IMAGE: /* alpha channel is added */
    case GIMP_RGBA_IMAGE:
      format = babl_format ("R'G'B'A u8");
      break;

    default:
      g_return_val_if_reached (FALSE);
      break;
    }

  bpp = babl_format_get_bytes_per_pixel (format);

  bh.header_size  = g_htonl (sizeof (bh) + strlen (name) + 1);
  bh.version      = g_htonl (2);
  bh.width        = g_htonl (rect->width);
  bh.height       = g_htonl (rect->height);
  bh.bytes        = g_htonl (bpp);
  bh.magic_number = g_htonl (GIMP_BRUSH_MAGIC);
  bh.spacing      = g_htonl (info.spacing);

  if (! g_output_stream_write_all (output, &bh, sizeof (bh),
                                   NULL, NULL, error))
    {
      return FALSE;
    }

  if (! g_output_stream_write_all (output, name, strlen (name) + 1,
                                   NULL, NULL, error))
    {
      return FALSE;
    }

  data = g_malloc (rect->width * bpp);

  for (y = 0; y < rect->height; y++)
    {
      gint x;

      gegl_buffer_get (buffer,
                       GEGL_RECTANGLE (rect->x, rect->y + y, rect->width, 1),
                       1.0, format, data,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      switch (bpp)
        {
        case 1: /* GRAY */
          for (x = 0; x < rect->width; x++)
            data[x] = 255 - data[x];
        }

      if (! g_output_stream_write_all (output, data, rect->width * bpp,
                                       NULL, NULL, error))
        {
          g_free (data);
          return FALSE;
        }
    }

  g_free (data);
  g_object_unref (buffer);

  return TRUE;
}

static gboolean
gih_save_image (GFile    *file,
                gint32    image_ID,
                gint32    orig_image_ID,
                gint32    drawable_ID,
                GError  **error)
{
  GOutputStream *output;
  GimpParasite  *name_parasite;
  GimpParasite  *pipe_parasite;
  gchar         *header;
  gchar         *parstring;
  gint32        *layer_ID;
  gint           nlayers, layer;
  gint           row, col;
  gint           imagew, imageh;
  gint           offsetx, offsety;
  gint           k;

  if (gihparams.ncells < 1)
    return FALSE;

  imagew = gimp_image_width (image_ID);
  imageh = gimp_image_height (image_ID);

  gimp_progress_init_printf (_("Exporting '%s'"),
                             g_file_get_parse_name (file));

  output = G_OUTPUT_STREAM (g_file_replace (file,
                                            NULL, FALSE, G_FILE_CREATE_NONE,
                                            NULL, error));
  if (! output)
    return FALSE;

  parstring = gimp_pixpipe_params_build (&gihparams);

  header = g_strdup_printf ("%s\n%d %s\n",
                            info.description, gihparams.ncells, parstring);

  if (! g_output_stream_write_all (output, header, strlen (header),
                                   NULL, NULL, error))
    {
      GCancellable *cancellable = g_cancellable_new ();

      g_cancellable_cancel (cancellable);
      g_output_stream_close (output, cancellable, NULL);
      g_object_unref (cancellable);

      g_free (parstring);
      g_free (header);
      g_object_unref (output);
      return FALSE;
    }

  g_free (header);

  name_parasite = gimp_parasite_new ("gimp-brush-pipe-name",
                                     GIMP_PARASITE_PERSISTENT,
                                     strlen (info.description) + 1,
                                     info.description);
  gimp_image_attach_parasite (orig_image_ID, name_parasite);
  gimp_parasite_free (name_parasite);

  pipe_parasite = gimp_parasite_new ("gimp-brush-pipe-parameters",
                                     GIMP_PARASITE_PERSISTENT,
                                     strlen (parstring) + 1, parstring);
  gimp_image_attach_parasite (orig_image_ID, pipe_parasite);
  gimp_parasite_free (pipe_parasite);

  g_free (parstring);

  layer_ID = gimp_image_get_layers (image_ID, &nlayers);

  for (layer = 0, k = 0; layer < nlayers; layer++)
    {
      gchar *name   = gimp_item_get_name (layer_ID[layer]);
      gint   width  = gimp_drawable_width (layer_ID[layer]);
      gint   height = gimp_drawable_height (layer_ID[layer]);

      gimp_drawable_offsets (layer_ID[layer], &offsetx, &offsety);

      for (row = 0; row < gihparams.rows; row++)
        {
          gint y, ynext;
          gint thisy, thish;

          y = (row * imageh) / gihparams.rows;
          ynext = ((row + 1) * imageh / gihparams.rows);

          /* Assume layer is offset to positive direction in x and y.
           * That's reasonable, as otherwise all of the layer
           * won't be visible.
           * thisy and thisx are in the drawable's coordinate space.
           */
          thisy = MAX (0, y - offsety);
          thish = (ynext - offsety) - thisy;
          thish = MIN (thish, height - thisy);

          for (col = 0; col < gihparams.cols; col++)
            {
              gint x, xnext;
              gint thisx, thisw;

              x = (col * imagew / gihparams.cols);
              xnext = ((col + 1) * imagew / gihparams.cols);
              thisx = MAX (0, x - offsetx);
              thisw = (xnext - offsetx) - thisx;
              thisw = MIN (thisw, width - thisx);

              if (! gih_save_one_brush (output, layer_ID[layer],
                                        GEGL_RECTANGLE (thisx, thisy,
                                                        thisw, thish),
                                        name, error))
                {
                  GCancellable *cancellable = g_cancellable_new ();

                  g_cancellable_cancel (cancellable);
                  g_output_stream_close (output, cancellable, NULL);
                  g_object_unref (cancellable);

                  g_object_unref (output);
                  return FALSE;
                }

              k++;
              gimp_progress_update ((gdouble) k / gihparams.ncells);
            }
        }

      g_free (name);
    }


  g_free (layer_ID);
  g_object_unref (output);

  gimp_progress_update (1.0);

  return TRUE;
}
