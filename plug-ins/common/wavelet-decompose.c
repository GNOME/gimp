/*
 *  Wavelet decompose plug-in by Miroslav Talasek, miroslav.talasek@seznam.cz
 */

/*
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

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-wavelet-decompose"
#define PLUG_IN_ROLE   "gimp-wavelet-decompose"
#define PLUG_IN_BINARY "wavelet-decompose"

#define SCALE_WIDTH   120
#define ENTRY_WIDTH     5


typedef struct
{
  gint     scales;
} WaveletDecomposeParams;


/* Declare local functions.
 */
static void      query                    (void);
static void      run                      (const gchar      *name,
                                           gint              nparams,
                                           const GimpParam  *param,
                                           gint             *nreturn_vals,
                                           GimpParam       **return_vals);

static void      wavelet_blur             (gint32            drawable_id,
                                           gint              radius);
static gboolean  wavelet_decompose_dialog (void);


/* create a few globals, set default values */
static WaveletDecomposeParams wavelet_params =
{
  5 /*default scales*/
};

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};


MAIN ()


static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode", "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",    "Input image (unused)"   },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable"         },
    { GIMP_PDB_INT32,    "scales",   "Number of scales (1-7)" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Wavelet decompose"),
                          "Compute and render wavelet scales",
                          "Miroslav Talasek <miroslav.talasek@seznam.cz>",
                          "Miroslav Talasek <miroslav.talasek@seznam.cz>",
                          "19january 2017",
                          N_("_Wavelet-decompose..."),
                          "RGB*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Enhance");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[1];
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  gint32             image_id;
  gint32             drawable_id;
  GimpRunMode        run_mode;

  run_mode = param[0].data.d_int32;

  INIT_I18N();
  gegl_init (NULL, NULL);

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  image_id    = param[1].data.d_image;
  drawable_id = param[2].data.d_drawable;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      gimp_get_data (PLUG_IN_PROC, &wavelet_params);

      if (! wavelet_decompose_dialog ())
        return;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      if (nparams != 4)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      else
        {
          wavelet_params.scales = param[3].data.d_int32;
        }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data (PLUG_IN_PROC, &wavelet_params);
      break;

    default:
      break;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      gint32 *scale_ids;
      gint32  original_id;
      gint    id;

      gimp_progress_init (_("Wavelet-Decompose"));

      gimp_image_undo_group_start (image_id);

      scale_ids = g_new (gint32, wavelet_params.scales);
      original_id = gimp_layer_copy (drawable_id);
      gimp_image_insert_layer (image_id, original_id, -1, 0);

      for (id = 0 ; id < wavelet_params.scales; ++id)
        {
          gint32 blur_id;
          gchar  scale_name[20];

          gimp_progress_update ((gdouble) id / (gdouble) wavelet_params.scales);

          /* blur */
          blur_id = gimp_layer_copy (original_id);
          gimp_image_insert_layer (image_id, blur_id, 0,
                                   gimp_image_get_item_position (image_id,
                                                                 original_id));
          wavelet_blur (blur_id, 1 << id);

          /* grain extract */
          gimp_layer_set_mode (blur_id,GIMP_LAYER_MODE_GRAIN_EXTRACT);

          /* new from visible */
          g_snprintf (scale_name, sizeof (scale_name), "Scale %d", id + 1);
          scale_ids[id] = gimp_layer_new_from_visible (image_id, image_id,
                                                       scale_name);

          gimp_image_insert_layer (image_id, scale_ids[id], 0,
                                   gimp_image_get_item_position (image_id,
                                                                 blur_id) );
          gimp_layer_set_mode (scale_ids[id], GIMP_LAYER_MODE_GRAIN_MERGE);
          gimp_layer_set_mode (blur_id, GIMP_LAYER_MODE_NORMAL_LEGACY);
          gimp_item_set_visible (scale_ids[id], FALSE);

          gimp_image_remove_layer (image_id, original_id);

          original_id = blur_id;
        }

      gimp_item_set_name (original_id, "Residual");

      for (id = 0 ; id < wavelet_params.scales; ++id)
        {
          gimp_item_set_visible (scale_ids[id], TRUE);
        }

      g_free (scale_ids);

      gimp_image_undo_group_end (image_id);

      gimp_progress_update (1.0);

      values[0].data.d_status = status;
      gimp_displays_flush ();

      /* set data for next use of filter */
      if (run_mode == GIMP_RUN_INTERACTIVE)
        gimp_set_data (PLUG_IN_PROC,
                       &wavelet_params, sizeof (WaveletDecomposeParams));
    }

  gegl_exit ();
}

static void
wavelet_blur (gint32 drawable_id,
              gint   radius)
{
  gint x, y, width, height;

  if (gimp_drawable_mask_intersect (drawable_id, &x, &y, &width, &height))
    {
      GeglBuffer *buffer;
      GeglBuffer *shadow_buffer;

      buffer        = gimp_drawable_get_buffer (drawable_id);
      shadow_buffer = gimp_drawable_get_shadow_buffer (drawable_id);

      gegl_render_op (buffer, shadow_buffer,
                      "gegl:wavelet-blur",
                      "radius", (gdouble) radius,
                      NULL);

      g_object_unref (shadow_buffer); /* flushes the shadow blur */
      g_object_unref (buffer);

      gimp_drawable_merge_shadow (drawable_id, TRUE);
      gimp_drawable_update (drawable_id, x, y, width, height);
      gimp_displays_flush ();
    }
}

static gboolean
wavelet_decompose_dialog (void)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *table;
  GtkObject *adj;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  dialog = gimp_dialog_new (_("Wavelet decompose"), PLUG_IN_ROLE,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_OK"),     GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  table = gtk_table_new (3, 1, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                              _("Scales:"), SCALE_WIDTH, ENTRY_WIDTH,
                              wavelet_params.scales,
                              1.0, 7.0, 1.0, 1.0, 0,
                              TRUE, 0, 0,
                              NULL, NULL);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &wavelet_params.scales);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}
