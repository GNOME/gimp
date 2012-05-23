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

#include "core/gimpimage.h"
#include "core/gimplayermask.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "actions.h"
#include "filters-actions.h"
#include "filters-commands.h"

#include "gimp-intl.h"


static const GimpStringActionEntry filters_actions[] =
{
  { "filters-color-reduction", GIMP_STOCK_GEGL,
    NC_("filters-action", "Color _Reduction..."), NULL,
    NC_("filters-action", "Reduce the number of colors in the image, with optional dithering"),
    "gegl:color-reduction",
    NULL /* FIXME GIMP_HELP_FILTER_COLOR_TEMPERATURE */ },

  { "filters-color-temperature", GIMP_STOCK_GEGL,
    NC_("filters-action", "Color T_emperature..."), NULL,
    NC_("filters-action", "Change the color temperature of the image"),
    "gegl:color-temperature",
    NULL /* FIXME GIMP_HELP_FILTER_COLOR_TEMPERATURE */ },

  { "filters-color-to-alpha", GIMP_STOCK_GEGL,
    NC_("filters-action", "Color to _Alpha..."), NULL,
    NC_("filters-action", "Convert a specified color to transparency"),
    "gegl:color-to-alpha",
    NULL /* FIXME GIMP_HELP_FILTER_COLOR_TO_ALPHA */ },

  { "filters-difference-of-gaussians", GIMP_STOCK_GEGL,
    NC_("filters-action", "Difference of Gaussians..."), NULL,
    NC_("filters-action", "Edge detection with control of edge thickness"),
    "gegl:difference-of-gaussians",
    NULL /* FIXME GIMP_HELP_FILTER_DIFFERENCE_OF_GAUSSIANS */ },

  { "filters-gaussian-blur", GIMP_STOCK_GEGL,
    NC_("filters-action", "_Gaussian Blur..."), NULL,
    NC_("filters-action", "Apply a gaussian blur"),
    "gegl:gaussian-blur",
    NULL /* FIXME GIMP_HELP_FILTER_GAUSSIAN_BLUR */ },

  { "filters-laplace", GIMP_STOCK_GEGL,
    NC_("filters-action", "_Laplace"), NULL,
    NC_("filters-action", "High-resolution edge detection"),
    "gegl:edge-laplace",
    NULL /* FIXME GIMP_HELP_FILTER_LAPLACE */ },

  { "filters-lens-distortion", GIMP_STOCK_GEGL,
    NC_("filters-action", "Lens Distortion..."), NULL,
    NC_("filters-action", "Corrects lens distortion"),
    "gegl:lens-distortion",
    NULL /* FIXME GIMP_HELP_FILTER_LENS_DISTORTION */ },

  { "filters-pixelize", GIMP_STOCK_GEGL,
    NC_("filters-action", "_Pixelize..."), NULL,
    NC_("filters-action", "Simplify image into an array of solid-colored squares"),
    "gegl:pixelize",
    NULL /* FIXME GIMP_HELP_FILTER_PIXELIZE */ },

  { "filters-polar-coordinates", GIMP_STOCK_GEGL,
    NC_("filters-action", "P_olar Coordinates..."), NULL,
    NC_("filters-action", "Convert image to or from polar coordinates"),
    "gegl:polar-coordinates",
    NULL /* FIXME GIMP_HELP_FILTER_POLAR_COORDINATES */ },

  { "filters-ripple", GIMP_STOCK_GEGL,
    NC_("filters-action", "_Ripple..."), NULL,
    NC_("filters-action", "Displace pixels in a ripple pattern"),
    "gegl:ripple",
    NULL /* FIXME GIMP_HELP_FILTER_RIPPLE */ },

  { "filters-semi-flatten", GIMP_STOCK_GEGL,
    NC_("filters-action", "_Semi-Flatten..."), NULL,
    NC_("filters-action", "Replace partial transparency with a color"),
    "gimp:semi-flatten",
    NULL /* FIXME GIMP_HELP_FILTER_POLAR_COORDINATES */ },

  { "filters-sobel", GIMP_STOCK_GEGL,
    NC_("filters-action", "_Sobel..."), NULL,
    NC_("filters-action", "Specialized direction-dependent edge-detection"),
    "gegl:edge-sobel",
    NULL /* FIXME GIMP_HELP_FILTER_SOBEL */ },

  { "filters-threshold-alpha", GIMP_STOCK_GEGL,
    NC_("filters-action", "_Threshold Alpha..."), NULL,
    NC_("filters-action", "Make transparency all-or-nothing"),
    "gimp:threshold-alpha",
    NULL /* FIXME GIMP_HELP_FILTER_POLAR_COORDINATES */ },

  { "filters-unsharp-mask", GIMP_STOCK_GEGL,
    NC_("filters-action", "_Unsharp Mask..."), NULL,
    NC_("filters-action", "The most widely used method for sharpening an image"),
    "gegl:unsharp-mask",
    NULL /* FIXME GIMP_HELP_FILTER_UNSHARP_MASK */ },
};

void
filters_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_string_actions (group, "filters-action",
                                        filters_actions,
                                        G_N_ELEMENTS (filters_actions),
                                        G_CALLBACK (filters_filter_cmd_callback));
}

void
filters_actions_update (GimpActionGroup *group,
                        gpointer         data)
{
  GimpImage    *image;
  GimpDrawable *drawable = NULL;
  gboolean      writable = FALSE;
  gboolean      gray     = FALSE;
  gboolean      alpha    = FALSE;

  image = action_data_get_image (data);

  if (image)
    {
      drawable = gimp_image_get_active_drawable (image);

      if (drawable)
        {
          GimpItem *item;

          alpha = gimp_drawable_has_alpha (drawable);
          gray  = gimp_drawable_is_gray (drawable);

          if (GIMP_IS_LAYER_MASK (drawable))
            item = GIMP_ITEM (gimp_layer_mask_get_layer (GIMP_LAYER_MASK (drawable)));
          else
            item = GIMP_ITEM (drawable);

          writable = ! gimp_item_is_content_locked (item);

          if (gimp_viewable_get_children (GIMP_VIEWABLE (drawable)))
            writable = FALSE;
        }
    }

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)

  SET_SENSITIVE ("filters-color-reduction",         writable);
  SET_SENSITIVE ("filters-color-temperature",       writable && !gray);
  SET_SENSITIVE ("filters-color-to-alpha",          writable && !gray && alpha);
  SET_SENSITIVE ("filters-difference-of-gaussians", writable);
  SET_SENSITIVE ("filters-gaussian-blur",           writable);
  SET_SENSITIVE ("filters-laplace",                 writable);
  SET_SENSITIVE ("filters-lens-distortion",         writable);
  SET_SENSITIVE ("filters-pixelize",                writable);
  SET_SENSITIVE ("filters-polar-coordinates",       writable);
  SET_SENSITIVE ("filters-ripple",                  writable);
  SET_SENSITIVE ("filters-sobel",                   writable);
  SET_SENSITIVE ("filters-semi-flatten",            writable && alpha);
  SET_SENSITIVE ("filters-threshold-alpha",         writable && alpha);
  SET_SENSITIVE ("filters-unsharp-mask",            writable);

#undef SET_SENSITIVE
}
