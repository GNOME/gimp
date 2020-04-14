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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <libgimpbase/gimpparasiteio.h>

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
  GimpImage           *image;
  GimpLayer           *toplayer;
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


typedef struct _Gih      Gih;
typedef struct _GihClass GihClass;

struct _Gih
{
  GimpPlugIn      parent_instance;
};

struct _GihClass
{
  GimpPlugInClass parent_class;
};


#define GIH_TYPE  (gih_get_type ())
#define GIH (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIH_TYPE, Gih))

GType                   gih_get_type         (void) G_GNUC_CONST;

static GList          * gih_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * gih_create_procedure (GimpPlugIn           *plug_in,
                                              const gchar          *name);

static GimpValueArray * gih_save             (GimpProcedure        *procedure,
                                              GimpRunMode           run_mode,
                                              GimpImage            *image,
                                              gint                  n_drawables,
                                              GimpDrawable        **drawables,
                                              GFile                *file,
                                              const GimpValueArray *args,
                                              gpointer              run_data);

static gboolean         gih_save_dialog      (GimpImage            *image);


G_DEFINE_TYPE (Gih, gih, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (GIH_TYPE)


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


static void
gih_class_init (GihClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = gih_query_procedures;
  plug_in_class->create_procedure = gih_create_procedure;
}

static void
gih_init (Gih *gih)
{
}

static GList *
gih_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (SAVE_PROC));
}

