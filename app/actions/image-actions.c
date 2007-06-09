/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "actions.h"
#include "image-actions.h"
#include "image-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry image_actions[] =
{
  { "toolbox-menubar", NULL,
    N_("Toolbox Menu"), NULL, NULL, NULL,
    GIMP_HELP_TOOLBOX },

  { "image-menubar", NULL,
    N_("Image Menu"), NULL, NULL, NULL,
    GIMP_HELP_IMAGE_WINDOW },

  { "image-popup", NULL,
    N_("Image Menu"), NULL, NULL, NULL,
    GIMP_HELP_IMAGE_WINDOW },

  { "extensions-menu",        NULL, N_("_Xtns")       },

  { "image-menu",             NULL, N_("_Image")      },
  { "image-mode-menu",        NULL, N_("_Mode")       },
  { "image-transform-menu",   NULL, N_("_Transform")  },
  { "image-guides-menu",      NULL, N_("_Guides")     },

  { "colors-menu",            NULL, N_("_Colors")     },
  { "colors-info-menu",       NULL, N_("I_nfo")       },
  { "colors-auto-menu",       NULL, N_("_Auto")       },
  { "colors-map-menu",        NULL, N_("_Map")        },
  { "colors-components-menu", NULL, N_("C_omponents") },

  { "image-new", GTK_STOCK_NEW,
    N_("_New..."), "<control>N",
    N_("Create a new image"),
    G_CALLBACK (image_new_cmd_callback),
    GIMP_HELP_FILE_NEW },

  { "image-new-from-image", GTK_STOCK_NEW,
    N_("_New..."), NULL,
    N_("Create a new image"),
    G_CALLBACK (image_new_from_image_cmd_callback),
    GIMP_HELP_FILE_NEW },

  { "image-resize", GIMP_STOCK_RESIZE,
    N_("Can_vas Size..."), NULL,
    N_("Adjust the image dimensions"),
    G_CALLBACK (image_resize_cmd_callback),
    GIMP_HELP_IMAGE_RESIZE },

  { "image-resize-to-layers", NULL,
    N_("F_it Canvas to Layers"), NULL,
    N_("Resize the image to enclose all layers"),
    G_CALLBACK (image_resize_to_layers_cmd_callback),
    GIMP_HELP_IMAGE_RESIZE_TO_LAYERS },

  { "image-resize-to-selection", NULL,
    N_("F_it Canvas to Selection"), NULL,
    N_("Resize the image to the extents of the selection"),
    G_CALLBACK (image_resize_to_selection_cmd_callback),
    GIMP_HELP_IMAGE_RESIZE_TO_SELECTION },

  { "image-print-size", GIMP_STOCK_PRINT_RESOLUTION,
    N_("_Print Size..."), NULL,
    N_("Adjust the print resolution"),
    G_CALLBACK (image_print_size_cmd_callback),
    GIMP_HELP_IMAGE_PRINT_SIZE },

  { "image-scale", GIMP_STOCK_SCALE,
    N_("_Scale Image..."), NULL,
    N_("Change the size of the image content"),
    G_CALLBACK (image_scale_cmd_callback),
    GIMP_HELP_IMAGE_SCALE },

  { "image-crop", GIMP_STOCK_TOOL_CROP,
    N_("_Crop to Selection"), NULL,
    N_("Crop the image to the extents of the selection"),
    G_CALLBACK (image_crop_cmd_callback),
    GIMP_HELP_IMAGE_CROP },

  { "image-duplicate", GIMP_STOCK_DUPLICATE,
    N_("_Duplicate"), "<control>D",
    N_("Create a duplicate of this image"),
    G_CALLBACK (image_duplicate_cmd_callback),
    GIMP_HELP_IMAGE_DUPLICATE },

  { "image-merge-layers", NULL,
    N_("Merge Visible _Layers..."), "<control>M",
    N_("Merge all visible layers into one layer"),
    G_CALLBACK (image_merge_layers_cmd_callback),
    GIMP_HELP_IMAGE_MERGE_LAYERS },

  { "image-flatten", NULL,
    N_("_Flatten Image"), NULL,
    N_("Merge all layers into one and remove transparency"),
    G_CALLBACK (image_flatten_image_cmd_callback),
    GIMP_HELP_IMAGE_FLATTEN },

  { "image-configure-grid", GIMP_STOCK_GRID,
    N_("Configure G_rid..."), NULL,
    N_("Configure the grid for this image"),
    G_CALLBACK (image_configure_grid_cmd_callback),
    GIMP_HELP_IMAGE_GRID },

  { "image-properties", GTK_STOCK_INFO,
    N_("Image Pr_operties"), NULL,
    N_("Display information about this image"),
    G_CALLBACK (image_properties_cmd_callback),
    GIMP_HELP_IMAGE_PROPERTIES }
};

