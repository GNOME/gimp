/* The GIMP -- an image manipulation program
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

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "image-actions.h"
#include "image-commands.h"

#include "gimp-intl.h"


static GimpActionEntry image_actions[] =
{
  { "toolbox-menubar",      NULL, N_("Toolbox Menu") },
  { "extensions-menu",      NULL, N_("_Xtns")        },
  { "image-menubar",        NULL, N_("Image Menu")   },
  { "image-menu",           NULL, N_("_Image")       },
  { "image-mode-menu",      NULL, N_("_Mode")        },
  { "image-transform-menu", NULL, N_("_Transform")   },

  { "image-convert-rgb", GIMP_STOCK_CONVERT_RGB,
    N_("_RGB"), NULL, NULL,
    G_CALLBACK (image_convert_rgb_cmd_callback),
    GIMP_HELP_IMAGE_CONVERT_RGB },

  { "image-convert-grayscale", GIMP_STOCK_CONVERT_GRAYSCALE,
    N_("_Grayscale"), NULL, NULL,
    G_CALLBACK (image_convert_grayscale_cmd_callback),
    GIMP_HELP_IMAGE_CONVERT_GRAYSCALE },

  { "image-convert-indexed", GIMP_STOCK_CONVERT_INDEXED,
    N_("_Indexed..."), NULL, NULL,
    G_CALLBACK (image_convert_indexed_cmd_callback),
    GIMP_HELP_IMAGE_CONVERT_INDEXED },

  { "image-resize", GIMP_STOCK_RESIZE,
    N_("Can_vas Size..."), NULL, NULL,
    G_CALLBACK (image_resize_cmd_callback),
    GIMP_HELP_IMAGE_RESIZE },

  { "image-scale", GIMP_STOCK_SCALE,
    N_("_Scale Image..."), NULL, NULL,
    G_CALLBACK (image_scale_cmd_callback),
    GIMP_HELP_IMAGE_SCALE },

  { "image-crop", GIMP_STOCK_TOOL_CROP,
    N_("_Crop Image"), NULL, NULL,
    G_CALLBACK (image_crop_cmd_callback),
    GIMP_HELP_IMAGE_CROP },

  { "image-duplicate", GIMP_STOCK_DUPLICATE,
    N_("_Duplicate"), "<control>D", NULL,
    G_CALLBACK (image_duplicate_cmd_callback),
    GIMP_HELP_IMAGE_DUPLICATE },

  { "image-merge-layers", NULL,
    N_("Merge Visible _Layers..."), "<control>M", NULL,
    G_CALLBACK (image_merge_layers_cmd_callback),
    GIMP_HELP_IMAGE_MERGE_LAYERS },

  { "image-flatten", NULL,
    N_("_Flatten Image"), NULL, NULL,
    G_CALLBACK (image_flatten_image_cmd_callback),
    GIMP_HELP_IMAGE_FLATTEN },

  { "image-configure-grid", GIMP_STOCK_GRID,
    N_("Configure G_rid..."), NULL, NULL,
    G_CALLBACK (image_configure_grid_cmd_callback),
    GIMP_HELP_IMAGE_GRID }
};

static GimpEnumActionEntry image_flip_actions[] =
{
  { "image-flip-horizontal", GIMP_STOCK_FLIP_HORIZONTAL,
    N_("Flip _Horizontally"), NULL, NULL,
    GIMP_ORIENTATION_HORIZONTAL,
    GIMP_HELP_IMAGE_FLIP_HORIZONTAL },

  { "image-flip-vertical", GIMP_STOCK_FLIP_VERTICAL,
    N_("Flip _Vertically"), NULL, NULL,
    GIMP_ORIENTATION_VERTICAL,
    GIMP_HELP_IMAGE_FLIP_VERTICAL }
};

static GimpEnumActionEntry image_rotate_actions[] =
{
  { "image-rotate-90", GIMP_STOCK_ROTATE_90,
    /*  please use the degree symbol in the translation  */
    N_("Rotate 90 degrees _CW"), NULL, NULL,
    GIMP_ROTATE_90,
    GIMP_HELP_IMAGE_ROTATE_90 },

  { "image-rotate-180", GIMP_STOCK_ROTATE_180,
    N_("Rotate _180 degrees"), NULL, NULL,
    GIMP_ROTATE_180,
    GIMP_HELP_IMAGE_ROTATE_180 },

  { "image-rotate-270", GIMP_STOCK_ROTATE_270,
    N_("Rotate 90 degrees CC_W"), NULL, NULL,
    GIMP_ROTATE_270,
    GIMP_HELP_IMAGE_ROTATE_270 }
};