static GimpProcedure *
gih_create_procedure (GimpPlugIn  *plug_in,
                      const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, SAVE_PROC))
    {
      procedure = gimp_save_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           gih_save, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "RGB*, GRAY*");

      gimp_procedure_set_menu_label (procedure, N_("GIMP brush (animated)"));
      gimp_procedure_set_icon_name (procedure, GIMP_ICON_BRUSH);

      gimp_procedure_set_documentation (procedure,
                                        "exports images in GIMP brush pipe "
                                        "format",
                                        "This plug-in exports an image in "
                                        "the GIMP brush pipe format. For a "
                                        "colored brush pipe, RGBA layers are "
                                        "used, otherwise the layers should be "
                                        "grayscale masks. The image can be "
                                        "multi-layered, and additionally the "
                                        "layers can be divided into a "
                                        "rectangular array of brushes.",
                                        SAVE_PROC);
      gimp_procedure_set_attribution (procedure,
                                      "Tor Lillqvist",
                                      "Tor Lillqvist",
                                      "1999");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/x-gimp-gih");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "gih");
      gimp_file_procedure_set_handles_remote (GIMP_FILE_PROCEDURE (procedure),
                                              TRUE);

      GIMP_PROC_ARG_INT (procedure, "spacing",
                         "Spacing",
                         "Spacing of the brush",
                         1, 1000, 10,
                         GIMP_PARAM_READWRITE);

      GIMP_PROC_ARG_STRING (procedure, "description",
                            "Description",
                            "Short description of the gihtern",
                            "GIMP Gihtern",
                            GIMP_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "cell-width",
                         "Cell width",
                         "Width of the brush cells",
                         1, 1000, 10,
                         GIMP_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "cell-height",
                         "Cell height",
                         "Height of the brush cells",
                         1, 1000, 10,
                         GIMP_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "display-cols",
                         "Display columns",
                         "Display column number",
                         1, 1000, 1,
                         GIMP_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "display-rows",
                         "Display rows",
                         "Display row number",
                         1, 1000, 1,
                         GIMP_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "dimension",
                         "Dimension",
                         "Dimension of the brush pipe",
                         1, 4, 1,
                         GIMP_PARAM_READWRITE);

      GIMP_PROC_ARG_UINT8_ARRAY (procedure, "rank",
                                 "Rank",
                                 "Ranks of the dimensions",
                                 GIMP_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "dimension-2",
                         "Dimension 2",
                         "Dimension of the brush pipe (same as dimension)",
                         1, 4, 1,
                         GIMP_PARAM_READWRITE);

      GIMP_PROC_ARG_STRING_ARRAY (procedure, "sel",
                                  "Sel",
                                  "Selection modes",
                                  GIMP_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
gih_save (GimpProcedure        *procedure,
          GimpRunMode           run_mode,
          GimpImage            *image,
          gint                  n_drawables,
          GimpDrawable        **drawables,
          GFile                *file,
          const GimpValueArray *args,
          gpointer              run_data)
{
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpExportReturn   export = GIMP_EXPORT_CANCEL;
  GimpParasite      *parasite;
  GimpImage         *orig_image;
  GError            *error  = NULL;
  gint               i;

  INIT_I18N();

  orig_image = image;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
      gimp_ui_init (PLUG_IN_BINARY);

      export = gimp_export_image (&image, &n_drawables, &drawables, "GIH",
                                  GIMP_EXPORT_CAN_HANDLE_RGB   |
                                  GIMP_EXPORT_CAN_HANDLE_GRAY  |
                                  GIMP_EXPORT_CAN_HANDLE_ALPHA |
                                  GIMP_EXPORT_CAN_HANDLE_LAYERS);

      if (export == GIMP_EXPORT_CANCEL)
        return gimp_procedure_new_return_values (procedure,
                                                 GIMP_PDB_CANCEL,
                                                 NULL);

      /*  Possibly retrieve data  */
      gimp_get_data (SAVE_PROC, &info);

      parasite = gimp_image_get_parasite (orig_image,
                                          "gimp-brush-pipe-name");
      if (parasite)
        {
          g_strlcpy (info.description,
                     gimp_parasite_data (parasite),
                     MIN (sizeof (info.description),
                          gimp_parasite_data_size (parasite)));

          gimp_parasite_free (parasite);
        }
      else
        {
          gchar *name = g_path_get_basename (gimp_file_get_utf8_name (file));

          if (g_str_has_suffix (name, ".gih"))
            name[strlen (name) - 4] = '\0';

          if (strlen (name))
            g_strlcpy (info.description, name, sizeof (info.description));

          g_free (name);
        }

      parasite = gimp_image_get_parasite (orig_image,
                                          "gimp-brush-pipe-spacing");
      if (parasite)
        {
          info.spacing = atoi (gimp_parasite_data (parasite));
          gimp_parasite_free (parasite);
        }
      break;

    default:
      break;
    }

  g_free (gimp_image_get_layers (image, &num_layers));

  gimp_pixpipe_params_init (&gihparams);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      gihparams.ncells = (num_layers * gihparams.rows * gihparams.cols);

      gihparams.cellwidth  = gimp_image_width (image)  / gihparams.cols;
      gihparams.cellheight = gimp_image_height (image) / gihparams.rows;

      parasite = gimp_image_get_parasite (orig_image,
                                          "gimp-brush-pipe-parameters");
      if (parasite)
        {
          gimp_pixpipe_params_parse (gimp_parasite_data (parasite),
                                     &gihparams);
          gimp_parasite_free (parasite);
        }

      /* Force default rank to same as number of cells if there is
       * just one dim
       */
      if (gihparams.dim == 1)
        gihparams.rank[0] = gihparams.ncells;

      if (! gih_save_dialog (image))
        {
          status = GIMP_PDB_CANCEL;
          goto out;
        }
      break;

    case GIMP_RUN_NONINTERACTIVE:
      info.spacing = GIMP_VALUES_GET_INT (args, 0);
      g_strlcpy (info.description,
                 GIMP_VALUES_GET_STRING (args, 1),
                 sizeof (info.description));

      gihparams.cellwidth  = GIMP_VALUES_GET_INT (args, 2);
      gihparams.cellheight = GIMP_VALUES_GET_INT (args, 3);
      gihparams.cols       = GIMP_VALUES_GET_INT (args, 4);
      gihparams.rows       = GIMP_VALUES_GET_INT (args, 5);
      gihparams.dim        = GIMP_VALUES_GET_INT (args, 6);
      gihparams.ncells     = 1;

      if (GIMP_VALUES_GET_INT (args, 8) != gihparams.dim)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      else
        {
          const guint8  *rank = GIMP_VALUES_GET_UINT8_ARRAY  (args, 7);
          const gchar  **sel  = GIMP_VALUES_GET_STRING_ARRAY (args, 9);

          for (i = 0; i < gihparams.dim; i++)
            {
              gihparams.rank[i]      = rank[i];
              gihparams.selection[i] = g_strdup (sel[i]);
              gihparams.ncells       *= gihparams.rank[i];
            }
        }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      parasite = gimp_image_get_parasite (orig_image,
                                          "gimp-brush-pipe-parameters");
      if (parasite)
        {
          gimp_pixpipe_params_parse (gimp_parasite_data (parasite),
                                     &gihparams);
          gimp_parasite_free (parasite);
        }
      break;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      GimpValueArray *save_retvals;
      gchar           spacing[8];
      gchar          *paramstring;

      paramstring = gimp_pixpipe_params_build (&gihparams);

      save_retvals =
        gimp_pdb_run_procedure (gimp_get_pdb (),
                                "file-gih-save-internal",
                                GIMP_TYPE_RUN_MODE, GIMP_RUN_NONINTERACTIVE,
                                GIMP_TYPE_IMAGE,        image,
                                G_TYPE_INT,             n_drawables,
                                GIMP_TYPE_OBJECT_ARRAY, drawables,
                                G_TYPE_FILE,            file,
                                G_TYPE_INT,             info.spacing,
                                G_TYPE_STRING,          info.description,
                                G_TYPE_STRING,          paramstring,
                                G_TYPE_NONE);

      if (GIMP_VALUES_GET_ENUM (save_retvals, 0) == GIMP_PDB_SUCCESS)
        {
          gimp_set_data (SAVE_PROC, &info, sizeof (info));

          parasite = gimp_parasite_new ("gimp-brush-pipe-name",
                                        GIMP_PARASITE_PERSISTENT,
                                        strlen (info.description) + 1,
                                        info.description);
          gimp_image_attach_parasite (orig_image, parasite);
          gimp_parasite_free (parasite);

          g_snprintf (spacing, sizeof (spacing), "%d",
                      info.spacing);

          parasite = gimp_parasite_new ("gimp-brush-pipe-spacing",
                                        GIMP_PARASITE_PERSISTENT,
                                        strlen (spacing) + 1, spacing);
          gimp_image_attach_parasite (orig_image, parasite);
          gimp_parasite_free (parasite);

          parasite = gimp_parasite_new ("gimp-brush-pipe-parameters",
                                        GIMP_PARASITE_PERSISTENT,
                                        strlen (paramstring) + 1,
                                        paramstring);
          gimp_image_attach_parasite (orig_image, parasite);
          gimp_parasite_free (parasite);
        }
      else
        {
          g_set_error (&error, 0, 0,
                       "Running procedure 'file-gih-save-internal' "
                       "failed: %s",
                       gimp_pdb_get_last_error (gimp_get_pdb ()));

          status = GIMP_PDB_EXECUTION_ERROR;
        }

      g_free (paramstring);
    }

  gimp_pixpipe_params_free (&gihparams);

 out:
  if (export == GIMP_EXPORT_EXPORT)
    {
      gimp_image_delete (image);
      g_free (drawables);
    }

  return gimp_procedure_new_return_values (procedure, status, error);
}


