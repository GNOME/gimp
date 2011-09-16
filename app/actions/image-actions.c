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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpitemstack.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "actions.h"
#include "image-actions.h"
#include "image-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry image_actions[] =
{
  { "image-menubar", NULL,
    NC_("image-action", "Image Menu"), NULL, NULL, NULL,
    GIMP_HELP_IMAGE_WINDOW },

  { "image-popup", NULL,
    NC_("image-action", "Image Menu"), NULL, NULL, NULL,
    GIMP_HELP_IMAGE_WINDOW },

  { "image-menu",             NULL, NC_("image-action", "_Image")      },
  { "image-mode-menu",        NULL, NC_("image-action", "_Mode")       },
  { "image-transform-menu",   NULL, NC_("image-action", "_Transform")  },
  { "image-guides-menu",      NULL, NC_("image-action", "_Guides")     },

  { "colors-menu",            NULL, NC_("image-action", "_Colors")     },
  { "colors-info-menu",       NULL, NC_("image-action", "I_nfo")       },
  { "colors-auto-menu",       NULL, NC_("image-action", "_Auto")       },
  { "colors-map-menu",        NULL, NC_("image-action", "_Map")        },
  { "colors-components-menu", NULL, NC_("image-action", "C_omponents") },

  { "image-new", GTK_STOCK_NEW,
    NC_("image-action", "_New..."), "<primary>N",
    NC_("image-action", "Create a new image"),
    G_CALLBACK (image_new_cmd_callback),
    GIMP_HELP_FILE_NEW },

  { "image-resize", GIMP_STOCK_RESIZE,
    NC_("image-action", "Can_vas Size..."), NULL,
    NC_("image-action", "Adjust the image dimensions"),
    G_CALLBACK (image_resize_cmd_callback),
    GIMP_HELP_IMAGE_RESIZE },

  { "image-resize-to-layers", NULL,
    NC_("image-action", "Fit Canvas to L_ayers"), NULL,
    NC_("image-action", "Resize the image to enclose all layers"),
    G_CALLBACK (image_resize_to_layers_cmd_callback),
    GIMP_HELP_IMAGE_RESIZE_TO_LAYERS },

  { "image-resize-to-selection", NULL,
    NC_("image-action", "F_it Canvas to Selection"), NULL,
    NC_("image-action", "Resize the image to the extents of the selection"),
    G_CALLBACK (image_resize_to_selection_cmd_callback),
    GIMP_HELP_IMAGE_RESIZE_TO_SELECTION },

  { "image-print-size", GIMP_STOCK_PRINT_RESOLUTION,
    NC_("image-action", "_Print Size..."), NULL,
    NC_("image-action", "Adjust the print resolution"),
    G_CALLBACK (image_print_size_cmd_callback),
    GIMP_HELP_IMAGE_PRINT_SIZE },

  { "image-scale", GIMP_STOCK_SCALE,
    NC_("image-action", "_Scale Image..."), NULL,
    NC_("image-action", "Change the size of the image content"),
    G_CALLBACK (image_scale_cmd_callback),
    GIMP_HELP_IMAGE_SCALE },

  { "image-crop", GIMP_STOCK_TOOL_CROP,
    NC_("image-action", "_Crop to Selection"), NULL,
    NC_("image-action", "Crop the image to the extents of the selection"),
    G_CALLBACK (image_crop_cmd_callback),
    GIMP_HELP_IMAGE_CROP },

  { "image-duplicate", GIMP_STOCK_DUPLICATE,
    NC_("image-action", "_Duplicate"), "<primary>D",
    NC_("image-action", "Create a duplicate of this image"),
    G_CALLBACK (image_duplicate_cmd_callback),
    GIMP_HELP_IMAGE_DUPLICATE },

  { "image-merge-layers", NULL,
    NC_("image-action", "Merge Visible _Layers..."), "<primary>M",
    NC_("image-action", "Merge all visible layers into one layer"),
    G_CALLBACK (image_merge_layers_cmd_callback),
    GIMP_HELP_IMAGE_MERGE_LAYERS },

  { "image-flatten", NULL,
    NC_("image-action", "_Flatten Image"), NULL,
    NC_("image-action", "Merge all layers into one and remove transparency"),
    G_CALLBACK (image_flatten_image_cmd_callback),
    GIMP_HELP_IMAGE_FLATTEN },

  { "image-configure-grid", GIMP_STOCK_GRID,
    NC_("image-action", "Configure G_rid..."), NULL,
    NC_("image-action", "Configure the grid for this image"),
    G_CALLBACK (image_configure_grid_cmd_callback),
    GIMP_HELP_IMAGE_GRID },

  { "image-properties", GTK_STOCK_INFO,
    NC_("image-action", "Image Pr_operties"), "<alt>Return",
    NC_("image-action", "Display information about this image"),
    G_CALLBACK (image_properties_cmd_callback),
    GIMP_HELP_IMAGE_PROPERTIES }
};

