/* Plug-in to load and export .gih (LIGMA Brush Pipe) files.
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

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>
#include <libligmabase/ligmaparasiteio.h>

#include "libligma/stdplugins-intl.h"


#define SAVE_PROC      "file-gih-save"
#define PLUG_IN_BINARY "file-gih"
#define PLUG_IN_ROLE   "ligma-file-gih"


/* Parameters applicable each time we export a gih, exported in the
 * main ligma application between invocations of this plug-in.
 */
typedef struct
{
  gchar description[256];
  gint  spacing;
} BrushInfo;

typedef struct
{
  LigmaOrientationType  orientation;
  LigmaImage           *image;
  LigmaLayer           *toplayer;
  gint                 nguides;
  gint32              *guides;
  gint                *value;
  GtkWidget           *count_label;    /* Corresponding count adjustment, */
  gint                *count;          /* cols or rows                    */
  gint                *other_count;    /* and the other one               */
  GtkAdjustment       *ncells;
  GtkAdjustment       *rank0;
  GtkWidget           *warning_label;
  GtkWidget           *rank_entry[LIGMA_PIXPIPE_MAXDIM];
  GtkWidget           *mode_entry[LIGMA_PIXPIPE_MAXDIM];
} SizeAdjustmentData;


typedef struct _Gih      Gih;
typedef struct _GihClass GihClass;

struct _Gih
{
  LigmaPlugIn      parent_instance;
};

struct _GihClass
{
  LigmaPlugInClass parent_class;
};


#define GIH_TYPE  (gih_get_type ())
#define GIH (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIH_TYPE, Gih))

GType                   gih_get_type         (void) G_GNUC_CONST;

static GList          * gih_query_procedures (LigmaPlugIn           *plug_in);
static LigmaProcedure  * gih_create_procedure (LigmaPlugIn           *plug_in,
                                              const gchar          *name);

static LigmaValueArray * gih_save             (LigmaProcedure        *procedure,
                                              LigmaRunMode           run_mode,
                                              LigmaImage            *image,
                                              gint                  n_drawables,
                                              LigmaDrawable        **drawables,
                                              GFile                *file,
                                              const LigmaValueArray *args,
                                              gpointer              run_data);

static gboolean         gih_save_dialog      (LigmaImage            *image);


G_DEFINE_TYPE (Gih, gih, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (GIH_TYPE)
DEFINE_STD_SET_I18N


static BrushInfo info =
{
  "LIGMA Brush Pipe",
  20
};

static gint              num_layers = 0;
static LigmaPixPipeParams gihparams  = { 0, };

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
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = gih_query_procedures;
  plug_in_class->create_procedure = gih_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
gih_init (Gih *gih)
{
}

static GList *
gih_query_procedures (LigmaPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (SAVE_PROC));
}