/*  save routines */

static void
size_adjustment_callback (GtkAdjustment      *adjustment,
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

  *(adj->value) = gtk_adjustment_get_value (adjustment);

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
    gtk_adjustment_set_value (adj->ncells,
                              *(adj->other_count) * *(adj->count));
  if (adj->rank0 != NULL)
    gtk_adjustment_set_value (adj->rank0,
                              *(adj->other_count) * *(adj->count));
}

static void
entry_callback (GtkWidget *widget,
                gpointer   data)
{
  if (data == info.description)
    g_strlcpy (info.description, gtk_entry_get_text (GTK_ENTRY (widget)),
               sizeof (info.description));
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
gih_save_dialog (GimpImage *image)
{
  GtkWidget          *dialog;
  GtkWidget          *grid;
  GtkWidget          *dimgrid;
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
  GList              *layers;
  gboolean            run;

  dialog = gimp_export_dialog_new (_("Brush Pipe"), PLUG_IN_BINARY, SAVE_PROC);

  /* The main grid */
  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_container_set_border_width (GTK_CONTAINER (grid), 12);
  gtk_box_pack_start (GTK_BOX (gimp_export_dialog_get_content_area (dialog)),
                      grid, TRUE, TRUE, 0);
  gtk_widget_show (grid);

  /*
   * Description: ___________
   */
  entry = gtk_entry_new ();
  gtk_widget_set_size_request (entry, 200, -1);
  gtk_entry_set_text (GTK_ENTRY (entry), info.description);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            _("_Description:"), 0.0, 0.5,
                            entry, 1);

  g_signal_connect (entry, "changed",
                    G_CALLBACK (entry_callback),
                    info.description);

  /*
   * Spacing: __
   */
  adjustment = gtk_adjustment_new (info.spacing, 1, 1000, 1, 10, 0);
  spinbutton = gimp_spin_button_new (adjustment, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                            _("_Spacing (percent):"), 0.0, 0.5,
                            spinbutton, 1);

  g_signal_connect (adjustment, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &info.spacing);

  /*
   * Cell size: __ x __ pixels
   */
  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);

  adjustment = gtk_adjustment_new (gihparams.cellwidth,
                                   2, gimp_image_width (image), 1, 10, 0);
  spinbutton = gimp_spin_button_new (adjustment, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_box_pack_start (GTK_BOX (box), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  layers = gimp_image_list_layers (image);

  cellw_adjust.orientation = GIMP_ORIENTATION_VERTICAL;
  cellw_adjust.image       = image;
  cellw_adjust.toplayer    = g_list_last (layers)->data;
  cellw_adjust.nguides     = 0;
  cellw_adjust.guides      = NULL;
  cellw_adjust.value       = &gihparams.cellwidth;

  g_signal_connect (adjustment, "value-changed",
                    G_CALLBACK (size_adjustment_callback),
                    &cellw_adjust);

  label = gtk_label_new ("x");
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  adjustment = gtk_adjustment_new (gihparams.cellheight,
                                   2, gimp_image_height (image), 1, 10, 0);
  spinbutton = gimp_spin_button_new (adjustment, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_box_pack_start (GTK_BOX (box), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  cellh_adjust.orientation = GIMP_ORIENTATION_HORIZONTAL;
  cellh_adjust.image       = image;
  cellh_adjust.toplayer    = g_list_last (layers)->data;
  cellh_adjust.nguides     = 0;
  cellh_adjust.guides      = NULL;
  cellh_adjust.value       = &gihparams.cellheight;

  g_signal_connect (adjustment, "value-changed",
                    G_CALLBACK (size_adjustment_callback),
                    &cellh_adjust);

  label = gtk_label_new ( _("Pixels"));
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 2,
                            _("Ce_ll size:"), 0.0, 0.5,
                            box, 1);

  g_list_free (layers);

  /*
   * Number of cells: ___
   */
  adjustment = gtk_adjustment_new (gihparams.ncells, 1, 1000, 1, 10, 0);
  spinbutton = gimp_spin_button_new (adjustment, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 3,
                            _("_Number of cells:"), 0.0, 0.5,
                            spinbutton, 1);

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

  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 4,
                            _("Display as:"), 0.0, 0.5,
                            box, 1);

  /*
   * Dimension: ___
   */
  adjustment = gtk_adjustment_new (gihparams.dim,
                                   1, GIMP_PIXPIPE_MAXDIM, 1, 1, 0);
  spinbutton = gimp_spin_button_new (adjustment, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 5,
                            _("Di_mension:"), 0.0, 0.5,
                            spinbutton, 1);

  g_signal_connect (adjustment, "value-changed",
                    G_CALLBACK (dim_callback),
                    &cellw_adjust);

  /*
   * Ranks / Selection: ______ ______ ______ ______ ______
   */

  dimgrid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (dimgrid), 4);
  for (i = 0; i < GIMP_PIXPIPE_MAXDIM; i++)
    {
      gint j;

      adjustment = gtk_adjustment_new (gihparams.rank[i], 1, 100, 1, 1, 0);
      spinbutton = gimp_spin_button_new (adjustment, 1.0, 0);
      gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
      gtk_grid_attach (GTK_GRID (dimgrid), spinbutton, 0, i, 1, 1);

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

      gtk_grid_attach (GTK_GRID (dimgrid), cb, 1, i, 1, 1);

      gtk_widget_show (cb);

      if (i >= gihparams.dim)
        gtk_widget_set_sensitive (cb, FALSE);

      g_signal_connect (GTK_COMBO_BOX (cb), "changed",
                        G_CALLBACK (cb_callback),
                        &gihparams.selection[i]);

      cellw_adjust.mode_entry[i] = cellh_adjust.mode_entry[i] = cb;


    }

  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 6,
                            _("Ranks:"), 0.0, 0.0,
                            dimgrid, 1);

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
    gimp_image_delete_guide (image, cellw_adjust.guides[i]);
  for (i = 0; i < cellh_adjust.nguides; i++)
    gimp_image_delete_guide (image, cellh_adjust.guides[i]);

  return run;
}