static const GimpRadioActionEntry image_convert_actions[] =
{
  { "image-convert-rgb", GIMP_STOCK_CONVERT_RGB,
    N_("_RGB"), NULL,
    N_("Convert the image to the RGB colorspace"),
    GIMP_RGB, GIMP_HELP_IMAGE_CONVERT_RGB },

  { "image-convert-grayscale", GIMP_STOCK_CONVERT_GRAYSCALE,
    N_("_Grayscale"), NULL,
    N_("Convert the image to grayscale"),
    GIMP_GRAY, GIMP_HELP_IMAGE_CONVERT_GRAYSCALE },

  { "image-convert-indexed", GIMP_STOCK_CONVERT_INDEXED,
    N_("_Indexed..."), NULL,
    N_("Convert the image to indexed colors"),
    GIMP_INDEXED, GIMP_HELP_IMAGE_CONVERT_INDEXED }
};

static const GimpEnumActionEntry image_flip_actions[] =
{
  { "image-flip-horizontal", GIMP_STOCK_FLIP_HORIZONTAL,
    N_("Flip _Horizontally"), NULL,
    N_("Flip image horizontally"),
    GIMP_ORIENTATION_HORIZONTAL, FALSE,
    GIMP_HELP_IMAGE_FLIP_HORIZONTAL },

  { "image-flip-vertical", GIMP_STOCK_FLIP_VERTICAL,
    N_("Flip _Vertically"), NULL,
    N_("Flip image vertically"),
    GIMP_ORIENTATION_VERTICAL, FALSE,
    GIMP_HELP_IMAGE_FLIP_VERTICAL }
};

static const GimpEnumActionEntry image_rotate_actions[] =
{
  { "image-rotate-90", GIMP_STOCK_ROTATE_90,
    N_("Rotate 90° _clockwise"), NULL,
    N_("Rotate the image 90 degrees to the right"),
    GIMP_ROTATE_90, FALSE,
    GIMP_HELP_IMAGE_ROTATE_90 },

  { "image-rotate-180", GIMP_STOCK_ROTATE_180,
    N_("Rotate _180°"), NULL,
    N_("Turn the image upside-down"),
    GIMP_ROTATE_180, FALSE,
    GIMP_HELP_IMAGE_ROTATE_180 },

  { "image-rotate-270", GIMP_STOCK_ROTATE_270,
    N_("Rotate 90° counter-clock_wise"), NULL,
    N_("Rotate the image 90 degrees to the left"),
    GIMP_ROTATE_270, FALSE,
    GIMP_HELP_IMAGE_ROTATE_270 }
};


void
image_actions_setup (GimpActionGroup *group)
{
  GtkAction *action;

  gimp_action_group_add_actions (group,
                                 image_actions,
                                 G_N_ELEMENTS (image_actions));

  action = gtk_action_group_get_action (GTK_ACTION_GROUP (group),
                                        "image-new-from-image");
  gtk_action_set_accel_path (action, "<Actions>/image/image-new");

  gimp_action_group_add_radio_actions (group,
                                       image_convert_actions,
                                       G_N_ELEMENTS (image_convert_actions),
                                       NULL, 0,
                                       G_CALLBACK (image_convert_cmd_callback));

  gimp_action_group_add_enum_actions (group,
                                      image_flip_actions,
                                      G_N_ELEMENTS (image_flip_actions),
                                      G_CALLBACK (image_flip_cmd_callback));

  gimp_action_group_add_enum_actions (group,
                                      image_rotate_actions,
                                      G_N_ELEMENTS (image_rotate_actions),
                                      G_CALLBACK (image_rotate_cmd_callback));
}

void
image_actions_update (GimpActionGroup *group,
                      gpointer         data)
{
  GimpImage *image = action_data_get_image (data);
  gboolean   fs    = FALSE;
  gboolean   aux   = FALSE;
  gboolean   lp    = FALSE;
  gboolean   sel   = FALSE;

  if (image)
    {
      const gchar *action = NULL;

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

      fs  = (gimp_image_floating_sel (image) != NULL);
      aux = (gimp_image_get_active_channel (image) != NULL);
      lp  = ! gimp_image_is_empty (image);
      sel = ! gimp_channel_is_empty (gimp_image_get_mask (image));
    }

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)

  SET_SENSITIVE ("image-convert-rgb",       image);
  SET_SENSITIVE ("image-convert-grayscale", image);
  SET_SENSITIVE ("image-convert-indexed",   image);

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
  SET_SENSITIVE ("image-merge-layers",        image && !fs && !aux && lp);
  SET_SENSITIVE ("image-flatten",             image && !fs && !aux && lp);
  SET_SENSITIVE ("image-configure-grid",      image);
  SET_SENSITIVE ("image-properties",          image);

#undef SET_SENSITIVE
}