void
image_actions_setup (GimpActionGroup *group,
                     gpointer         data)
{
  gimp_action_group_add_actions (group,
                                 image_actions,
                                 G_N_ELEMENTS (image_actions),
                                 data);

  gimp_action_group_add_enum_actions (group,
                                      image_flip_actions,
                                      G_N_ELEMENTS (image_flip_actions),
                                      G_CALLBACK (image_flip_cmd_callback),
                                      data);

  gimp_action_group_add_enum_actions (group,
                                      image_rotate_actions,
                                      G_N_ELEMENTS (image_rotate_actions),
                                      G_CALLBACK (image_rotate_cmd_callback),
                                      data);
}

void
image_actions_update (GimpActionGroup *group,
                      gpointer         data)
{
  GimpDisplay      *gdisp      = NULL;
  GimpDisplayShell *shell      = NULL;
  GimpImage        *gimage     = NULL;
  gboolean          is_rgb     = FALSE;
  gboolean          is_gray    = FALSE;
  gboolean          is_indexed = FALSE;
  gboolean          fs         = FALSE;
  gboolean          aux        = FALSE;
  gboolean          lp         = FALSE;
  gboolean          sel        = FALSE;

  if (GIMP_IS_DISPLAY_SHELL (data))
    {
      shell = GIMP_DISPLAY_SHELL (data);
      gdisp = shell->gdisp;
    }
  else if (GIMP_IS_DISPLAY (data))
    {
      gdisp = GIMP_DISPLAY (data);
      shell = GIMP_DISPLAY_SHELL (gdisp->shell);
    }

  if (gdisp)
    {
      GimpImageBaseType base_type;

      gimage = gdisp->gimage;

      base_type = gimp_image_base_type (gimage);

      is_rgb     = (base_type == GIMP_RGB);
      is_gray    = (base_type == GIMP_GRAY);
      is_indexed = (base_type == GIMP_INDEXED);

      fs  = (gimp_image_floating_sel (gimage) != NULL);
      aux = (gimp_image_get_active_channel (gimage) != NULL);
      lp  = ! gimp_image_is_empty (gimage);
      sel = ! gimp_channel_is_empty (gimp_image_get_mask (gimage));
    }

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)

  SET_SENSITIVE ("image-convert-rgb",       gdisp && ! is_rgb);
  SET_SENSITIVE ("image-convert-grayscale", gdisp && ! is_gray);
  SET_SENSITIVE ("image-convert-indexed",   gdisp && ! is_indexed);

  SET_SENSITIVE ("image-flip-horizontal", gdisp);
  SET_SENSITIVE ("image-flip-vertical",   gdisp);
  SET_SENSITIVE ("image-rotate-90",       gdisp);
  SET_SENSITIVE ("image-rotate-180",      gdisp);
  SET_SENSITIVE ("image-rotate-270",      gdisp);

  SET_SENSITIVE ("image-resize",         gdisp);
  SET_SENSITIVE ("image-scale",          gdisp);
  SET_SENSITIVE ("image-crop",           gdisp && sel);
  SET_SENSITIVE ("image-duplicate",      gdisp);
  SET_SENSITIVE ("image-merge-layers",   gdisp && !fs && !aux && lp);
  SET_SENSITIVE ("image-flatten",        gdisp && !fs && !aux && lp);
  SET_SENSITIVE ("image-configure-grid", gdisp);

#undef SET_SENSITIVE
}