static LigmaProcedure *
gih_create_procedure (LigmaPlugIn  *plug_in,
                      const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, SAVE_PROC))
    {
      procedure = ligma_save_procedure_new (plug_in, name,
                                           LIGMA_PDB_PROC_TYPE_PLUGIN,
                                           gih_save, NULL, NULL);

      ligma_procedure_set_image_types (procedure, "RGB*, GRAY*");

      ligma_procedure_set_menu_label (procedure, _("LIGMA brush (animated)"));
      ligma_procedure_set_icon_name (procedure, LIGMA_ICON_BRUSH);

      ligma_procedure_set_documentation (procedure,
                                        "exports images in LIGMA brush pipe "
                                        "format",
                                        "This plug-in exports an image in "
                                        "the LIGMA brush pipe format. For a "
                                        "colored brush pipe, RGBA layers are "
                                        "used, otherwise the layers should be "
                                        "grayscale masks. The image can be "
                                        "multi-layered, and additionally the "
                                        "layers can be divided into a "
                                        "rectangular array of brushes.",
                                        SAVE_PROC);
      ligma_procedure_set_attribution (procedure,
                                      "Tor Lillqvist",
                                      "Tor Lillqvist",
                                      "1999");

      ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                          "image/x-ligma-gih");
      ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                          "gih");
      ligma_file_procedure_set_handles_remote (LIGMA_FILE_PROCEDURE (procedure),
                                              TRUE);

      LIGMA_PROC_ARG_INT (procedure, "spacing",
                         "Spacing",
                         "Spacing of the brush",
                         1, 1000, 10,
                         LIGMA_PARAM_READWRITE);

      LIGMA_PROC_ARG_STRING (procedure, "description",
                            "Description",
                            "Short description of the gihtern",
                            "LIGMA Gihtern",
                            LIGMA_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "cell-width",
                         "Cell width",
                         "Width of the brush cells",
                         1, 1000, 10,
                         LIGMA_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "cell-height",
                         "Cell height",
                         "Height of the brush cells",
                         1, 1000, 10,
                         LIGMA_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "display-cols",
                         "Display columns",
                         "Display column number",
                         1, 1000, 1,
                         LIGMA_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "display-rows",
                         "Display rows",
                         "Display row number",
                         1, 1000, 1,
                         LIGMA_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "dimension",
                         "Dimension",
                         "Dimension of the brush pipe",
                         1, 4, 1,
                         LIGMA_PARAM_READWRITE);

      LIGMA_PROC_ARG_UINT8_ARRAY (procedure, "rank",
                                 "Rank",
                                 "Ranks of the dimensions",
                                 LIGMA_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "dimension-2",
                         "Dimension 2",
                         "Dimension of the brush pipe (same as dimension)",
                         1, 4, 1,
                         LIGMA_PARAM_READWRITE);

      LIGMA_PROC_ARG_STRV (procedure, "sel",
                          "Sel",
                          "Selection modes",
                          LIGMA_PARAM_READWRITE);
    }

  return procedure;
}