static const GimpRadioActionEntry image_convert_actions[] =
{
  { "image-convert-rgb", GIMP_STOCK_CONVERT_RGB,
    NC_("image-convert-action", "_RGB"), NULL,
    NC_("image-convert-action", "Convert the image to the RGB colorspace"),
    GIMP_RGB, GIMP_HELP_IMAGE_CONVERT_RGB },

  { "image-convert-grayscale", GIMP_STOCK_CONVERT_GRAYSCALE,
    NC_("image-convert-action", "_Grayscale"), NULL,
    NC_("image-convert-action", "Convert the image to grayscale"),
    GIMP_GRAY, GIMP_HELP_IMAGE_CONVERT_GRAYSCALE },

  { "image-convert-indexed", GIMP_STOCK_CONVERT_INDEXED,
    NC_("image-convert-action", "_Indexed..."), NULL,
    NC_("image-convert-action", "Convert the image to indexed colors"),
    GIMP_INDEXED, GIMP_HELP_IMAGE_CONVERT_INDEXED }
};

static const GimpEnumActionEntry image_flip_actions[] =
{
  { "image-flip-horizontal", GIMP_STOCK_FLIP_HORIZONTAL,
    NC_("image-action", "Flip _Horizontally"), NULL,
    NC_("image-action", "Flip image horizontally"),
    GIMP_ORIENTATION_HORIZONTAL, FALSE,
    GIMP_HELP_IMAGE_FLIP_HORIZONTAL },

  { "image-flip-vertical", GIMP_STOCK_FLIP_VERTICAL,
    NC_("image-action", "Flip _Vertically"), NULL,
    NC_("image-action", "Flip image vertically"),
    GIMP_ORIENTATION_VERTICAL, FALSE,
    GIMP_HELP_IMAGE_FLIP_VERTICAL }
};

static const GimpEnumActionEntry image_rotate_actions[] =
{
  { "image-rotate-90", GIMP_STOCK_ROTATE_90,
    NC_("image-action", "Rotate 90° _clockwise"), NULL,
    NC_("image-action", "Rotate the image 90 degrees to the right"),
    GIMP_ROTATE_90, FALSE,
    GIMP_HELP_IMAGE_ROTATE_90 },

  { "image-rotate-180", GIMP_STOCK_ROTATE_180,
    NC_("image-action", "Rotate _180°"), NULL,
    NC_("image-action", "Turn the image upside-down"),
    GIMP_ROTATE_180, FALSE,
    GIMP_HELP_IMAGE_ROTATE_180 },

  { "image-rotate-270", GIMP_STOCK_ROTATE_270,
    NC_("image-action", "Rotate 90° counter-clock_wise"), NULL,
    NC_("image-action", "Rotate the image 90 degrees to the left"),
    GIMP_ROTATE_270, FALSE,
    GIMP_HELP_IMAGE_ROTATE_270 }
};


void
image_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group, "image-action",
                                 image_actions,
                                 G_N_ELEMENTS (image_actions));

  gimp_action_group_add_radio_actions (group, "image-convert-action",
                                       image_convert_actions,
                                       G_N_ELEMENTS (image_convert_actions),
                                       NULL, 0,
                                       G_CALLBACK (image_convert_cmd_callback));

  gimp_action_group_add_enum_actions (group, "image-action",
                                      image_flip_actions,
                                      G_N_ELEMENTS (image_flip_actions),
                                      G_CALLBACK (image_flip_cmd_callback));

  gimp_action_group_add_enum_actions (group, "image-action",
                                      image_rotate_actions,
                                      G_N_ELEMENTS (image_rotate_actions),
                                      G_CALLBACK (image_rotate_cmd_callback));

#define SET_ALWAYS_SHOW_IMAGE(action,show) \
        gimp_action_group_set_action_always_show_image (group, action, show)

  SET_ALWAYS_SHOW_IMAGE ("image-rotate-90",  TRUE);
  SET_ALWAYS_SHOW_IMAGE ("image-rotate-180", TRUE);
  SET_ALWAYS_SHOW_IMAGE ("image-rotate-270", TRUE);

#undef SET_ALWAYS_SHOW_IMAGE
}

void
image_actions_update (GimpActionGroup *group,
                      gpointer         data)
{
  GimpImage *image  = action_data_get_image (data);
  gboolean   aux    = FALSE;
  gboolean   lp     = FALSE;
  gboolean   sel    = FALSE;
  gboolean   groups = FALSE;

  if (image)
    {
      GimpContainer *layers;
      const gchar   *action = NULL;

      switch (gimp_image_base_type (image))
        {
        case GIMP_RGB:
          action = "image-convert-rgb";
          break;

        case GIMP_GRAY:
          action = "image-convert-grayscale";
          break;

        case GIMP_INDEXED:
          action = "image-convert-indexed";
          break;
        }

      gimp_action_group_set_action_active (group, action, TRUE);

      aux = (gimp_image_get_active_channel (image) != NULL);
      lp  = ! gimp_image_is_empty (image);
      sel = ! gimp_channel_is_empty (gimp_image_get_mask (image));

      layers = gimp_image_get_layers (image);

      groups = ! gimp_item_stack_is_flat (GIMP_ITEM_STACK (layers));
    }

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)

  SET_SENSITIVE ("image-convert-rgb",       image);
  SET_SENSITIVE ("image-convert-grayscale", image);
  SET_SENSITIVE ("image-convert-indexed",   image && !groups);

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
  SET_SENSITIVE ("image-crop",                image && sel);
  SET_SENSITIVE ("image-duplicate",           image);
  SET_SENSITIVE ("image-merge-layers",        image && !aux && lp);
  SET_SENSITIVE ("image-flatten",             image && !aux && lp);
  SET_SENSITIVE ("image-configure-grid",      image);
  SET_SENSITIVE ("image-properties",          image);

#undef SET_SENSITIVE
}
