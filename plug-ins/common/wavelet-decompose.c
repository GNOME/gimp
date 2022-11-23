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

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#include "libligma/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-wavelet-decompose"
#define PLUG_IN_ROLE   "ligma-wavelet-decompose"
#define PLUG_IN_BINARY "wavelet-decompose"


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
  LigmaPlugIn parent_instance;
};

struct _WaveletClass
{
  LigmaPlugInClass parent_class;
};


#define WAVELET_TYPE  (wavelet_get_type ())
#define WAVELET (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), WAVELET_TYPE, Wavelet))

GType                   wavelet_get_type         (void) G_GNUC_CONST;

static GList          * wavelet_query_procedures (LigmaPlugIn           *plug_in);
static LigmaProcedure  * wavelet_create_procedure (LigmaPlugIn           *plug_in,
                                                   const gchar          *name);

static LigmaValueArray * wavelet_run              (LigmaProcedure        *procedure,
                                                  LigmaRunMode           run_mode,
                                                  LigmaImage            *image,
                                                  gint                  n_drawables,
                                                  LigmaDrawable        **drawables,
                                                  const LigmaValueArray *args,
                                                  gpointer              run_data);

static void             wavelet_blur             (LigmaDrawable         *drawable,
                                                  gint                  radius);

static gboolean         wavelet_decompose_dialog (void);
static void       wavelet_scale_entry_update_int (LigmaLabelSpin        *entry,
                                                  gint                 *value);