static LigmaValueArray *
gih_save (LigmaProcedure        *procedure,
          LigmaRunMode           run_mode,
          LigmaImage            *image,
          gint                  n_drawables,
          LigmaDrawable        **drawables,
          GFile                *file,
          const LigmaValueArray *args,
          gpointer              run_data)
{
  LigmaPDBStatusType  status = LIGMA_PDB_SUCCESS;
  LigmaExportReturn   export = LIGMA_EXPORT_CANCEL;
  LigmaParasite      *parasite;
  LigmaImage         *orig_image;
  GError            *error  = NULL;
  gint               i;

  orig_image = image;

  switch (run_mode)
    {
    case LIGMA_RUN_INTERACTIVE:
    case LIGMA_RUN_WITH_LAST_VALS:
      ligma_ui_init (PLUG_IN_BINARY);

      export = ligma_export_image (&image, &n_drawables, &drawables, "GIH",
                                  LIGMA_EXPORT_CAN_HANDLE_RGB   |
                                  LIGMA_EXPORT_CAN_HANDLE_GRAY  |
                                  LIGMA_EXPORT_CAN_HANDLE_ALPHA |
                                  LIGMA_EXPORT_CAN_HANDLE_LAYERS);

      if (export == LIGMA_EXPORT_CANCEL)
        return ligma_procedure_new_return_values (procedure,
                                                 LIGMA_PDB_CANCEL,
                                                 NULL);

      /*  Possibly retrieve data  */
      ligma_get_data (SAVE_PROC, &info);

      parasite = ligma_image_get_parasite (orig_image,
                                          "ligma-brush-pipe-name");
      if (parasite)
        {
          gchar   *parasite_data;
          guint32  parasite_size;

          parasite_data = (gchar *) ligma_parasite_get_data (parasite, &parasite_size);

          g_strlcpy (info.description, parasite_data,
                     MIN (sizeof (info.description), parasite_size));

          ligma_parasite_free (parasite);
        }
      else
        {
          gchar *name = g_path_get_basename (ligma_file_get_utf8_name (file));

          if (g_str_has_suffix (name, ".gih"))
            name[strlen (name) - 4] = '\0';

          if (strlen (name))
            g_strlcpy (info.description, name, sizeof (info.description));

          g_free (name);
        }

      parasite = ligma_image_get_parasite (orig_image,
                                          "ligma-brush-pipe-spacing");
      if (parasite)
        {
          gchar   *parasite_data;
          guint32  parasite_size;

          parasite_data = (gchar *) ligma_parasite_get_data (parasite, &parasite_size);
          parasite_data = g_strndup (parasite_data, parasite_size);

          info.spacing = atoi (parasite_data);

          ligma_parasite_free (parasite);
          g_free (parasite_data);
        }
      break;

    default:
      break;
    }

  g_free (ligma_image_get_layers (image, &num_layers));

  ligma_pixpipe_params_init (&gihparams);

  switch (run_mode)
    {
    case LIGMA_RUN_INTERACTIVE:
      gihparams.ncells = (num_layers * gihparams.rows * gihparams.cols);

      gihparams.cellwidth  = ligma_image_get_width (image)  / gihparams.cols;
      gihparams.cellheight = ligma_image_get_height (image) / gihparams.rows;

      parasite = ligma_image_get_parasite (orig_image,
                                          "ligma-brush-pipe-parameters");
      if (parasite)
        {
          gchar   *parasite_data;
          guint32  parasite_size;

          parasite_data = (gchar *) ligma_parasite_get_data (parasite, &parasite_size);
          parasite_data = g_strndup (parasite_data, parasite_size);

          ligma_pixpipe_params_parse (parasite_data, &gihparams);

          ligma_parasite_free (parasite);
          g_free (parasite_data);
        }

      /* Force default rank to same as number of cells if there is
       * just one dim
       */
      if (gihparams.dim == 1)
        gihparams.rank[0] = gihparams.ncells;

      if (! gih_save_dialog (image))
        {
          status = LIGMA_PDB_CANCEL;
          goto out;
        }
      break;

    case LIGMA_RUN_NONINTERACTIVE:
      info.spacing = LIGMA_VALUES_GET_INT (args, 0);
      g_strlcpy (info.description,
                 LIGMA_VALUES_GET_STRING (args, 1),
                 sizeof (info.description));

      gihparams.cellwidth  = LIGMA_VALUES_GET_INT (args, 2);
      gihparams.cellheight = LIGMA_VALUES_GET_INT (args, 3);
      gihparams.cols       = LIGMA_VALUES_GET_INT (args, 4);
      gihparams.rows       = LIGMA_VALUES_GET_INT (args, 5);
      gihparams.dim        = LIGMA_VALUES_GET_INT (args, 6);
      gihparams.ncells     = 1;

      if (LIGMA_VALUES_GET_INT (args, 8) != gihparams.dim)
        {
          status = LIGMA_PDB_CALLING_ERROR;
        }
      else
        {
          const guint8  *rank = LIGMA_VALUES_GET_UINT8_ARRAY  (args, 7);
          const gchar  **sel  = LIGMA_VALUES_GET_STRV (args, 9);

          for (i = 0; i < gihparams.dim; i++)
            {
              gihparams.rank[i]      = rank[i];
              gihparams.selection[i] = g_strdup (sel[i]);
              gihparams.ncells       *= gihparams.rank[i];
            }
        }
      break;

    case LIGMA_RUN_WITH_LAST_VALS:
      parasite = ligma_image_get_parasite (orig_image,
                                          "ligma-brush-pipe-parameters");
      if (parasite)
        {
          gchar   *parasite_data;
          guint32  parasite_size;

          parasite_data = (gchar *) ligma_parasite_get_data (parasite, &parasite_size);
          parasite_data = g_strndup (parasite_data, parasite_size);

          ligma_pixpipe_params_parse (parasite_data, &gihparams);

          ligma_parasite_free (parasite);
          g_free (parasite_data);
        }
      break;
    }

  if (status == LIGMA_PDB_SUCCESS)
    {
      LigmaValueArray *save_retvals;
      gchar           spacing[8];
      gchar          *paramstring;
      LigmaValueArray *args;

      paramstring = ligma_pixpipe_params_build (&gihparams);

      args = ligma_value_array_new_from_types (NULL,
                                              LIGMA_TYPE_RUN_MODE,     LIGMA_RUN_NONINTERACTIVE,
                                              LIGMA_TYPE_IMAGE,        image,
                                              G_TYPE_INT,             n_drawables,
                                              LIGMA_TYPE_OBJECT_ARRAY, NULL,
                                              G_TYPE_FILE,            file,
                                              G_TYPE_INT,             info.spacing,
                                              G_TYPE_STRING,          info.description,
                                              G_TYPE_STRING,          paramstring,
                                              G_TYPE_NONE);
      ligma_value_set_object_array (ligma_value_array_index (args, 3),
                                   LIGMA_TYPE_ITEM, (GObject **) drawables, n_drawables);

      save_retvals = ligma_pdb_run_procedure_array (ligma_get_pdb (),
                                                   "file-gih-save-internal",
                                                   args);
      ligma_value_array_unref (args);

      if (LIGMA_VALUES_GET_ENUM (save_retvals, 0) == LIGMA_PDB_SUCCESS)
        {
          ligma_set_data (SAVE_PROC, &info, sizeof (info));

          parasite = ligma_parasite_new ("ligma-brush-pipe-name",
                                        LIGMA_PARASITE_PERSISTENT,
                                        strlen (info.description) + 1,
                                        info.description);
          ligma_image_attach_parasite (orig_image, parasite);
          ligma_parasite_free (parasite);

          g_snprintf (spacing, sizeof (spacing), "%d",
                      info.spacing);

          parasite = ligma_parasite_new ("ligma-brush-pipe-spacing",
                                        LIGMA_PARASITE_PERSISTENT,
                                        strlen (spacing) + 1, spacing);
          ligma_image_attach_parasite (orig_image, parasite);
          ligma_parasite_free (parasite);

          parasite = ligma_parasite_new ("ligma-brush-pipe-parameters",
                                        LIGMA_PARASITE_PERSISTENT,
                                        strlen (paramstring) + 1,
                                        paramstring);
          ligma_image_attach_parasite (orig_image, parasite);
          ligma_parasite_free (parasite);
        }
      else
        {
          g_set_error (&error, 0, 0,
                       "Running procedure 'file-gih-save-internal' "
                       "failed: %s",
                       ligma_pdb_get_last_error (ligma_get_pdb ()));

          status = LIGMA_PDB_EXECUTION_ERROR;
        }

      g_free (paramstring);
    }

  ligma_pixpipe_params_free (&gihparams);

 out:
  if (export == LIGMA_EXPORT_EXPORT)
    {
      ligma_image_delete (image);
      g_free (drawables);
    }

  return ligma_procedure_new_return_values (procedure, status, error);
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
    ligma_image_delete_guide (adj->image, adj->guides[i]);

  g_free (adj->guides);
  adj->guides = NULL;
  ligma_displays_flush ();

  *(adj->value) = gtk_adjustment_get_value (adjustment);

  if (adj->orientation == LIGMA_ORIENTATION_VERTICAL)
    {
      size = ligma_image_get_width (adj->image);
      newn = size / *(adj->value);
      adj->nguides = newn - 1;
      adj->guides = g_new (gint32, adj->nguides);
      for (i = 0; i < adj->nguides; i++)
        adj->guides[i] = ligma_image_add_vguide (adj->image,
                                                *(adj->value) * (i+1));
    }
  else
    {
      size = ligma_image_get_height (adj->image);
      newn = size / *(adj->value);
      adj->nguides = newn - 1;
      adj->guides = g_new (gint32, adj->nguides);
      for (i = 0; i < adj->nguides; i++)
        adj->guides[i] = ligma_image_add_hguide (adj->image,
                                                *(adj->value) * (i+1));
    }
  ligma_displays_flush ();
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

  for (i = 0; i < LIGMA_PIXPIPE_MAXDIM; i++)
    {
      gtk_widget_set_sensitive (data->rank_entry[i], i < gihparams.dim);
      gtk_widget_set_sensitive (data->mode_entry[i], i < gihparams.dim);
    }
}

