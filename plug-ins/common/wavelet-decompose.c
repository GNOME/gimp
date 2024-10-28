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
#define WAVELET(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), WAVELET_TYPE, Wavelet))

GType                   wavelet_get_type         (void) G_GNUC_CONST;

static GList          * wavelet_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * wavelet_create_procedure (GimpPlugIn           *plug_in,
                                                  const gchar          *name);

static GimpValueArray * wavelet_run              (GimpProcedure        *procedure,
                                                  GimpRunMode           run_mode,
                                                  GimpImage            *image,
                                                  GimpDrawable        **drawables,
                                                  GimpProcedureConfig  *config,
                                                  gpointer              run_data);

static void             wavelet_blur             (GimpDrawable         *drawable,
                                                  gint                  radius);

static gboolean         wavelet_decompose_dialog (GimpProcedure        *procedure,
                                                  GObject              *config);


G_DEFINE_TYPE (Wavelet, wavelet, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (WAVELET_TYPE)
DEFINE_STD_SET_I18N




static void
wavelet_class_init (WaveletClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = wavelet_query_procedures;
  plug_in_class->create_procedure = wavelet_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
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
      gimp_procedure_set_sensitivity_mask (procedure,
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLE);

      gimp_procedure_set_menu_label (procedure, _("_Wavelet-decompose..."));
      gimp_procedure_add_menu_path (procedure, "<Image>/Filters/Enhance");

      gimp_procedure_set_documentation (procedure,
                                        _("Wavelet decompose"),
                                        "Compute and render wavelet scales",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Miroslav Talasek <miroslav.talasek@seznam.cz>",
                                      "Miroslav Talasek <miroslav.talasek@seznam.cz>",
                                      "19 January 2017");

      gimp_procedure_add_int_argument (procedure, "scales",
                                       _("Scal_es"),
                                       _("Number of scales"),
                                       1, 7, 5,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "create-group",
                                           _("Create a layer group to store the "
                                             "_decomposition"),
                                           _("Create a layer group to store the "
                                           "decomposition"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "create-masks",
                                           _("_Add a layer mask to each scales layer"),
                                           _("Add a layer mask to each scales layer"),
                                           FALSE,
                                           G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
wavelet_run (GimpProcedure        *procedure,
              GimpRunMode           run_mode,
              GimpImage            *image,
              GimpDrawable        **drawables,
              GimpProcedureConfig  *config,
              gpointer              run_data)
{
  GimpLayer     **scale_layers;
  GimpLayer      *new_scale;
  GimpLayer      *parent             = NULL;
  GimpDrawable   *drawable;
  GimpLayerMode   grain_extract_mode = GIMP_LAYER_MODE_GRAIN_EXTRACT;
  GimpLayerMode   grain_merge_mode   = GIMP_LAYER_MODE_GRAIN_MERGE;
  gint            id;
  gint            scales;
  gboolean        create_group;
  gboolean        create_masks;

  gegl_init (NULL, NULL);

  if (gimp_core_object_array_get_length ((GObject **) drawables) != 1)
    {
      GError *error = NULL;

      g_set_error (&error, GIMP_PLUG_IN_ERROR, 0,
                   _("Procedure '%s' only works with one drawable."),
                   gimp_procedure_get_name (procedure));

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_CALLING_ERROR,
                                               error);
    }
  else
    {
      drawable = drawables[0];
    }

  if (run_mode == GIMP_RUN_INTERACTIVE && ! wavelet_decompose_dialog (procedure, G_OBJECT (config)))
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_CANCEL,
                                             NULL);

  g_object_get (config,
                "scales",       &scales,
                "create-group", &create_group,
                "create-masks", &create_masks,
                NULL);

  gimp_progress_init (_("Wavelet-Decompose"));

  gimp_image_undo_group_start (image);

  gimp_image_freeze_layers (image);

  if (create_group)
    {
      parent = GIMP_LAYER (gimp_group_layer_new (image, _("Decomposition")));

      gimp_item_set_visible (GIMP_ITEM (parent), FALSE);
      gimp_image_insert_layer (image, parent,
                               GIMP_LAYER (gimp_item_get_parent (GIMP_ITEM (drawable))),
                               gimp_image_get_item_position (image,
                                                             GIMP_ITEM (drawable)));
    }

  scale_layers = g_new (GimpLayer *, scales);
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

  for (id = 0 ; id < scales; id++)
    {
      GimpLayer *blur;
      GimpLayer *tmp;
      gchar      scale_name[20];

      gimp_progress_update ((gdouble) id / (gdouble) scales);

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

  for (id = 0; id < scales; id++)
    {
      gimp_image_reorder_item (image, GIMP_ITEM (scale_layers[id]),
                               GIMP_ITEM (parent),
                               gimp_image_get_item_position (image,
                                                             GIMP_ITEM (new_scale)));
      gimp_layer_set_mode (scale_layers[id], grain_merge_mode);

      if (create_masks)
        {
          GimpLayerMask *mask = gimp_layer_create_mask (scale_layers[id],
                                                        GIMP_ADD_MASK_WHITE);
          gimp_layer_add_mask (scale_layers[id], mask);
        }

      gimp_item_set_visible (GIMP_ITEM (scale_layers[id]), TRUE);
    }

  if (create_group)
    gimp_item_set_visible (GIMP_ITEM (parent), TRUE);

  g_free (scale_layers);

  gimp_image_thaw_layers (image);

  gimp_image_undo_group_end (image);

  gimp_progress_update (1.0);

  gimp_displays_flush ();

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
wavelet_decompose_dialog (GimpProcedure *procedure,
                          GObject       *config)
{
  GtkWidget *dialog;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY);

  dialog = gimp_procedure_dialog_new (procedure,
                                      GIMP_PROCEDURE_CONFIG (config),
                                      _("Wavelet decompose"));

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  /* scales */
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "scales", 1.0);

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                              NULL);

  gtk_widget_show (dialog);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}