G_DEFINE_TYPE (Wavelet, wavelet, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (WAVELET_TYPE)
DEFINE_STD_SET_I18N


static WaveletDecomposeParams wavelet_params =
{
  5,     /* default scales */
  TRUE,  /* create group */
  FALSE  /* do not add mask by default */
};


static void
wavelet_class_init (WaveletClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = wavelet_query_procedures;
  plug_in_class->create_procedure = wavelet_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
wavelet_init (Wavelet *wavelet)
{
}

static GList *
wavelet_query_procedures (LigmaPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static LigmaProcedure *
wavelet_create_procedure (LigmaPlugIn  *plug_in,
                           const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = ligma_image_procedure_new (plug_in, name,
                                            LIGMA_PDB_PROC_TYPE_PLUGIN,
                                            wavelet_run, NULL, NULL);

      ligma_procedure_set_image_types (procedure, "RGB*, GRAY*");
      ligma_procedure_set_sensitivity_mask (procedure,
                                           LIGMA_PROCEDURE_SENSITIVE_DRAWABLE);

      ligma_procedure_set_menu_label (procedure, _("_Wavelet-decompose..."));
      ligma_procedure_add_menu_path (procedure, "<Image>/Filters/Enhance");

      ligma_procedure_set_documentation (procedure,
                                        _("Wavelet decompose"),
                                        "Compute and render wavelet scales",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Miroslav Talasek <miroslav.talasek@seznam.cz>",
                                      "Miroslav Talasek <miroslav.talasek@seznam.cz>",
                                      "19january 2017");

      LIGMA_PROC_ARG_INT (procedure, "scales",
                         "Scales",
                         "Number of scales",
                         1, 7, 5,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_BOOLEAN (procedure, "create-group",
                             "Create group",
                             "Create a layer group to store the "
                             "decomposition",
                             TRUE,
                             G_PARAM_READWRITE);

      LIGMA_PROC_ARG_BOOLEAN (procedure, "create-masks",
                             "Create masks",
                             "Add a layer mask to each scales layer",
                             FALSE,
                             G_PARAM_READWRITE);
    }

  return procedure;
}

static LigmaValueArray *
wavelet_run (LigmaProcedure        *procedure,
              LigmaRunMode           run_mode,
              LigmaImage            *image,
              gint                  n_drawables,
              LigmaDrawable        **drawables,
              const LigmaValueArray *args,
              gpointer              run_data)
{
  LigmaLayer    **scale_layers;
  LigmaLayer     *new_scale;
  LigmaLayer     *parent             = NULL;
  LigmaDrawable  *drawable;
  LigmaLayerMode  grain_extract_mode = LIGMA_LAYER_MODE_GRAIN_EXTRACT;
  LigmaLayerMode  grain_merge_mode   = LIGMA_LAYER_MODE_GRAIN_MERGE;
  gint           id;

  gegl_init (NULL, NULL);

  if (n_drawables != 1)
    {
      GError *error = NULL;

      g_set_error (&error, LIGMA_PLUG_IN_ERROR, 0,
                   _("Procedure '%s' only works with one drawable."),
                   ligma_procedure_get_name (procedure));

      return ligma_procedure_new_return_values (procedure,
                                               LIGMA_PDB_CALLING_ERROR,
                                               error);
    }
  else
    {
      drawable = drawables[0];
    }

  switch (run_mode)
    {
    case LIGMA_RUN_INTERACTIVE:
      ligma_get_data (PLUG_IN_PROC, &wavelet_params);

      if (! wavelet_decompose_dialog ())
        return ligma_procedure_new_return_values (procedure,
                                                 LIGMA_PDB_CANCEL,
                                                 NULL);
      break;

    case LIGMA_RUN_NONINTERACTIVE:
      wavelet_params.scales       = LIGMA_VALUES_GET_INT     (args, 0);
      wavelet_params.create_group = LIGMA_VALUES_GET_BOOLEAN (args, 1);
      wavelet_params.create_masks = LIGMA_VALUES_GET_BOOLEAN (args, 2);
      break;

    case LIGMA_RUN_WITH_LAST_VALS:
      ligma_get_data (PLUG_IN_PROC, &wavelet_params);
      break;

    default:
      break;
    }

  ligma_progress_init (_("Wavelet-Decompose"));

  ligma_image_undo_group_start (image);

  ligma_image_freeze_layers (image);

  if (wavelet_params.create_group)
    {
      parent = ligma_layer_group_new (image);

      ligma_item_set_name (LIGMA_ITEM (parent), _("Decomposition"));
      ligma_item_set_visible (LIGMA_ITEM (parent), FALSE);
      ligma_image_insert_layer (image, parent,
                               LIGMA_LAYER (ligma_item_get_parent (LIGMA_ITEM (drawable))),
                               ligma_image_get_item_position (image,
                                                             LIGMA_ITEM (drawable)));
    }

  scale_layers = g_new (LigmaLayer *, wavelet_params.scales);
  new_scale = ligma_layer_copy (LIGMA_LAYER (drawable));
  ligma_image_insert_layer (image, new_scale, parent,
                           ligma_image_get_item_position (image,
                                                         LIGMA_ITEM (drawable)));

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
  switch (ligma_image_get_precision (image))
    {
    case LIGMA_PRECISION_U8_LINEAR:
    case LIGMA_PRECISION_U8_NON_LINEAR:
    case LIGMA_PRECISION_U8_PERCEPTUAL:
    case LIGMA_PRECISION_U16_LINEAR:
    case LIGMA_PRECISION_U16_NON_LINEAR:
    case LIGMA_PRECISION_U16_PERCEPTUAL:
    case LIGMA_PRECISION_U32_LINEAR:
    case LIGMA_PRECISION_U32_NON_LINEAR:
    case LIGMA_PRECISION_U32_PERCEPTUAL:
      grain_extract_mode = LIGMA_LAYER_MODE_GRAIN_EXTRACT_LEGACY;
      grain_merge_mode   = LIGMA_LAYER_MODE_GRAIN_MERGE_LEGACY;
      break;

    case LIGMA_PRECISION_HALF_LINEAR:
    case LIGMA_PRECISION_HALF_NON_LINEAR:
    case LIGMA_PRECISION_HALF_PERCEPTUAL:
    case LIGMA_PRECISION_FLOAT_LINEAR:
    case LIGMA_PRECISION_FLOAT_NON_LINEAR:
    case LIGMA_PRECISION_FLOAT_PERCEPTUAL:
    case LIGMA_PRECISION_DOUBLE_LINEAR:
    case LIGMA_PRECISION_DOUBLE_NON_LINEAR:
    case LIGMA_PRECISION_DOUBLE_PERCEPTUAL:
      grain_extract_mode = LIGMA_LAYER_MODE_GRAIN_EXTRACT;
      grain_merge_mode   = LIGMA_LAYER_MODE_GRAIN_MERGE;
      break;
    }

  for (id = 0 ; id < wavelet_params.scales; id++)
    {
      LigmaLayer *blur;
      LigmaLayer *tmp;
      gchar      scale_name[20];

      ligma_progress_update ((gdouble) id / (gdouble) wavelet_params.scales);

      scale_layers[id] = new_scale;

      g_snprintf (scale_name, sizeof (scale_name), _("Scale %d"), id + 1);
      ligma_item_set_name (LIGMA_ITEM (new_scale), scale_name);

      tmp = ligma_layer_copy (new_scale);
      ligma_image_insert_layer (image, tmp, parent,
                               ligma_image_get_item_position (image,
                                                             LIGMA_ITEM (new_scale)));
      wavelet_blur (LIGMA_DRAWABLE (tmp), pow (2.0, id));

      blur = ligma_layer_copy (tmp);
      ligma_image_insert_layer (image, blur, parent,
                               ligma_image_get_item_position (image,
                                                             LIGMA_ITEM (tmp)));

      ligma_layer_set_mode (tmp, grain_extract_mode);
      new_scale = ligma_image_merge_down (image, tmp,
                                         LIGMA_EXPAND_AS_NECESSARY);
      scale_layers[id] = new_scale;

      ligma_item_set_visible (LIGMA_ITEM (new_scale), FALSE);

      new_scale = blur;
    }

  ligma_item_set_name (LIGMA_ITEM (new_scale), _("Residual"));

  for (id = 0; id < wavelet_params.scales; id++)
    {
      ligma_image_reorder_item (image, LIGMA_ITEM (scale_layers[id]),
                               LIGMA_ITEM (parent),
                               ligma_image_get_item_position (image,
                                                             LIGMA_ITEM (new_scale)));
      ligma_layer_set_mode (scale_layers[id], grain_merge_mode);

      if (wavelet_params.create_masks)
        {
          LigmaLayerMask *mask = ligma_layer_create_mask (scale_layers[id],
                                                        LIGMA_ADD_MASK_WHITE);
          ligma_layer_add_mask (scale_layers[id], mask);
        }

      ligma_item_set_visible (LIGMA_ITEM (scale_layers[id]), TRUE);
    }

  if (wavelet_params.create_group)
    ligma_item_set_visible (LIGMA_ITEM (parent), TRUE);

  g_free (scale_layers);

  ligma_image_thaw_layers (image);

  ligma_image_undo_group_end (image);

  ligma_progress_update (1.0);

  ligma_displays_flush ();

  if (run_mode == LIGMA_RUN_INTERACTIVE)
    ligma_set_data (PLUG_IN_PROC,
                   &wavelet_params, sizeof (WaveletDecomposeParams));

  gegl_exit ();

  return ligma_procedure_new_return_values (procedure, LIGMA_PDB_SUCCESS, NULL);
}

static void
wavelet_blur (LigmaDrawable *drawable,
              gint          radius)
{
  gint x, y, width, height;

  if (ligma_drawable_mask_intersect (drawable, &x, &y, &width, &height))
    {
      GeglBuffer *buffer = ligma_drawable_get_buffer (drawable);
      GeglBuffer *shadow = ligma_drawable_get_shadow_buffer (drawable);

      gegl_render_op (buffer, shadow,
                      "gegl:wavelet-blur",
                      "radius", (gdouble) radius,
                      NULL);

      gegl_buffer_flush (shadow);
      ligma_drawable_merge_shadow (drawable, FALSE);
      ligma_drawable_update (drawable, x, y, width, height);
      g_object_unref (buffer);
      g_object_unref (shadow);
    }
}

static gboolean
wavelet_decompose_dialog (void)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *button;
  GtkWidget *scale;
  gboolean   run;

  ligma_ui_init (PLUG_IN_BINARY);

  dialog = ligma_dialog_new (_("Wavelet decompose"), PLUG_IN_ROLE,
                            NULL, 0,
                            ligma_standard_help_func, PLUG_IN_PROC,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_OK"),     GTK_RESPONSE_OK,

                            NULL);

  ligma_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  ligma_window_set_transient (GTK_WINDOW (dialog));

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  /* scales */

  scale = ligma_scale_entry_new (_("Scales:"), wavelet_params.scales, 1.0, 7.0, 0);
  gtk_box_pack_start (GTK_BOX (main_vbox), scale, FALSE, FALSE, 6);
  gtk_widget_show (scale);

  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (wavelet_scale_entry_update_int),
                    &wavelet_params.scales);

  /* create group layer */

  button = gtk_check_button_new_with_mnemonic (_("Create a layer group to store the decomposition"));
  gtk_box_pack_start (GTK_BOX (main_vbox), button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                wavelet_params.create_group);
  gtk_widget_show (button);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (ligma_toggle_button_update),
                    &wavelet_params.create_group);

  /* create layer masks */

  button = gtk_check_button_new_with_mnemonic (_("Add a layer mask to each scales layers"));
  gtk_box_pack_start (GTK_BOX (main_vbox), button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                wavelet_params.create_masks);
  gtk_widget_show (button);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (ligma_toggle_button_update),
                    &wavelet_params.create_masks);

  gtk_widget_show (dialog);

  run = (ligma_dialog_run (LIGMA_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

static void
wavelet_scale_entry_update_int (LigmaLabelSpin *entry,
                                gint          *value)
{
  *value = (int) ligma_label_spin_get_value (entry);
}