static gboolean
gih_save_dialog (LigmaImage *image)
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

  dialog = ligma_export_dialog_new (_("Brush Pipe"), PLUG_IN_BINARY, SAVE_PROC);

  /* The main grid */
  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_container_set_border_width (GTK_CONTAINER (grid), 12);
  gtk_box_pack_start (GTK_BOX (ligma_export_dialog_get_content_area (dialog)),
                      grid, TRUE, TRUE, 0);
  gtk_widget_show (grid);

  /*
   * Description: ___________
   */
  entry = gtk_entry_new ();
  gtk_widget_set_size_request (entry, 200, -1);
  gtk_entry_set_text (GTK_ENTRY (entry), info.description);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            _("_Description:"), 0.0, 0.5,
                            entry, 1);

  g_signal_connect (entry, "changed",
                    G_CALLBACK (entry_callback),
                    info.description);

  /*
   * Spacing: __
   */
  adjustment = gtk_adjustment_new (info.spacing, 1, 1000, 1, 10, 0);
  spinbutton = ligma_spin_button_new (adjustment, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                            _("_Spacing (percent):"), 0.0, 0.5,
                            spinbutton, 1);

  g_signal_connect (adjustment, "value-changed",
                    G_CALLBACK (ligma_int_adjustment_update),
                    &info.spacing);

  /*
   * Cell size: __ x __ pixels
   */
  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);

  adjustment = gtk_adjustment_new (gihparams.cellwidth,
                                   2, ligma_image_get_width (image), 1, 10, 0);
  spinbutton = ligma_spin_button_new (adjustment, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_box_pack_start (GTK_BOX (box), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  layers = ligma_image_list_layers (image);

  cellw_adjust.orientation = LIGMA_ORIENTATION_VERTICAL;
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
                                   2, ligma_image_get_height (image), 1, 10, 0);
  spinbutton = ligma_spin_button_new (adjustment, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_box_pack_start (GTK_BOX (box), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  cellh_adjust.orientation = LIGMA_ORIENTATION_HORIZONTAL;
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

  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 2,
                            _("Ce_ll size:"), 0.0, 0.5,
                            box, 1);

  g_list_free (layers);

  /*
   * Number of cells: ___
   */
  adjustment = gtk_adjustment_new (gihparams.ncells, 1, 1000, 1, 10, 0);
  spinbutton = ligma_spin_button_new (adjustment, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 3,
                            _("_Number of cells:"), 0.0, 0.5,
                            spinbutton, 1);

  g_signal_connect (adjustment, "value-changed",
                    G_CALLBACK (ligma_int_adjustment_update),
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

  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 4,
                            _("Display as:"), 0.0, 0.5,
                            box, 1);

  /*
   * Dimension: ___
   */
  adjustment = gtk_adjustment_new (gihparams.dim,
                                   1, LIGMA_PIXPIPE_MAXDIM, 1, 1, 0);
  spinbutton = ligma_spin_button_new (adjustment, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 5,
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
  for (i = 0; i < LIGMA_PIXPIPE_MAXDIM; i++)
    {
      gsize j;

      adjustment = gtk_adjustment_new (gihparams.rank[i], 1, 100, 1, 1, 0);
      spinbutton = ligma_spin_button_new (adjustment, 1.0, 0);
      gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
      gtk_grid_attach (GTK_GRID (dimgrid), spinbutton, 0, i, 1, 1);

      gtk_widget_show (spinbutton);

      if (i >= gihparams.dim)
        gtk_widget_set_sensitive (spinbutton, FALSE);

      g_signal_connect (adjustment, "value-changed",
                        G_CALLBACK (ligma_int_adjustment_update),
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

  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 6,
                            _("Ranks:"), 0.0, 0.0,
                            dimgrid, 1);

  gtk_widget_show (dialog);

  run = (ligma_dialog_run (LIGMA_DIALOG (dialog)) == GTK_RESPONSE_OK);

  if (run)
    {
      gint i;

      for (i = 0; i < LIGMA_PIXPIPE_MAXDIM; i++)
        gihparams.selection[i] = g_strdup (gihparams.selection[i]);

      /* Fix up bogus values */
      gihparams.ncells = MIN (gihparams.ncells,
                              num_layers * gihparams.rows * gihparams.cols);
    }

  gtk_widget_destroy (dialog);

  for (i = 0; i < cellw_adjust.nguides; i++)
    ligma_image_delete_guide (image, cellw_adjust.guides[i]);
  for (i = 0; i < cellh_adjust.nguides; i++)
    ligma_image_delete_guide (image, cellh_adjust.guides[i]);

  return run;
}
