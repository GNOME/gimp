/* LIGMA - The GNU Image Manipulation Program
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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "actions-types.h"

#include "config/ligmaguiconfig.h"

#include "gegl/ligma-babl.h"

#include "core/ligma.h"
#include "core/ligmachannel.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-color-profile.h"
#include "core/ligmaitemstack.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"

#include "widgets/ligmaactiongroup.h"
#include "widgets/ligmahelp-ids.h"

#include "actions.h"
#include "image-actions.h"
#include "image-commands.h"

#include "ligma-intl.h"


static const LigmaActionEntry image_actions[] =
{
  { "image-menubar", NULL,
    NC_("image-action", "Image Menu"), NULL, NULL, NULL,
    LIGMA_HELP_IMAGE_WINDOW },

  { "image-popup", NULL,
    NC_("image-action", "Image Menu"), NULL, NULL, NULL,
    LIGMA_HELP_IMAGE_WINDOW },

  { "image-menu",                  NULL, NC_("image-action", "_Image")     },
  { "image-mode-menu",             NULL, NC_("image-action", "_Mode")      },
  { "image-precision-menu",        NULL, NC_("image-action", "_Encoding")  },
  { "image-color-management-menu", NULL, NC_("image-action",
                                             "Color Ma_nagement")          },
  { "image-transform-menu",        NULL, NC_("image-action", "_Transform") },
  { "image-guides-menu",           NULL, NC_("image-action", "_Guides")    },
  { "image-metadata-menu",         NULL, NC_("image-action", "Meta_data")  },

  { "colors-menu",              NULL, NC_("image-action", "_Colors")       },
  { "colors-info-menu",         NULL, NC_("image-action", "I_nfo")         },
  { "colors-auto-menu",         NULL, NC_("image-action", "_Auto")         },
  { "colors-map-menu",          NULL, NC_("image-action", "_Map")          },
  { "colors-tone-mapping-menu", NULL, NC_("image-action", "_Tone Mapping") },
  { "colors-components-menu",   NULL, NC_("image-action", "C_omponents")   },
  { "colors-desaturate-menu",   NULL, NC_("image-action", "D_esaturate")   },

  { "image-new", LIGMA_ICON_DOCUMENT_NEW,
    NC_("image-action", "_New..."), "<primary>N",
    NC_("image-action", "Create a new image"),
    image_new_cmd_callback,
    LIGMA_HELP_FILE_NEW },

  { "image-duplicate", LIGMA_ICON_OBJECT_DUPLICATE,
    NC_("image-action", "_Duplicate"), "<primary>D",
    NC_("image-action", "Create a duplicate of this image"),
    image_duplicate_cmd_callback,
    LIGMA_HELP_IMAGE_DUPLICATE },

  { "image-color-profile-assign", NULL,
    NC_("image-action", "_Assign Color Profile..."), NULL,
    NC_("image-action", "Set a color profile on the image"),
    image_color_profile_assign_cmd_callback,
    LIGMA_HELP_IMAGE_COLOR_PROFILE_ASSIGN },

  { "image-color-profile-convert", NULL,
    NC_("image-action", "_Convert to Color Profile..."), NULL,
    NC_("image-action", "Apply a color profile to the image"),
    image_color_profile_convert_cmd_callback,
    LIGMA_HELP_IMAGE_COLOR_PROFILE_CONVERT },

  { "image-color-profile-discard", NULL,
    NC_("image-action", "_Discard Color Profile"), NULL,
    NC_("image-action", "Remove the image's color profile"),
    image_color_profile_discard_cmd_callback,
    LIGMA_HELP_IMAGE_COLOR_PROFILE_DISCARD },

  { "image-softproof-profile", NULL,
    NC_("image-action", "_Softproof Profile..."), NULL,
    NC_("image-action", "Set the softproofing profile"),
    image_softproof_profile_cmd_callback,
    LIGMA_HELP_VIEW_COLOR_MANAGEMENT },

  { "image-color-profile-save", NULL,
    NC_("image-action", "_Save Color Profile to File..."), NULL,
    NC_("image-action", "Save the image's color profile to an ICC file"),
    image_color_profile_save_cmd_callback,
    LIGMA_HELP_IMAGE_COLOR_PROFILE_SAVE },

  { "image-softproof-intent-menu", NULL,
    NC_("image-action", "Soft-Proofing Re_ndering Intent") },

  { "image-resize", LIGMA_ICON_OBJECT_RESIZE,
    NC_("image-action", "Can_vas Size..."), NULL,
    NC_("image-action", "Adjust the image dimensions"),
    image_resize_cmd_callback,
    LIGMA_HELP_IMAGE_RESIZE },

  { "image-resize-to-layers", NULL,
    NC_("image-action", "Fit Canvas to L_ayers"), NULL,
    NC_("image-action", "Resize the image to enclose all layers"),
    image_resize_to_layers_cmd_callback,
    LIGMA_HELP_IMAGE_RESIZE_TO_LAYERS },

  { "image-resize-to-selection", NULL,
    NC_("image-action", "F_it Canvas to Selection"), NULL,
    NC_("image-action", "Resize the image to the extents of the selection"),
    image_resize_to_selection_cmd_callback,
    LIGMA_HELP_IMAGE_RESIZE_TO_SELECTION },

  { "image-print-size", LIGMA_ICON_DOCUMENT_PRINT_RESOLUTION,
    NC_("image-action", "_Print Size..."), NULL,
    NC_("image-action", "Adjust the print resolution"),
    image_print_size_cmd_callback,
    LIGMA_HELP_IMAGE_PRINT_SIZE },

  { "image-scale", LIGMA_ICON_OBJECT_SCALE,
    NC_("image-action", "_Scale Image..."), NULL,
    NC_("image-action", "Change the size of the image content"),
    image_scale_cmd_callback,
    LIGMA_HELP_IMAGE_SCALE },

  { "image-crop-to-selection", LIGMA_ICON_TOOL_CROP,
    NC_("image-action", "_Crop to Selection"), NULL,
    NC_("image-action", "Crop the image to the extents of the selection"),
    image_crop_to_selection_cmd_callback,
    LIGMA_HELP_IMAGE_CROP },

  { "image-crop-to-content", LIGMA_ICON_TOOL_CROP,
    NC_("image-action", "Crop to C_ontent"), NULL,
    NC_("image-action", "Crop the image to the extents of its content (remove empty borders from the image)"),
    image_crop_to_content_cmd_callback,
    LIGMA_HELP_IMAGE_CROP },

  { "image-merge-layers", NULL,
    NC_("image-action", "Merge Visible _Layers..."), "<primary>M",
    NC_("image-action", "Merge all visible layers into one layer"),
    image_merge_layers_cmd_callback,
    LIGMA_HELP_IMAGE_MERGE_LAYERS },

  { "image-flatten", NULL,
    NC_("image-action", "_Flatten Image"), NULL,
    NC_("image-action", "Merge all layers into one and remove transparency"),
    image_flatten_image_cmd_callback,
    LIGMA_HELP_IMAGE_FLATTEN },

  { "image-configure-grid", LIGMA_ICON_GRID,
    NC_("image-action", "Configure G_rid..."), NULL,
    NC_("image-action", "Configure the grid for this image"),
    image_configure_grid_cmd_callback,
    LIGMA_HELP_IMAGE_GRID },

  { "image-properties", "dialog-information",
    NC_("image-action", "Image Pr_operties"), "<alt>Return",
    NC_("image-action", "Display information about this image"),
    image_properties_cmd_callback,
    LIGMA_HELP_IMAGE_PROPERTIES }
};

static const LigmaToggleActionEntry image_toggle_actions[] =
{
  { "image-color-profile-use-srgb", NULL,
    NC_("image-action", "Use _sRGB Profile"), NULL,
    NC_("image-action", "Temporarily use an sRGB profile for the image. "
        "This is the same as discarding the image's color profile, but "
        "allows to easily restore the profile."),
    image_color_profile_use_srgb_cmd_callback,
    TRUE,
    LIGMA_HELP_IMAGE_COLOR_PROFILE_USE_SRGB },

  { "image-softproof-black-point-compensation", NULL,
    NC_("image-action", "_Black Point Compensation"), NULL,
    NC_("image-action", "Use black point compensation for soft-proofing"),
    image_softproof_bpc_cmd_callback,
    TRUE,
    LIGMA_HELP_VIEW_COLOR_MANAGEMENT }
};

static const LigmaRadioActionEntry image_convert_base_type_actions[] =
{
  { "image-convert-rgb", LIGMA_ICON_CONVERT_RGB,
    NC_("image-convert-action", "_RGB"), NULL,
    NC_("image-convert-action", "Convert the image to the RGB colorspace"),
    LIGMA_RGB, LIGMA_HELP_IMAGE_CONVERT_RGB },

  { "image-convert-grayscale", LIGMA_ICON_CONVERT_GRAYSCALE,
    NC_("image-convert-action", "_Grayscale"), NULL,
    NC_("image-convert-action", "Convert the image to grayscale"),
    LIGMA_GRAY, LIGMA_HELP_IMAGE_CONVERT_GRAYSCALE },

  { "image-convert-indexed", LIGMA_ICON_CONVERT_INDEXED,
    NC_("image-convert-action", "_Indexed..."), NULL,
    NC_("image-convert-action", "Convert the image to indexed colors"),
    LIGMA_INDEXED, LIGMA_HELP_IMAGE_CONVERT_INDEXED }
};

static const LigmaRadioActionEntry image_convert_precision_actions[] =
{
  { "image-convert-u8", NULL,
    NC_("image-convert-action", "8 bit integer"), NULL,
    NC_("image-convert-action",
        "Convert the image to 8 bit integer"),
    LIGMA_COMPONENT_TYPE_U8, LIGMA_HELP_IMAGE_CONVERT_U8 },

  { "image-convert-u16", NULL,
    NC_("image-convert-action", "16 bit integer"), NULL,
    NC_("image-convert-action",
        "Convert the image to 16 bit integer"),
    LIGMA_COMPONENT_TYPE_U16, LIGMA_HELP_IMAGE_CONVERT_U16 },

  { "image-convert-u32", NULL,
    NC_("image-convert-action", "32 bit integer"), NULL,
    NC_("image-convert-action",
        "Convert the image to 32 bit integer"),
    LIGMA_COMPONENT_TYPE_U32, LIGMA_HELP_IMAGE_CONVERT_U32 },

  { "image-convert-half", NULL,
    NC_("image-convert-action", "16 bit floating point"), NULL,
    NC_("image-convert-action",
        "Convert the image to 16 bit floating point"),
    LIGMA_COMPONENT_TYPE_HALF, LIGMA_HELP_IMAGE_CONVERT_HALF },

  { "image-convert-float", NULL,
    NC_("image-convert-action", "32 bit floating point"), NULL,
    NC_("image-convert-action",
        "Convert the image to 32 bit floating point"),
    LIGMA_COMPONENT_TYPE_FLOAT, LIGMA_HELP_IMAGE_CONVERT_FLOAT },

  { "image-convert-double", NULL,
    NC_("image-convert-action", "64 bit floating point"), NULL,
    NC_("image-convert-action",
        "Convert the image to 64 bit floating point"),
    LIGMA_COMPONENT_TYPE_DOUBLE, LIGMA_HELP_IMAGE_CONVERT_DOUBLE }
};

static const LigmaRadioActionEntry image_convert_trc_actions[] =
{
  { "image-convert-linear", NULL,
    NC_("image-convert-action", "Linear light"), NULL,
    NC_("image-convert-action",
        "Convert the image to linear light"),
    LIGMA_TRC_LINEAR, LIGMA_HELP_IMAGE_CONVERT_GAMMA },

  { "image-convert-non-linear", NULL,
    NC_("image-convert-action", "Non-Linear"), NULL,
    NC_("image-convert-action",
        "Convert the image to non-linear gamma from the color profile"),
    LIGMA_TRC_NON_LINEAR, LIGMA_HELP_IMAGE_CONVERT_GAMMA },

  { "image-convert-perceptual", NULL,
    NC_("image-convert-action", "Perceptual (sRGB)"), NULL,
    NC_("image-convert-action",
        "Convert the image to perceptual (sRGB) gamma"),
    LIGMA_TRC_PERCEPTUAL, LIGMA_HELP_IMAGE_CONVERT_GAMMA }
};

static const LigmaEnumActionEntry image_flip_actions[] =
{
  { "image-flip-horizontal", LIGMA_ICON_OBJECT_FLIP_HORIZONTAL,
    NC_("image-action", "Flip _Horizontally"), NULL,
    NC_("image-action", "Flip image horizontally"),
    LIGMA_ORIENTATION_HORIZONTAL, FALSE,
    LIGMA_HELP_IMAGE_FLIP_HORIZONTAL },

  { "image-flip-vertical", LIGMA_ICON_OBJECT_FLIP_VERTICAL,
    NC_("image-action", "Flip _Vertically"), NULL,
    NC_("image-action", "Flip image vertically"),
    LIGMA_ORIENTATION_VERTICAL, FALSE,
    LIGMA_HELP_IMAGE_FLIP_VERTICAL }
};

static const LigmaEnumActionEntry image_rotate_actions[] =
{
  { "image-rotate-90", LIGMA_ICON_OBJECT_ROTATE_90,
    NC_("image-action", "Rotate 90° _clockwise"), NULL,
    NC_("image-action", "Rotate the image 90 degrees to the right"),
    LIGMA_ROTATE_90, FALSE,
    LIGMA_HELP_IMAGE_ROTATE_90 },

  { "image-rotate-180", LIGMA_ICON_OBJECT_ROTATE_180,
    NC_("image-action", "Rotate _180°"), NULL,
    NC_("image-action", "Turn the image upside-down"),
    LIGMA_ROTATE_180, FALSE,
    LIGMA_HELP_IMAGE_ROTATE_180 },

  { "image-rotate-270", LIGMA_ICON_OBJECT_ROTATE_270,
    NC_("image-action", "Rotate 90° counter-clock_wise"), NULL,
    NC_("image-action", "Rotate the image 90 degrees to the left"),
    LIGMA_ROTATE_270, FALSE,
    LIGMA_HELP_IMAGE_ROTATE_270 }
};

static const LigmaRadioActionEntry image_softproof_intent_actions[] =
{
  { "image-softproof-intent-perceptual", NULL,
    NC_("image-action", "_Perceptual"), NULL,
    NC_("image-action", "Soft-proofing rendering intent is perceptual"),
    LIGMA_COLOR_RENDERING_INTENT_PERCEPTUAL,
    LIGMA_HELP_VIEW_COLOR_MANAGEMENT },

  { "image-softproof-intent-relative-colorimetric", NULL,
    NC_("image-action", "_Relative Colorimetric"), NULL,
    NC_("image-action", "Soft-proofing rendering intent is relative colorimetric"),
    LIGMA_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
    LIGMA_HELP_VIEW_COLOR_MANAGEMENT },

  { "image-softproof-intent-saturation", NULL,
    NC_("image-action", "_Saturation"), NULL,
    NC_("image-action", "Soft-proofing rendering intent is saturation"),
    LIGMA_COLOR_RENDERING_INTENT_SATURATION,
    LIGMA_HELP_VIEW_COLOR_MANAGEMENT },

  { "image-softproof-intent-absolute-colorimetric", NULL,
    NC_("image-action", "_Absolute Colorimetric"), NULL,
    NC_("image-action", "Soft-proofing rendering intent is absolute colorimetric"),
    LIGMA_COLOR_RENDERING_INTENT_ABSOLUTE_COLORIMETRIC,
    LIGMA_HELP_VIEW_COLOR_MANAGEMENT }
};


void
image_actions_setup (LigmaActionGroup *group)
{
  ligma_action_group_add_actions (group, "image-action",
                                 image_actions,
                                 G_N_ELEMENTS (image_actions));

  ligma_action_group_add_toggle_actions (group, "image-action",
                                        image_toggle_actions,
                                        G_N_ELEMENTS (image_toggle_actions));

  ligma_action_group_add_radio_actions (group, "image-action",
                                       image_softproof_intent_actions,
                                       G_N_ELEMENTS (image_softproof_intent_actions),
                                       NULL,
                                       LIGMA_COLOR_MANAGEMENT_DISPLAY,
                                       image_softproof_intent_cmd_callback);

  ligma_action_group_add_radio_actions (group, "image-convert-action",
                                       image_convert_base_type_actions,
                                       G_N_ELEMENTS (image_convert_base_type_actions),
                                       NULL, 0,
                                       image_convert_base_type_cmd_callback);

  ligma_action_group_add_radio_actions (group, "image-convert-action",
                                       image_convert_precision_actions,
                                       G_N_ELEMENTS (image_convert_precision_actions),
                                       NULL, 0,
                                       image_convert_precision_cmd_callback);

  ligma_action_group_add_radio_actions (group, "image-convert-action",
                                       image_convert_trc_actions,
                                       G_N_ELEMENTS (image_convert_trc_actions),
                                       NULL, 0,
                                       image_convert_trc_cmd_callback);

  ligma_action_group_add_enum_actions (group, "image-action",
                                      image_flip_actions,
                                      G_N_ELEMENTS (image_flip_actions),
                                      image_flip_cmd_callback);

  ligma_action_group_add_enum_actions (group, "image-action",
                                      image_rotate_actions,
                                      G_N_ELEMENTS (image_rotate_actions),
                                      image_rotate_cmd_callback);

#define SET_ALWAYS_SHOW_IMAGE(action,show) \
        ligma_action_group_set_action_always_show_image (group, action, show)

  SET_ALWAYS_SHOW_IMAGE ("image-rotate-90",  TRUE);
  SET_ALWAYS_SHOW_IMAGE ("image-rotate-180", TRUE);
  SET_ALWAYS_SHOW_IMAGE ("image-rotate-270", TRUE);

#undef SET_ALWAYS_SHOW_IMAGE
}

void
image_actions_update (LigmaActionGroup *group,
                      gpointer         data)
{
  LigmaImage        *image          = action_data_get_image (data);
  gboolean          is_indexed     = FALSE;
  gboolean          is_u8_gamma    = FALSE;
  gboolean          is_double      = FALSE;
  gboolean          aux            = FALSE;
  gboolean          lp             = FALSE;
  gboolean          sel            = FALSE;
  gboolean          groups         = FALSE;
  gboolean          profile_srgb   = FALSE;
  gboolean          profile_hidden = FALSE;
  gboolean          profile        = FALSE;
  gboolean          s_bpc          = FALSE;

#define SET_LABEL(action,label) \
        ligma_action_group_set_action_label (group, action, (label))
#define SET_SENSITIVE(action,condition) \
        ligma_action_group_set_action_sensitive (group, action, (condition) != 0, NULL)
#define SET_ACTIVE(action,condition) \
        ligma_action_group_set_action_active (group, action, (condition) != 0)
#define SET_VISIBLE(action,condition) \
        ligma_action_group_set_action_visible (group, action, (condition) != 0)

  if (image)
    {
      LigmaContainer     *layers;
      const gchar       *action = NULL;
      LigmaImageBaseType  base_type;
      LigmaPrecision      precision;
      LigmaComponentType  component_type;
      LigmaTRCType        trc;

      base_type      = ligma_image_get_base_type (image);
      precision      = ligma_image_get_precision (image);
      component_type = ligma_image_get_component_type (image);
      trc            = ligma_babl_format_get_trc
                         (ligma_image_get_layer_format (image, FALSE));

      switch (base_type)
        {
        case LIGMA_RGB:     action = "image-convert-rgb";       break;
        case LIGMA_GRAY:    action = "image-convert-grayscale"; break;
        case LIGMA_INDEXED: action = "image-convert-indexed";   break;
        }

      SET_ACTIVE (action, TRUE);

      switch (component_type)
        {
        case LIGMA_COMPONENT_TYPE_U8:     action = "image-convert-u8";     break;
        case LIGMA_COMPONENT_TYPE_U16:    action = "image-convert-u16";    break;
        case LIGMA_COMPONENT_TYPE_U32:    action = "image-convert-u32";    break;
        case LIGMA_COMPONENT_TYPE_HALF:   action = "image-convert-half";   break;
        case LIGMA_COMPONENT_TYPE_FLOAT:  action = "image-convert-float";  break;
        case LIGMA_COMPONENT_TYPE_DOUBLE: action = "image-convert-double"; break;
        }

      SET_ACTIVE (action, TRUE);

      switch (trc)
        {
        case LIGMA_TRC_LINEAR:
          action = "image-convert-linear";
          SET_VISIBLE ("image-convert-perceptual", FALSE);
          break;

        case LIGMA_TRC_NON_LINEAR:
          action = "image-convert-non-linear";
          SET_VISIBLE ("image-convert-perceptual", FALSE);
          break;

        case LIGMA_TRC_PERCEPTUAL:
          action = "image-convert-perceptual";
          SET_VISIBLE ("image-convert-perceptual", TRUE);
          break;
        }

      SET_ACTIVE (action, TRUE);

      is_indexed  = (base_type == LIGMA_INDEXED);
      is_u8_gamma = (precision == LIGMA_PRECISION_U8_NON_LINEAR);
      is_double   = (component_type == LIGMA_COMPONENT_TYPE_DOUBLE);
      aux         = (ligma_image_get_selected_channels (image) != NULL);
      lp          = ! ligma_image_is_empty (image);
      sel         = ! ligma_channel_is_empty (ligma_image_get_mask (image));

      layers = ligma_image_get_layers (image);

      groups = ! ligma_item_stack_is_flat (LIGMA_ITEM_STACK (layers));

      profile_srgb = ligma_image_get_use_srgb_profile (image, &profile_hidden);
      profile      = (ligma_image_get_color_profile (image) != NULL);

      switch (ligma_image_get_simulation_intent (image))
        {
        case LIGMA_COLOR_RENDERING_INTENT_PERCEPTUAL:
          action = "image-softproof-intent-perceptual";
          break;

        case LIGMA_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC:
          action = "image-softproof-intent-relative-colorimetric";
          break;

        case LIGMA_COLOR_RENDERING_INTENT_SATURATION:
          action = "image-softproof-intent-saturation";
          break;

        case LIGMA_COLOR_RENDERING_INTENT_ABSOLUTE_COLORIMETRIC:
          action = "image-softproof-intent-absolute-colorimetric";
          break;
        }

      ligma_action_group_set_action_active (group, action, TRUE);

      s_bpc  = ligma_image_get_simulation_bpc (image);
    }
  else
    {
      SET_VISIBLE ("image-convert-perceptual", FALSE);
    }

  SET_SENSITIVE ("image-duplicate", image);

  if (profile)
    {
      SET_LABEL ("image-convert-rgb",
                 C_("image-convert-action", "_RGB..."));
      SET_LABEL ("image-convert-grayscale",
                 C_("image-convert-action", "_Grayscale..."));
    }
  else
    {
      SET_LABEL ("image-convert-rgb",
                 C_("image-convert-action", "_RGB"));
      SET_LABEL ("image-convert-grayscale",
                 C_("image-convert-action", "_Grayscale"));
    }

  SET_SENSITIVE ("image-convert-rgb",       image);
  SET_SENSITIVE ("image-convert-grayscale", image);
  SET_SENSITIVE ("image-convert-indexed",   image && !groups && is_u8_gamma);

  SET_SENSITIVE ("image-softproof-profile",                      image);
  SET_SENSITIVE ("image-softproof-intent-perceptual",            image);
  SET_SENSITIVE ("image-softproof-intent-relative-colorimetric", image);
  SET_SENSITIVE ("image-softproof-intent-saturation",            image);
  SET_SENSITIVE ("image-softproof-intent-absolute-colorimetric", image);
  SET_SENSITIVE ("image-softproof-black-point-compensation",     image);
  SET_ACTIVE    ("image-softproof-black-point-compensation",     s_bpc);

  SET_SENSITIVE ("image-convert-u8",     image);
  SET_SENSITIVE ("image-convert-u16",    image && !is_indexed);
  SET_SENSITIVE ("image-convert-u32",    image && !is_indexed);
  SET_SENSITIVE ("image-convert-half",   image && !is_indexed);
  SET_SENSITIVE ("image-convert-float",  image && !is_indexed);
  SET_SENSITIVE ("image-convert-double", image && !is_indexed);
  SET_VISIBLE   ("image-convert-double", is_double);

  SET_SENSITIVE ("image-convert-linear",     image && !is_indexed);
  SET_SENSITIVE ("image-convert-non-linear", image);
  SET_SENSITIVE ("image-convert-perceptual", image && !is_indexed);

  SET_SENSITIVE ("image-color-profile-use-srgb", image && (profile || profile_hidden));
  SET_ACTIVE    ("image-color-profile-use-srgb", image && profile_srgb);

  SET_SENSITIVE ("image-color-profile-assign",  image);
  SET_SENSITIVE ("image-color-profile-convert", image);
  SET_SENSITIVE ("image-color-profile-discard", image && profile);
  SET_SENSITIVE ("image-color-profile-save",    image);

  SET_SENSITIVE ("image-flip-horizontal", image);
  SET_SENSITIVE ("image-flip-vertical",   image);
  SET_SENSITIVE ("image-rotate-90",       image);
  SET_SENSITIVE ("image-rotate-180",      image);
  SET_SENSITIVE ("image-rotate-270",      image);

  SET_SENSITIVE ("image-resize",              image);
  SET_SENSITIVE ("image-resize-to-layers",    image);
  SET_SENSITIVE ("image-resize-to-selection", image && sel);
  SET_SENSITIVE ("image-print-size",          image);
  SET_SENSITIVE ("image-scale",               image);
  SET_SENSITIVE ("image-crop-to-selection",   image && sel);
  SET_SENSITIVE ("image-crop-to-content",     image);
  SET_SENSITIVE ("image-merge-layers",        image && !aux && lp);
  SET_SENSITIVE ("image-flatten",             image && !aux && lp);
  SET_SENSITIVE ("image-configure-grid",      image);
  SET_SENSITIVE ("image-properties",          image);

#undef SET_LABEL
#undef SET_SENSITIVE
#undef SET_ACTIVE
#undef SET_VISIBLE
}
