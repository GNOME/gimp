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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
  gboolean create_group;
  gboolean create_masks;
} WaveletDecomposeParams;


typedef struct _Wavelet      Wavelet;
typedef struct _WaveletClass WaveletClass;

struct _Wavelet
{
  GimpPlugIn parent_instance;
};

struct _WaveletClass
{
  GimpPlugInClass parent_class;
};


#define WAVELET_TYPE  (wavelet_get_type ())
#define WAVELET (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), WAVELET_TYPE, Wavelet))

GType                   wavelet_get_type         (void) G_GNUC_CONST;

static GList          * wavelet_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * wavelet_create_procedure (GimpPlugIn           *plug_in,
                                                   const gchar          *name);

static GimpValueArray * wavelet_run              (GimpProcedure        *procedure,
                                                  GimpRunMode           run_mode,
                                                  GimpImage            *image,
                                                  GimpDrawable         *drawable,
                                                  const GimpValueArray *args,
                                                  gpointer              run_data);

static void             wavelet_blur             (GimpDrawable         *drawable,
                                                  gint              radius);

static gboolean         wavelet_decompose_dialog (void);


G_DEFINE_TYPE (Wavelet, wavelet, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (WAVELET_TYPE)


static WaveletDecomposeParams wavelet_params =
{
  5,     /* default scales */
  TRUE,  /* create group */
  FALSE  /* do not add mask by default */
};


static void
wavelet_class_init (WaveletClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = wavelet_query_procedures;
  plug_in_class->create_procedure = wavelet_create_procedure;
}

static void
wavelet_init (Wavelet *wavelet)
{
}

static GList *
wavelet_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
wavelet_create_procedure (GimpPlugIn  *plug_in,
                           const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            wavelet_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "RGB*, GRAY*");

      gimp_procedure_set_menu_label (procedure, N_("_Wavelet-decompose..."));
      gimp_procedure_add_menu_path (procedure, "<Image>/Filters/Enhance");

      gimp_procedure_set_documentation (procedure,
                                        N_("Wavelet decompose"),
                                        "Compute and render wavelet scales",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Miroslav Talasek <miroslav.talasek@seznam.cz>",
                                      "Miroslav Talasek <miroslav.talasek@seznam.cz>",
                                      "19january 2017");

      GIMP_PROC_ARG_INT (procedure, "scales",
                         "Scales",
                         "Number of scales",
                         1, 7, 5,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "create-group",
                             "Create grooup",
                             "Create a layer group to store the "
                             "decomposition",
                             TRUE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "create-masks",
                             "Create masks",
                             "Add a layer mask to each scales layer",
                             FALSE,
                             G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
wavelet_run (GimpProcedure        *procedure,
              GimpRunMode           run_mode,
              GimpImage            *image,
              GimpDrawable         *drawable,
              const GimpValueArray *args,
              gpointer              run_data)
{
  GimpLayer    **scale_layers;
  GimpLayer     *new_scale;
  GimpLayer     *parent             = NULL;
  GimpLayerMode  grain_extract_mode = GIMP_LAYER_MODE_GRAIN_EXTRACT;
  GimpLayerMode  grain_merge_mode   = GIMP_LAYER_MODE_GRAIN_MERGE;
  gint           id;

  INIT_I18N();
  gegl_init (NULL, NULL);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      gimp_get_data (PLUG_IN_PROC, &wavelet_params);

      if (! wavelet_decompose_dialog ())
        return gimp_procedure_new_return_values (procedure,
                                                 GIMP_PDB_CANCEL,
                                                 NULL);
      break;

    case GIMP_RUN_NONINTERACTIVE:
      wavelet_params.scales       = GIMP_VALUES_GET_INT     (args, 0);
      wavelet_params.create_group = GIMP_VALUES_GET_BOOLEAN (args, 1);
      wavelet_params.create_masks = GIMP_VALUES_GET_BOOLEAN (args, 2);
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data (PLUG_IN_PROC, &wavelet_params);
      break;

    default:
      break;
    }

  gimp_progress_init (_("Wavelet-Decompose"));

  gimp_image_undo_group_start (image);

  gimp_image_freeze_layers (image);

  if (wavelet_params.create_group)
    {
      parent = gimp_layer_group_new (image);

      gimp_item_set_name (GIMP_ITEM (parent), _("Decomposition"));
      gimp_item_set_visible (GIMP_ITEM (parent), FALSE);
      gimp_image_insert_layer (image, parent,
                               GIMP_LAYER (gimp_item_get_parent (GIMP_ITEM (drawable))),
                               gimp_image_get_item_position (image,
                                                             GIMP_ITEM (drawable)));
    }

  scale_layers = g_new (GimpLayer *, wavelet_params.scales);
  new_scale = gimp_layer_copy (GIMP_LAYER (drawable));
  gimp_image_insert_layer (image, new_scale, parent,
                           gimp_image_get_item_position (image,
                                                         GIMP_ITEM (drawable)));

  /* the exact result of the grain-extract and grain-merge modes
   * depends on the choice of (gamma-corrected) midpoint intensity
   * value.  for the non-legacy modes, the midpoint value is 0.5,
   * which isn't representable exactly using integer precision.  for
   * the legacy modes, the midpoint value is 128/255 (i.e., 0x80),
   * which is representable exactly using (gamma-corrected) integer
   * precision.  we therefore use the legacy modes when the input
   * image precision is integer, and only use the (preferable)
   * non-legacy modes when the input image precision is floating
   * point.
   *
   * this avoids imperfect reconstruction of the image when using
   * integer precision.  see bug #786844.
   */
  switch (gimp_image_get_precision (image))
    {
    case GIMP_PRECISION_U8_LINEAR:
    case GIMP_PRECISION_U8_NON_LINEAR:
    case GIMP_PRECISION_U8_PERCEPTUAL:
    case GIMP_PRECISION_U16_LINEAR:
    case GIMP_PRECISION_U16_NON_LINEAR:
    case GIMP_PRECISION_U16_PERCEPTUAL:
    case GIMP_PRECISION_U32_LINEAR:
    case GIMP_PRECISION_U32_NON_LINEAR:
    case GIMP_PRECISION_U32_PERCEPTUAL:
      grain_extract_mode = GIMP_LAYER_MODE_GRAIN_EXTRACT_LEGACY;
      grain_merge_mode   = GIMP_LAYER_MODE_GRAIN_MERGE_LEGACY;
      break;

    case GIMP_PRECISION_HALF_LINEAR:
    case GIMP_PRECISION_HALF_NON_LINEAR:
    case GIMP_PRECISION_HALF_PERCEPTUAL:
    case GIMP_PRECISION_FLOAT_LINEAR:
    case GIMP_PRECISION_FLOAT_NON_LINEAR:
    case GIMP_PRECISION_FLOAT_PERCEPTUAL:
    case GIMP_PRECISION_DOUBLE_LINEAR:
    case GIMP_PRECISION_DOUBLE_NON_LINEAR:
    case GIMP_PRECISION_DOUBLE_PERCEPTUAL:
      grain_extract_mode = GIMP_LAYER_MODE_GRAIN_EXTRACT;
      grain_merge_mode   = GIMP_LAYER_MODE_GRAIN_MERGE;
      break;
    }

  for (id = 0 ; id < wavelet_params.scales; id++)
    {
      GimpLayer *blur;
      GimpLayer *tmp;
      gchar      scale_name[20];

      gimp_progress_update ((gdouble) id / (gdouble) wavelet_params.scales);

      scale_layers[id] = new_scale;

      g_snprintf (scale_name, sizeof (scale_name), _("Scale %d"), id + 1);
      gimp_item_set_name (GIMP_ITEM (new_scale), scale_name);

      tmp = gimp_layer_copy (new_scale);
      gimp_image_insert_layer (image, tmp, parent,
                               gimp_image_get_item_position (image,
                                                             GIMP_ITEM (new_scale)));
      wavelet_blur (GIMP_DRAWABLE (tmp), pow (2.0, id));

      blur = gimp_layer_copy (tmp);
      gimp_image_insert_layer (image, blur, parent,
                               gimp_image_get_item_position (image,
                                                             GIMP_ITEM (tmp)));

      gimp_layer_set_mode (tmp, grain_extract_mode);
      new_scale = gimp_image_merge_down (image, tmp,
                                         GIMP_EXPAND_AS_NECESSARY);
      scale_layers[id] = new_scale;

      gimp_item_set_visible (GIMP_ITEM (new_scale), FALSE);

      new_scale = blur;
    }

  gimp_item_set_name (GIMP_ITEM (new_scale), _("Residual"));

  for (id = 0; id < wavelet_params.scales; id++)
    {
      gimp_image_reorder_item (image, GIMP_ITEM (scale_layers[id]),
                               GIMP_ITEM (parent),
                               gimp_image_get_item_position (image,
                                                             GIMP_ITEM (new_scale)));
      gimp_layer_set_mode (scale_layers[id], grain_merge_mode);

      if (wavelet_params.create_masks)
        {
          GimpLayerMask *mask = gimp_layer_create_mask (scale_layers[id],
                                                        GIMP_ADD_MASK_WHITE);
          gimp_layer_add_mask (scale_layers[id], mask);
        }

      gimp_item_set_visible (GIMP_ITEM (scale_layers[id]), TRUE);
    }

  if (wavelet_params.create_group)
    gimp_item_set_visible (GIMP_ITEM (parent), TRUE);

  g_free (scale_layers);

  gimp_image_thaw_layers (image);

  gimp_image_undo_group_end (image);

  gimp_progress_update (1.0);

  gimp_displays_flush ();

  if (run_mode == GIMP_RUN_INTERACTIVE)
    gimp_set_data (PLUG_IN_PROC,
                   &wavelet_params, sizeof (WaveletDecomposeParams));

  gegl_exit ();

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}

static void
wavelet_blur (GimpDrawable *drawable,
              gint          radius)
{
  gint x, y, width, height;

  if (gimp_drawable_mask_intersect (drawable, &x, &y, &width, &height))
    {
      GeglBuffer *buffer = gimp_drawable_get_buffer (drawable);
      GeglBuffer *shadow = gimp_drawable_get_shadow_buffer (drawable);

      gegl_render_op (buffer, shadow,
                      "gegl:wavelet-blur",
                      "radius", (gdouble) radius,
                      NULL);

      gegl_buffer_flush (shadow);
      gimp_drawable_merge_shadow (drawable, FALSE);
      gimp_drawable_update (drawable, x, y, width, height);
      g_object_unref (buffer);
      g_object_unref (shadow);
    }
}

static gboolean
wavelet_decompose_dialog (void)
{
  GtkWidget     *dialog;
  GtkWidget     *main_vbox;
  GtkWidget     *grid;
  GtkWidget     *button;
  GtkAdjustment *adj;
  gboolean       run;

  gimp_ui_init (PLUG_IN_BINARY);

  dialog = gimp_dialog_new (_("Wavelet decompose"), PLUG_IN_ROLE,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_OK"),     GTK_RESPONSE_OK,

                            NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  /* scales */

  adj = gimp_scale_entry_new (GTK_GRID (grid), 0, 0,
                              _("Scales:"), SCALE_WIDTH, ENTRY_WIDTH,
                              wavelet_params.scales,
                              1.0, 7.0, 1.0, 1.0, 0,
                              TRUE, 0, 0,
                              NULL, NULL);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &wavelet_params.scales);

  /* create group layer */

  button = gtk_check_button_new_with_mnemonic (_("Create a layer group to store the decomposition"));
  gtk_box_pack_start (GTK_BOX (main_vbox), button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                wavelet_params.create_group);
  gtk_widget_show (button);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &wavelet_params.create_group);

  /* create layer masks */

  button = gtk_check_button_new_with_mnemonic (_("Add a layer mask to each scales layers"));
  gtk_box_pack_start (GTK_BOX (main_vbox), button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                wavelet_params.create_masks);
  gtk_widget_show (button);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &wavelet_params.create_masks);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}
