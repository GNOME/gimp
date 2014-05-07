/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpstock.c
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "gimpstock.h"

#include "icons/gimp-stock-pixbufs.h"

#include "libgimp/libgimp-intl.h"


/**
 * SECTION: gimpstock
 * @title: GimpStock
 * @short_description: Prebuilt common menu/toolbar items and
 *                     corresponding icons
 *
 * GIMP registers a set of menu/toolbar items and corresponding icons
 * in addition to the standard GTK+ stock items. These can be used
 * just like GTK+ stock items. GIMP also overrides a few of the GTK+
 * icons (namely the ones in dialog size).
 *
 * Stock icons may have a RTL variant which gets used for
 * right-to-left locales.
 **/


#define LIBGIMP_DOMAIN     GETTEXT_PACKAGE "-libgimp"
#define GIMP_TOILET_PAPER  "gimp-toilet-paper"


static GtkIconFactory *gimp_stock_factory = NULL;


static void
icon_set_from_inline (GtkIconSet       *set,
                      const guchar     *inline_data,
                      GtkIconSize       size,
                      GtkTextDirection  direction,
                      gboolean          fallback)
{
  GtkIconSource *source;
  GdkPixbuf     *pixbuf;

  source = gtk_icon_source_new ();

  if (direction != GTK_TEXT_DIR_NONE)
    {
      gtk_icon_source_set_direction (source, direction);
      gtk_icon_source_set_direction_wildcarded (source, FALSE);
    }

  gtk_icon_source_set_size (source, size);
  gtk_icon_source_set_size_wildcarded (source, FALSE);

  pixbuf = gdk_pixbuf_new_from_inline (-1, inline_data, FALSE, NULL);

  g_assert (pixbuf);

  gtk_icon_source_set_pixbuf (source, pixbuf);

  g_object_unref (pixbuf);

  gtk_icon_set_add_source (set, source);

  if (fallback)
    {
      gtk_icon_source_set_size_wildcarded (source, TRUE);
      gtk_icon_set_add_source (set, source);
    }

  gtk_icon_source_free (source);
}

static void
add_sized_with_same_fallback (GtkIconFactory *factory,
                              const guchar   *inline_data,
                              const guchar   *inline_data_rtl,
                              GtkIconSize     size,
                              const gchar    *stock_id)
{
  GtkIconSet *set;
  gboolean    fallback = FALSE;

  set = gtk_icon_factory_lookup (factory, stock_id);

  if (! set)
    {
      set = gtk_icon_set_new ();
      gtk_icon_factory_add (factory, stock_id, set);
      gtk_icon_set_unref (set);

      fallback = TRUE;
    }

  icon_set_from_inline (set, inline_data, size, GTK_TEXT_DIR_NONE, fallback);

  if (inline_data_rtl)
    icon_set_from_inline (set,
                          inline_data_rtl, size, GTK_TEXT_DIR_RTL, fallback);
}


static const GtkStockItem gimp_stock_items[] =
{
  { GIMP_STOCK_ANCHOR,         N_("Anchor"),          0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_CENTER,         N_("C_enter"),         0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_DUPLICATE,      N_("_Duplicate"),      0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_EDIT,           N_("_Edit"),           0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_LINKED,         N_("Linked"),          0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_PASTE_AS_NEW,   N_("Paste as New"),    0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_PASTE_INTO,     N_("Paste Into"),      0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_RESET,          N_("_Reset"),          0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_VISIBLE,        N_("Visible"),         0, 0, LIBGIMP_DOMAIN },

  { GIMP_STOCK_GRADIENT_LINEAR,               NULL,   0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_GRADIENT_BILINEAR,             NULL,   0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_GRADIENT_RADIAL,               NULL,   0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_GRADIENT_SQUARE,               NULL,   0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_GRADIENT_CONICAL_SYMMETRIC,    NULL,   0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_GRADIENT_CONICAL_ASYMMETRIC,   NULL,   0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_GRADIENT_SHAPEBURST_ANGULAR,   NULL,   0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_GRADIENT_SHAPEBURST_SPHERICAL, NULL,   0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_GRADIENT_SHAPEBURST_DIMPLED,   NULL,   0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_GRADIENT_SPIRAL_CLOCKWISE,     NULL,   0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_GRADIENT_SPIRAL_ANTICLOCKWISE, NULL,   0, 0, LIBGIMP_DOMAIN },

  { GIMP_STOCK_GRAVITY_EAST,             NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_GRAVITY_NORTH,            NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_GRAVITY_NORTH_EAST,       NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_GRAVITY_NORTH_WEST,       NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_GRAVITY_SOUTH,            NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_GRAVITY_SOUTH_EAST,       NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_GRAVITY_SOUTH_WEST,       NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_GRAVITY_WEST,             NULL,        0, 0, LIBGIMP_DOMAIN },

  { GIMP_STOCK_HCENTER,                  NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_VCENTER,                  NULL,        0, 0, LIBGIMP_DOMAIN },

  { GIMP_STOCK_HFILL,                    NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_VFILL,                    NULL,        0, 0, LIBGIMP_DOMAIN },

  { GIMP_STOCK_HCHAIN,                   NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_HCHAIN_BROKEN,            NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_VCHAIN,                   NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_VCHAIN_BROKEN,            NULL,        0, 0, LIBGIMP_DOMAIN },

  { GIMP_STOCK_SELECTION,                NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_SELECTION_REPLACE,        NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_SELECTION_ADD,            NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_SELECTION_SUBTRACT,       NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_SELECTION_INTERSECT,      NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_SELECTION_STROKE,       N_("_Stroke"), 0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_SELECTION_TO_CHANNEL,     NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_SELECTION_TO_PATH,        NULL,        0, 0, LIBGIMP_DOMAIN },

  { GIMP_STOCK_PATH_STROKE,            N_("_Stroke"), 0, 0, LIBGIMP_DOMAIN },

  { GIMP_STOCK_CURVE_FREE,               NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_CURVE_SMOOTH,             NULL,        0, 0, LIBGIMP_DOMAIN },

  { GIMP_STOCK_COLOR_PICKER_BLACK,       NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_COLOR_PICKER_GRAY,        NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_COLOR_PICKER_WHITE,       NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_COLOR_TRIANGLE,           NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_COLOR_PICK_FROM_SCREEN,   NULL,        0, 0, LIBGIMP_DOMAIN },

  { GIMP_STOCK_CHAR_PICKER,              NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_LETTER_SPACING, N_("L_etter Spacing"), 0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_LINE_SPACING,   N_("L_ine Spacing"),   0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TEXT_DIR_LTR,             NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TEXT_DIR_RTL,             NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_PRINT_RESOLUTION,         NULL,        0, 0, LIBGIMP_DOMAIN },

  { GIMP_STOCK_CONVERT_RGB,              NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_CONVERT_GRAYSCALE,        NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_CONVERT_INDEXED,          NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_INVERT,                   NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_MERGE_DOWN,               NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_LAYER_TO_IMAGESIZE,       NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_PLUGIN,                   NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_RESHOW_FILTER,            NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_ROTATE_90,                NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_ROTATE_180,               NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_ROTATE_270,               NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_RESIZE,         N_("Re_size"),         0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_SCALE,          N_("_Scale"),          0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_FLIP_HORIZONTAL,          NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_FLIP_VERTICAL,            NULL,        0, 0, LIBGIMP_DOMAIN },

  { GIMP_STOCK_IMAGES,                   NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_LAYERS,                   NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_CHANNELS,                 NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_PATHS,                    NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOLS,                    NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_OPTIONS,             NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_DEVICE_STATUS,            NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_INPUT_DEVICE,             NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_CURSOR,                   NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_SAMPLE_POINT,             NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_DYNAMICS,                 NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_PRESET,              NULL,        0, 0, LIBGIMP_DOMAIN },

  { GIMP_STOCK_IMAGE,                    NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_LAYER,                    NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TEXT_LAYER,               NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_FLOATING_SELECTION,       NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_CHANNEL,                  NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_CHANNEL_RED,              NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_CHANNEL_GREEN,            NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_CHANNEL_BLUE,             NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_CHANNEL_GRAY,             NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_CHANNEL_INDEXED,          NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_CHANNEL_ALPHA,            NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_LAYER_MASK,               NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_PATH,                     NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TEMPLATE,                 NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_COLORMAP,                 NULL,        0, 0, LIBGIMP_DOMAIN },
  { "gimp-indexed-palette", /* compat */ NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_HISTOGRAM,                NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_HISTOGRAM_LINEAR,         NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_HISTOGRAM_LOGARITHMIC,    NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_UNDO_HISTORY,             NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TRANSPARENCY,             NULL,        0, 0, LIBGIMP_DOMAIN },

  { GIMP_STOCK_SELECTION_ALL,            NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_SELECTION_NONE,           NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_SELECTION_GROW,           NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_SELECTION_SHRINK,         NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_SELECTION_BORDER,         NULL,        0, 0, LIBGIMP_DOMAIN },

  { GIMP_STOCK_NAVIGATION,               NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_QUICK_MASK_OFF,           NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_QUICK_MASK_ON,            NULL,        0, 0, LIBGIMP_DOMAIN },
  { "gimp-qmask-off", /* compat */       NULL,        0, 0, LIBGIMP_DOMAIN },
  { "gimp-qmask-on",  /* compat */       NULL,        0, 0, LIBGIMP_DOMAIN },

  { GIMP_STOCK_CONTROLLER,               NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_CONTROLLER_KEYBOARD,      NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_CONTROLLER_LINUX_INPUT,   NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_CONTROLLER_MIDI,          NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_CONTROLLER_WHEEL,         NULL,        0, 0, LIBGIMP_DOMAIN },

  { GIMP_STOCK_DISPLAY_FILTER,           NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_DISPLAY_FILTER_COLORBLIND, NULL,       0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_DISPLAY_FILTER_CONTRAST,  NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_DISPLAY_FILTER_GAMMA,     NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_DISPLAY_FILTER_LCMS,      NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_DISPLAY_FILTER_PROOF,     NULL,        0, 0, LIBGIMP_DOMAIN },

  { GIMP_STOCK_LIST,                     NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_GRID,                     NULL,        0, 0, LIBGIMP_DOMAIN },

  { GIMP_STOCK_PORTRAIT,                 NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_LANDSCAPE,                NULL,        0, 0, LIBGIMP_DOMAIN },

  { GIMP_TOILET_PAPER,                   NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_WEB,                      NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_VIDEO,                    NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_GEGL,                     NULL,        0, 0, LIBGIMP_DOMAIN },

  { GIMP_STOCK_SHAPE_CIRCLE,             NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_SHAPE_DIAMOND,            NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_SHAPE_SQUARE,             NULL,        0, 0, LIBGIMP_DOMAIN },

  { GIMP_STOCK_CAP_BUTT,                 NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_CAP_ROUND,                NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_CAP_SQUARE,               NULL,        0, 0, LIBGIMP_DOMAIN },

  { GIMP_STOCK_JOIN_MITER,               NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_JOIN_ROUND,               NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_JOIN_BEVEL,               NULL,        0, 0, LIBGIMP_DOMAIN },

  { GIMP_STOCK_ERROR,                    NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_INFO,                     NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_QUESTION,                 NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_USER_MANUAL,              NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_WARNING,                  NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_WILBER,                   NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_WILBER_EEK,               NULL,        0, 0, LIBGIMP_DOMAIN },

  { GIMP_STOCK_FRAME,                    NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TEXTURE,                  NULL,        0, 0, LIBGIMP_DOMAIN },

  { GIMP_STOCK_TOOL_AIRBRUSH,            NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_ALIGN,               NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_BLEND,               NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_BLUR,                NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_BRIGHTNESS_CONTRAST, NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_BUCKET_FILL,         NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_BY_COLOR_SELECT,     NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_CAGE,                NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_CLONE,               NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_COLOR_BALANCE,       NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_COLOR_PICKER,        NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_COLORIZE,            NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_CROP,         N_("Cr_op"),        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_CURVES,              NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_DESATURATE,          NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_DODGE,               NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_ELLIPSE_SELECT,      NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_ERASER,              NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_FLIP,                NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_FREE_SELECT,         NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_FOREGROUND_SELECT, N_("_Select"), 0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_FUZZY_SELECT,        NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_HUE_SATURATION,      NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_HEAL,                NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_INK,                 NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_ISCISSORS,           NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_LEVELS,              NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_MEASURE,             NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_MOVE,                NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_PAINTBRUSH,          NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_PATH,                NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_PENCIL,              NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_PERSPECTIVE,  N_("_Transform"),   0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_PERSPECTIVE_CLONE,   NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_POSTERIZE,           NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_RECT_SELECT,         NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_ROTATE,       N_("_Rotate"),      0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_SCALE,        N_("_Scale"),       0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_SEAMLESS_CLONE,      NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_SHEAR,        N_("_Shear"),       0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_SMUDGE,              NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_TEXT,                NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_THRESHOLD,           NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_UNIFIED_TRANSFORM, N_("_Transform"), 0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_WARP,                NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_ZOOM,                NULL,        0, 0, LIBGIMP_DOMAIN }
};

static const struct
{
  const gchar   *stock_id;
  gconstpointer  inline_data;
}
gimp_stock_button_pixbufs[] =
{
  { GIMP_STOCK_ANCHOR,                   gimp_anchor_16                   },
  { GIMP_STOCK_CENTER,                   gimp_center_24                   },
  { GIMP_STOCK_DUPLICATE,                gimp_duplicate_16                },
  { GIMP_STOCK_EDIT,                     gimp_edit_16                     },
  { GIMP_STOCK_PASTE_AS_NEW,             gimp_paste_as_new_16             },
  { GIMP_STOCK_PASTE_INTO,               gimp_paste_into_16               },
  { GIMP_STOCK_RESET,                    gimp_reset_16                    },

  { GIMP_STOCK_GRAVITY_EAST,             gimp_gravity_east_24             },
  { GIMP_STOCK_GRAVITY_NORTH,            gimp_gravity_north_24            },
  { GIMP_STOCK_GRAVITY_NORTH_EAST,       gimp_gravity_north_east_24       },
  { GIMP_STOCK_GRAVITY_NORTH_WEST,       gimp_gravity_north_west_24       },
  { GIMP_STOCK_GRAVITY_SOUTH,            gimp_gravity_south_24            },
  { GIMP_STOCK_GRAVITY_SOUTH_EAST,       gimp_gravity_south_east_24       },
  { GIMP_STOCK_GRAVITY_SOUTH_WEST,       gimp_gravity_south_west_24       },
  { GIMP_STOCK_GRAVITY_WEST,             gimp_gravity_west_24             },

  { GIMP_STOCK_HCENTER,                  gimp_hcenter_24                  },
  { GIMP_STOCK_VCENTER,                  gimp_vcenter_24                  },

  { GIMP_STOCK_HFILL,                    gimp_hfill_24                    },
  { GIMP_STOCK_VFILL,                    gimp_vfill_24                    },

  { GIMP_STOCK_HCHAIN,                   gimp_hchain_24                   },
  { GIMP_STOCK_HCHAIN_BROKEN,            gimp_hchain_broken_24            },
  { GIMP_STOCK_VCHAIN,                   gimp_vchain_24                   },
  { GIMP_STOCK_VCHAIN_BROKEN,            gimp_vchain_broken_24            },

  { GIMP_STOCK_SELECTION,                gimp_selection_16                },
  { GIMP_STOCK_SELECTION_REPLACE,        gimp_selection_replace_16        },
  { GIMP_STOCK_SELECTION_ADD,            gimp_selection_add_16            },
  { GIMP_STOCK_SELECTION_SUBTRACT,       gimp_selection_subtract_16       },
  { GIMP_STOCK_SELECTION_INTERSECT,      gimp_selection_intersect_16      },
  { GIMP_STOCK_SELECTION_STROKE,         gimp_selection_stroke_16         },
  { GIMP_STOCK_SELECTION_TO_CHANNEL,     gimp_selection_to_channel_16     },
  { GIMP_STOCK_SELECTION_TO_PATH,        gimp_selection_to_path_16        },

  { GIMP_STOCK_PATH_STROKE,              gimp_path_stroke_16              },

  { GIMP_STOCK_CURVE_FREE,               gimp_curve_free_16               },
  { GIMP_STOCK_CURVE_SMOOTH,             gimp_curve_smooth_16             },

  { GIMP_STOCK_COLOR_PICKER_BLACK,       gimp_color_picker_black_18       },
  { GIMP_STOCK_COLOR_PICKER_GRAY,        gimp_color_picker_gray_18        },
  { GIMP_STOCK_COLOR_PICKER_WHITE,       gimp_color_picker_white_18       },
  { GIMP_STOCK_COLOR_TRIANGLE,           gimp_color_triangle_16           },
  { GIMP_STOCK_COLOR_PICK_FROM_SCREEN,   gimp_color_pick_from_screen_16   },

  { GIMP_STOCK_CHAR_PICKER,              gimp_char_picker_22              },
  { GIMP_STOCK_LETTER_SPACING,           gimp_letter_spacing_22           },
  { GIMP_STOCK_LINE_SPACING,             gimp_line_spacing_22             },
  { GIMP_STOCK_TEXT_DIR_LTR,             gimp_text_dir_ltr_24             },
  { GIMP_STOCK_TEXT_DIR_RTL,             gimp_text_dir_rtl_24             },
  { GIMP_STOCK_PRINT_RESOLUTION,         gimp_print_resolution_24         },

  { GIMP_STOCK_IMAGES,                   gimp_images_24                   },
  { GIMP_STOCK_LAYERS,                   gimp_layers_24                   },
  { GIMP_STOCK_CHANNELS,                 gimp_channels_24                 },
  { GIMP_STOCK_PATHS,                    gimp_paths_22                    },
  { GIMP_STOCK_TOOLS,                    gimp_tools_24                    },
  { GIMP_STOCK_TOOL_OPTIONS,             gimp_tool_options_24             },
  { GIMP_STOCK_DEVICE_STATUS,            gimp_device_status_24            },
  { GIMP_STOCK_INPUT_DEVICE,             gimp_input_device_22             },
  { GIMP_STOCK_CURSOR,                   gimp_cursor_24                   },
  { GIMP_STOCK_SAMPLE_POINT,             gimp_sample_point_24             },
  { GIMP_STOCK_DYNAMICS,                 gimp_dynamics_22                 },
  { GIMP_STOCK_TOOL_PRESET,              gimp_tool_preset_22              },

  { GIMP_STOCK_CONTROLLER,               gimp_controller_24               },
  { GIMP_STOCK_CONTROLLER_KEYBOARD,      gimp_controller_keyboard_24      },
  { GIMP_STOCK_CONTROLLER_LINUX_INPUT,   gimp_controller_linux_input_24   },
  { GIMP_STOCK_CONTROLLER_MIDI,          gimp_controller_midi_24          },
  { GIMP_STOCK_CONTROLLER_WHEEL,         gimp_controller_wheel_24         },

  { GIMP_STOCK_DISPLAY_FILTER,            gimp_display_filter_24            },
  { GIMP_STOCK_DISPLAY_FILTER_COLORBLIND, gimp_display_filter_colorblind_24 },
  { GIMP_STOCK_DISPLAY_FILTER_CONTRAST,   gimp_display_filter_contrast_24   },
  { GIMP_STOCK_DISPLAY_FILTER_GAMMA,      gimp_display_filter_gamma_24      },
  { GIMP_STOCK_DISPLAY_FILTER_LCMS,       gimp_display_filter_lcms_24       },
  { GIMP_STOCK_DISPLAY_FILTER_PROOF,      gimp_display_filter_proof_24      },

  { GIMP_STOCK_CHANNEL,                  gimp_channel_24                  },
  { GIMP_STOCK_CHANNEL_RED,              gimp_channel_red_24              },
  { GIMP_STOCK_CHANNEL_GREEN,            gimp_channel_green_24            },
  { GIMP_STOCK_CHANNEL_BLUE,             gimp_channel_blue_24             },
  { GIMP_STOCK_CHANNEL_GRAY,             gimp_channel_gray_24             },
  { GIMP_STOCK_CHANNEL_INDEXED,          gimp_channel_indexed_24          },
  { GIMP_STOCK_CHANNEL_ALPHA,            gimp_channel_alpha_24            },
  { GIMP_STOCK_LAYER_MASK,               gimp_layer_mask_24               },
  { GIMP_STOCK_IMAGE,                    gimp_image_24                    },
  { GIMP_STOCK_LAYER,                    gimp_layer_24                    },
  { GIMP_STOCK_TEXT_LAYER,               gimp_text_layer_24               },
  { GIMP_STOCK_FLOATING_SELECTION,       gimp_floating_selection_24       },
  { GIMP_STOCK_PATH,                     gimp_path_22                     },
  { GIMP_STOCK_TEMPLATE,                 gimp_template_24                 },
  { GIMP_STOCK_COLORMAP,                 gimp_colormap_24                 },
  { "gimp-indexed-palette", /* compat */ gimp_colormap_24                 },
  { GIMP_STOCK_HISTOGRAM,                gimp_histogram_22                },
  { GIMP_STOCK_UNDO_HISTORY,             gimp_undo_history_24             },
  { GIMP_STOCK_TRANSPARENCY,             gimp_transparency_24             },

  { GIMP_STOCK_LINKED,                   gimp_linked_20                   },
  { GIMP_STOCK_VISIBLE,                  gimp_eye_20                      },

  { GIMP_STOCK_MOVE_TO_SCREEN,           gimp_move_to_screen_24           },

  { GIMP_STOCK_TOOL_AIRBRUSH,            gimp_tool_airbrush_22            },
  { GIMP_STOCK_TOOL_ALIGN,               gimp_tool_align_22               },
  { GIMP_STOCK_TOOL_BLEND,               gimp_tool_blend_22               },
  { GIMP_STOCK_TOOL_BLUR,                gimp_tool_blur_22                },
  { GIMP_STOCK_TOOL_BRIGHTNESS_CONTRAST, gimp_tool_brightness_contrast_22 },
  { GIMP_STOCK_TOOL_BUCKET_FILL,         gimp_tool_bucket_fill_22         },
  { GIMP_STOCK_TOOL_BY_COLOR_SELECT,     gimp_tool_by_color_select_22     },
  { GIMP_STOCK_TOOL_CAGE,                gimp_tool_cage_22                },
  { GIMP_STOCK_TOOL_CLONE,               gimp_tool_clone_22               },
  { GIMP_STOCK_TOOL_COLOR_BALANCE,       gimp_tool_color_balance_22       },
  { GIMP_STOCK_TOOL_COLOR_PICKER,        gimp_tool_color_picker_22        },
  { GIMP_STOCK_TOOL_COLORIZE,            gimp_tool_colorize_22            },
  { GIMP_STOCK_TOOL_CROP,                gimp_tool_crop_22                },
  { GIMP_STOCK_TOOL_CURVES,              gimp_tool_curves_22              },
  { GIMP_STOCK_TOOL_DESATURATE,          gimp_tool_desaturate_22          },
  { GIMP_STOCK_TOOL_DODGE,               gimp_tool_dodge_22               },
  { GIMP_STOCK_TOOL_ELLIPSE_SELECT,      gimp_tool_ellipse_select_22      },
  { GIMP_STOCK_TOOL_ERASER,              gimp_tool_eraser_22              },
  { GIMP_STOCK_TOOL_FLIP,                gimp_tool_flip_22                },
  { GIMP_STOCK_TOOL_FREE_SELECT,         gimp_tool_free_select_22         },
  { GIMP_STOCK_TOOL_FOREGROUND_SELECT,   gimp_tool_foreground_select_22   },
  { GIMP_STOCK_TOOL_FUZZY_SELECT,        gimp_tool_fuzzy_select_22        },
  { GIMP_STOCK_TOOL_HEAL,                gimp_tool_heal_22                },
  { GIMP_STOCK_TOOL_HUE_SATURATION,      gimp_tool_hue_saturation_22      },
  { GIMP_STOCK_TOOL_INK,                 gimp_tool_ink_22                 },
  { GIMP_STOCK_TOOL_ISCISSORS,           gimp_tool_iscissors_22           },
  { GIMP_STOCK_TOOL_LEVELS,              gimp_tool_levels_22              },
  { GIMP_STOCK_TOOL_MEASURE,             gimp_tool_measure_22             },
  { GIMP_STOCK_TOOL_MOVE,                gimp_tool_move_22                },
  { GIMP_STOCK_TOOL_PAINTBRUSH,          gimp_tool_paintbrush_22          },
  { GIMP_STOCK_TOOL_PATH,                gimp_tool_path_22                },
  { GIMP_STOCK_TOOL_PENCIL,              gimp_tool_pencil_22              },
  { GIMP_STOCK_TOOL_PERSPECTIVE,         gimp_tool_perspective_22         },
  { GIMP_STOCK_TOOL_PERSPECTIVE_CLONE,   gimp_tool_perspective_clone_22   },
  { GIMP_STOCK_TOOL_POSTERIZE,           gimp_tool_posterize_22           },
  { GIMP_STOCK_TOOL_RECT_SELECT,         gimp_tool_rect_select_22         },
  { GIMP_STOCK_TOOL_ROTATE,              gimp_tool_rotate_22              },
  { GIMP_STOCK_TOOL_SCALE,               gimp_tool_scale_22               },
  { GIMP_STOCK_TOOL_SEAMLESS_CLONE,      gimp_tool_clone_22               },
  { GIMP_STOCK_TOOL_SHEAR,               gimp_tool_shear_22               },
  { GIMP_STOCK_TOOL_SMUDGE,              gimp_tool_smudge_22              },
  { GIMP_STOCK_TOOL_TEXT,                gimp_tool_text_22                },
  { GIMP_STOCK_TOOL_THRESHOLD,           gimp_tool_threshold_22           },
  { GIMP_STOCK_TOOL_UNIFIED_TRANSFORM,   gimp_tool_unified_transform_22   },
  { GIMP_STOCK_TOOL_WARP,                gimp_tool_warp_22                },
  { GIMP_STOCK_TOOL_ZOOM,                gimp_tool_zoom_22                },

  { GIMP_STOCK_INFO,                     gimp_info_24                     },
  { GIMP_STOCK_WARNING,                  gimp_warning_24                  },
  { GIMP_TOILET_PAPER,                   gimp_toilet_paper_24             },
  { GIMP_STOCK_WEB,                      gimp_web_24                      },
  { GIMP_STOCK_WILBER,                   gimp_wilber_22                   },
  { GIMP_STOCK_VIDEO,                    gimp_video_24                    },
  { GIMP_STOCK_GEGL,                     gimp_gegl_22                     }
};

static const struct
{
  const gchar   *stock_id;
  gconstpointer  inline_data;
}
gimp_stock_menu_pixbufs[] =
{
  { GIMP_STOCK_CENTER,                   gimp_center_16                   },

  { GIMP_STOCK_CONVERT_RGB,              gimp_convert_rgb_16              },
  { GIMP_STOCK_CONVERT_GRAYSCALE,        gimp_convert_grayscale_16        },
  { GIMP_STOCK_CONVERT_INDEXED,          gimp_convert_indexed_16          },
  { GIMP_STOCK_INVERT,                   gimp_invert_16                   },
  { GIMP_STOCK_MERGE_DOWN,               gimp_merge_down_16               },
  { GIMP_STOCK_LAYER_TO_IMAGESIZE,       gimp_layer_to_imagesize_16       },
  { GIMP_STOCK_PLUGIN,                   gimp_plugin_16                   },
  { GIMP_STOCK_RESHOW_FILTER,            gimp_reshow_filter_16            },
  { GIMP_STOCK_ROTATE_90,                gimp_rotate_90_16                },
  { GIMP_STOCK_ROTATE_180,               gimp_rotate_180_16               },
  { GIMP_STOCK_ROTATE_270,               gimp_rotate_270_16               },
  { GIMP_STOCK_RESIZE,                   gimp_resize_16                   },
  { GIMP_STOCK_SCALE,                    gimp_scale_16                    },
  { GIMP_STOCK_FLIP_HORIZONTAL,          gimp_flip_horizontal_16          },
  { GIMP_STOCK_FLIP_VERTICAL,            gimp_flip_vertical_16            },

  { GIMP_STOCK_PRINT_RESOLUTION,         gimp_print_resolution_16         },

  { GIMP_STOCK_IMAGES,                   gimp_images_16                   },
  { GIMP_STOCK_LAYERS,                   gimp_layers_16                   },
  { GIMP_STOCK_CHANNELS,                 gimp_channels_16                 },
  { GIMP_STOCK_PATHS,                    gimp_paths_16                    },
  { GIMP_STOCK_TOOLS,                    gimp_tools_16                    },
  { GIMP_STOCK_TOOL_OPTIONS,             gimp_tool_options_16             },
  { GIMP_STOCK_DEVICE_STATUS,            gimp_device_status_16            },
  { GIMP_STOCK_INPUT_DEVICE,             gimp_input_device_16             },
  { GIMP_STOCK_CURSOR,                   gimp_cursor_16                   },
  { GIMP_STOCK_SAMPLE_POINT,             gimp_sample_point_16             },
  { GIMP_STOCK_DYNAMICS,                 gimp_dynamics_16                 },
  { GIMP_STOCK_TOOL_PRESET,              gimp_tool_preset_16              },

  { GIMP_STOCK_CONTROLLER,               gimp_controller_16               },
  { GIMP_STOCK_CONTROLLER_KEYBOARD,      gimp_controller_keyboard_16      },
  { GIMP_STOCK_CONTROLLER_LINUX_INPUT,   gimp_controller_linux_input_16   },
  { GIMP_STOCK_CONTROLLER_MIDI,          gimp_controller_midi_16          },
  { GIMP_STOCK_CONTROLLER_WHEEL,         gimp_controller_wheel_16         },

  { GIMP_STOCK_DISPLAY_FILTER,            gimp_display_filter_16            },
  { GIMP_STOCK_DISPLAY_FILTER_COLORBLIND, gimp_display_filter_colorblind_16 },
  { GIMP_STOCK_DISPLAY_FILTER_CONTRAST,   gimp_display_filter_contrast_16   },
  { GIMP_STOCK_DISPLAY_FILTER_GAMMA,      gimp_display_filter_gamma_16      },
  { GIMP_STOCK_DISPLAY_FILTER_LCMS,       gimp_display_filter_lcms_16       },
  { GIMP_STOCK_DISPLAY_FILTER_PROOF,      gimp_display_filter_proof_16      },

  { GIMP_STOCK_CHANNEL,                  gimp_channel_16                  },
  { GIMP_STOCK_CHANNEL_RED,              gimp_channel_red_16              },
  { GIMP_STOCK_CHANNEL_GREEN,            gimp_channel_green_16            },
  { GIMP_STOCK_CHANNEL_BLUE,             gimp_channel_blue_16             },
  { GIMP_STOCK_CHANNEL_GRAY,             gimp_channel_gray_16             },
  { GIMP_STOCK_CHANNEL_INDEXED,          gimp_channel_indexed_16          },
  { GIMP_STOCK_CHANNEL_ALPHA,            gimp_channel_alpha_16            },
  { GIMP_STOCK_LAYER_MASK,               gimp_layer_mask_16               },
  { GIMP_STOCK_IMAGE,                    gimp_image_16                    },
  { GIMP_STOCK_LAYER,                    gimp_layer_16                    },
  { GIMP_STOCK_TEXT_LAYER,               gimp_text_layer_16               },
  { GIMP_STOCK_FLOATING_SELECTION,       gimp_floating_selection_16       },
  { GIMP_STOCK_PATH,                     gimp_path_16                     },
  { GIMP_STOCK_TEMPLATE,                 gimp_template_16                 },
  { GIMP_STOCK_COLORMAP,                 gimp_colormap_16                 },
  { "gimp-indexed-palette", /* compat */ gimp_colormap_16                 },
  { GIMP_STOCK_HISTOGRAM,                gimp_histogram_16                },
  { GIMP_STOCK_HISTOGRAM_LINEAR,         gimp_histogram_linear_16         },
  { GIMP_STOCK_HISTOGRAM_LOGARITHMIC,    gimp_histogram_logarithmic_16    },
  { GIMP_STOCK_UNDO_HISTORY,             gimp_undo_history_16             },
  { GIMP_STOCK_TRANSPARENCY,             gimp_transparency_16             },

  { GIMP_STOCK_LINKED,                   gimp_linked_12                   },
  { GIMP_STOCK_VISIBLE,                  gimp_eye_12                      },

  { GIMP_STOCK_SELECTION_ALL,            gimp_selection_all_16            },
  { GIMP_STOCK_SELECTION_NONE,           gimp_selection_none_16           },
  { GIMP_STOCK_SELECTION_GROW,           gimp_selection_grow_16           },
  { GIMP_STOCK_SELECTION_SHRINK,         gimp_selection_shrink_16         },
  { GIMP_STOCK_SELECTION_BORDER,         gimp_selection_border_16         },

  { GIMP_STOCK_NAVIGATION,               gimp_navigation_16               },
  { GIMP_STOCK_QUICK_MASK_OFF,           gimp_quick_mask_off_12           },
  { GIMP_STOCK_QUICK_MASK_ON,            gimp_quick_mask_on_12            },
  { "gimp-qmask-off", /* compat */       gimp_quick_mask_off_12           },
  { "gimp-qmask-on",  /* compat */       gimp_quick_mask_on_12            },

  { GIMP_STOCK_LIST,                     gimp_list_16                     },
  { GIMP_STOCK_GRID,                     gimp_grid_16                     },

  { GIMP_STOCK_PORTRAIT,                 gimp_portrait_16                 },
  { GIMP_STOCK_LANDSCAPE,                gimp_landscape_16                },

  { GIMP_STOCK_CLOSE,                    gimp_close_12                    },
  { GIMP_STOCK_MOVE_TO_SCREEN,           gimp_move_to_screen_16           },
  { GIMP_STOCK_DEFAULT_COLORS,           gimp_default_colors_12           },
  { GIMP_STOCK_SWAP_COLORS,              gimp_swap_colors_12              },
  { GIMP_STOCK_ZOOM_FOLLOW_WINDOW,       gimp_zoom_follow_window_12       },

  { GIMP_STOCK_GRADIENT_LINEAR,               gimp_gradient_linear_16               },
  { GIMP_STOCK_GRADIENT_BILINEAR,             gimp_gradient_bilinear_16             },
  { GIMP_STOCK_GRADIENT_RADIAL,               gimp_gradient_radial_16               },
  { GIMP_STOCK_GRADIENT_SQUARE,               gimp_gradient_square_16               },
  { GIMP_STOCK_GRADIENT_CONICAL_SYMMETRIC,    gimp_gradient_conical_symmetric_16    },
  { GIMP_STOCK_GRADIENT_CONICAL_ASYMMETRIC,   gimp_gradient_conical_asymmetric_16   },
  { GIMP_STOCK_GRADIENT_SHAPEBURST_ANGULAR,   gimp_gradient_shapeburst_angular_16   },
  { GIMP_STOCK_GRADIENT_SHAPEBURST_SPHERICAL, gimp_gradient_shapeburst_spherical_16 },
  { GIMP_STOCK_GRADIENT_SHAPEBURST_DIMPLED,   gimp_gradient_shapeburst_dimpled_16   },
  { GIMP_STOCK_GRADIENT_SPIRAL_CLOCKWISE,     gimp_gradient_spiral_clockwise_16     },
  { GIMP_STOCK_GRADIENT_SPIRAL_ANTICLOCKWISE, gimp_gradient_spiral_anticlockwise_16 },

  { GIMP_STOCK_TOOL_AIRBRUSH,            gimp_tool_airbrush_16            },
  { GIMP_STOCK_TOOL_ALIGN,               gimp_tool_align_16               },
  { GIMP_STOCK_TOOL_BLEND,               gimp_tool_blend_16               },
  { GIMP_STOCK_TOOL_BLUR,                gimp_tool_blur_16                },
  { GIMP_STOCK_TOOL_BRIGHTNESS_CONTRAST, gimp_tool_brightness_contrast_16 },
  { GIMP_STOCK_TOOL_BUCKET_FILL,         gimp_tool_bucket_fill_16         },
  { GIMP_STOCK_TOOL_BY_COLOR_SELECT,     gimp_tool_by_color_select_16     },
  { GIMP_STOCK_TOOL_CAGE,                gimp_tool_cage_16                },
  { GIMP_STOCK_TOOL_CLONE,               gimp_tool_clone_16               },
  { GIMP_STOCK_TOOL_COLOR_BALANCE,       gimp_tool_color_balance_16       },
  { GIMP_STOCK_TOOL_COLOR_PICKER,        gimp_tool_color_picker_16        },
  { GIMP_STOCK_TOOL_COLORIZE,            gimp_tool_colorize_16            },
  { GIMP_STOCK_TOOL_CROP,                gimp_tool_crop_16                },
  { GIMP_STOCK_TOOL_CURVES,              gimp_tool_curves_16              },
  { GIMP_STOCK_TOOL_DESATURATE,          gimp_tool_desaturate_16          },
  { GIMP_STOCK_TOOL_DODGE,               gimp_tool_dodge_16               },
  { GIMP_STOCK_TOOL_ELLIPSE_SELECT,      gimp_tool_ellipse_select_16      },
  { GIMP_STOCK_TOOL_ERASER,              gimp_tool_eraser_16              },
  { GIMP_STOCK_TOOL_FLIP,                gimp_tool_flip_16                },
  { GIMP_STOCK_TOOL_FREE_SELECT,         gimp_tool_free_select_16         },
  { GIMP_STOCK_TOOL_FOREGROUND_SELECT,   gimp_tool_foreground_select_16   },
  { GIMP_STOCK_TOOL_FUZZY_SELECT,        gimp_tool_fuzzy_select_16        },
  { GIMP_STOCK_TOOL_HEAL,                gimp_tool_heal_16                },
  { GIMP_STOCK_TOOL_HUE_SATURATION,      gimp_tool_hue_saturation_16      },
  { GIMP_STOCK_TOOL_INK,                 gimp_tool_ink_16                 },
  { GIMP_STOCK_TOOL_ISCISSORS,           gimp_tool_iscissors_16           },
  { GIMP_STOCK_TOOL_LEVELS,              gimp_tool_levels_16              },
  { GIMP_STOCK_TOOL_MEASURE,             gimp_tool_measure_16             },
  { GIMP_STOCK_TOOL_MOVE,                gimp_tool_move_16                },
  { GIMP_STOCK_TOOL_PAINTBRUSH,          gimp_tool_paintbrush_16          },
  { GIMP_STOCK_TOOL_PATH,                gimp_tool_path_16                },
  { GIMP_STOCK_TOOL_PENCIL,              gimp_tool_pencil_16              },
  { GIMP_STOCK_TOOL_PERSPECTIVE,         gimp_tool_perspective_16         },
  { GIMP_STOCK_TOOL_PERSPECTIVE_CLONE,   gimp_tool_perspective_clone_16   },
  { GIMP_STOCK_TOOL_POSTERIZE,           gimp_tool_posterize_16           },
  { GIMP_STOCK_TOOL_RECT_SELECT,         gimp_tool_rect_select_16         },
  { GIMP_STOCK_TOOL_ROTATE,              gimp_tool_rotate_16              },
  { GIMP_STOCK_TOOL_SCALE,               gimp_tool_scale_16               },
  { GIMP_STOCK_TOOL_SEAMLESS_CLONE,      gimp_tool_clone_16               },
  { GIMP_STOCK_TOOL_SHEAR,               gimp_tool_shear_16               },
  { GIMP_STOCK_TOOL_SMUDGE,              gimp_tool_smudge_16              },
  { GIMP_STOCK_TOOL_TEXT,                gimp_tool_text_16                },
  { GIMP_STOCK_TOOL_THRESHOLD,           gimp_tool_threshold_16           },
  { GIMP_STOCK_TOOL_UNIFIED_TRANSFORM,   gimp_tool_unified_transform_16   },
  { GIMP_STOCK_TOOL_WARP,                gimp_tool_warp_16                },
  { GIMP_STOCK_TOOL_ZOOM,                gimp_tool_zoom_16                },

  { GIMP_STOCK_INFO,                     gimp_info_16                     },
  { GIMP_STOCK_WARNING,                  gimp_warning_16                  },
  { GIMP_STOCK_WILBER,                   gimp_wilber_16                   },
  { GIMP_TOILET_PAPER,                   gimp_toilet_paper_16             },
  { GIMP_STOCK_USER_MANUAL,              gimp_user_manual_16              },
  { GIMP_STOCK_WEB,                      gimp_web_16                      },
  { GIMP_STOCK_VIDEO,                    gimp_video_16                    },
  { GIMP_STOCK_GEGL,                     gimp_gegl_16                     },

  { GIMP_STOCK_SHAPE_CIRCLE,             gimp_shape_circle_16             },
  { GIMP_STOCK_SHAPE_SQUARE,             gimp_shape_square_16             },
  { GIMP_STOCK_SHAPE_DIAMOND,            gimp_shape_diamond_16            },
  { GIMP_STOCK_CAP_BUTT,                 gimp_cap_butt_16                 },
  { GIMP_STOCK_CAP_ROUND,                gimp_cap_round_16                },
  { GIMP_STOCK_CAP_SQUARE,               gimp_cap_square_16               },
  { GIMP_STOCK_JOIN_MITER,               gimp_join_miter_16               },
  { GIMP_STOCK_JOIN_ROUND,               gimp_join_round_16               },
  { GIMP_STOCK_JOIN_BEVEL,               gimp_join_bevel_16               }
};

static const struct
{
  const gchar   *stock_id;
  gconstpointer  inline_data;
}
gimp_stock_dnd_pixbufs[] =
{
  { GIMP_STOCK_CHANNEL,              gimp_channel_32              },
  { GIMP_STOCK_CHANNEL_RED,          gimp_channel_red_32          },
  { GIMP_STOCK_CHANNEL_GREEN,        gimp_channel_green_32        },
  { GIMP_STOCK_CHANNEL_BLUE,         gimp_channel_blue_32         },
  { GIMP_STOCK_CHANNEL_GRAY,         gimp_channel_gray_32         },
  { GIMP_STOCK_CHANNEL_INDEXED,      gimp_channel_indexed_32      },
  { GIMP_STOCK_CHANNEL_ALPHA,        gimp_channel_alpha_32        },
  { GIMP_STOCK_FLOATING_SELECTION,   gimp_floating_selection_32   },
  { GIMP_STOCK_LAYER_MASK,           gimp_layer_mask_32           },
  { GIMP_STOCK_IMAGE,                gimp_image_32                },
  { GIMP_STOCK_LAYER,                gimp_layer_32                },
  { GIMP_STOCK_TEXT_LAYER,           gimp_text_layer_32           },
  { GIMP_STOCK_USER_MANUAL,          gimp_user_manual_32          }
};

static const struct
{
  const gchar   *stock_id;
  gconstpointer  inline_data;
}
gimp_stock_dialog_pixbufs[] =
{
  { GIMP_STOCK_CHANNEL,              gimp_channel_48              },
  { GIMP_STOCK_CHANNEL_RED,          gimp_channel_red_48          },
  { GIMP_STOCK_CHANNEL_GREEN,        gimp_channel_green_48        },
  { GIMP_STOCK_CHANNEL_BLUE,         gimp_channel_blue_48         },
  { GIMP_STOCK_CHANNEL_GRAY,         gimp_channel_gray_48         },
  { GIMP_STOCK_CHANNEL_INDEXED,      gimp_channel_indexed_48      },
  { GIMP_STOCK_CHANNEL_ALPHA,        gimp_channel_alpha_48        },
  { GIMP_STOCK_FLOATING_SELECTION,   gimp_floating_selection_48   },
  { GIMP_STOCK_LAYER_MASK,           gimp_layer_mask_48           },
  { GIMP_STOCK_IMAGE,                gimp_image_48                },
  { GIMP_STOCK_LAYER,                gimp_layer_48                },
  { GIMP_STOCK_TEXT_LAYER,           gimp_text_layer_48           },

  { GIMP_STOCK_ERROR,                gimp_error_64                },
  { GIMP_STOCK_INFO,                 gimp_info_64                 },
  { GIMP_STOCK_QUESTION,             gimp_question_64             },
  { GIMP_STOCK_USER_MANUAL,          gimp_user_manual_64          },
  { GIMP_STOCK_WARNING,              gimp_warning_64              },
  { GIMP_STOCK_WILBER,               gimp_wilber_64               },
  { GIMP_STOCK_WILBER_EEK,           gimp_wilber_eek_64           },

  { GIMP_STOCK_FRAME,                gimp_frame_64                },
  { GIMP_STOCK_TEXTURE,              gimp_texture_64              }
};

static const struct
{
  const gchar   *stock_id;
  gconstpointer  inline_data_ltr;
  gconstpointer  inline_data_rtl;
}
gimp_stock_direction_pixbufs[] =
{
  { GIMP_STOCK_MENU_LEFT,  gimp_menu_left_12,  gimp_menu_right_12 },
  { GIMP_STOCK_MENU_RIGHT, gimp_menu_right_12, gimp_menu_left_12  }
};


/**
 * gimp_stock_init:
 *
 * Initializes the GIMP stock icon factory.
 *
 * You don't need to call this function as gimp_ui_init() already does
 * this for you.
 */
void
gimp_stock_init (void)
{
  static gboolean initialized = FALSE;

  gchar *icons_dir;
  gint   i;

  if (initialized)
    return;

  gimp_stock_factory = gtk_icon_factory_new ();

  for (i = 0; i < G_N_ELEMENTS (gimp_stock_dialog_pixbufs); i++)
    {
      add_sized_with_same_fallback (gimp_stock_factory,
                                    gimp_stock_dialog_pixbufs[i].inline_data,
                                    NULL,
                                    GTK_ICON_SIZE_DIALOG,
                                    gimp_stock_dialog_pixbufs[i].stock_id);
    }

  for (i = 0; i < G_N_ELEMENTS (gimp_stock_dnd_pixbufs); i++)
    {
      add_sized_with_same_fallback (gimp_stock_factory,
                                    gimp_stock_dnd_pixbufs[i].inline_data,
                                    NULL,
                                    GTK_ICON_SIZE_DND,
                                    gimp_stock_dnd_pixbufs[i].stock_id);
    }

  for (i = 0; i < G_N_ELEMENTS (gimp_stock_button_pixbufs); i++)
    {
      add_sized_with_same_fallback (gimp_stock_factory,
                                    gimp_stock_button_pixbufs[i].inline_data,
                                    NULL,
                                    GTK_ICON_SIZE_BUTTON,
                                    gimp_stock_button_pixbufs[i].stock_id);
    }

  for (i = 0; i < G_N_ELEMENTS (gimp_stock_menu_pixbufs); i++)
    {
      add_sized_with_same_fallback (gimp_stock_factory,
                                    gimp_stock_menu_pixbufs[i].inline_data,
                                    NULL,
                                    GTK_ICON_SIZE_MENU,
                                    gimp_stock_menu_pixbufs[i].stock_id);
    }

  for (i = 0; i < G_N_ELEMENTS (gimp_stock_direction_pixbufs); i++)
    {
      add_sized_with_same_fallback (gimp_stock_factory,
                                    gimp_stock_direction_pixbufs[i].inline_data_ltr,
                                    gimp_stock_direction_pixbufs[i].inline_data_rtl,
                                    GTK_ICON_SIZE_MENU,
                                    gimp_stock_direction_pixbufs[i].stock_id);
    }

  gtk_icon_factory_add_default (gimp_stock_factory);

  gtk_stock_add_static (gimp_stock_items, G_N_ELEMENTS (gimp_stock_items));

  icons_dir = g_build_filename (gimp_data_directory (), "icons", NULL);
  gtk_icon_theme_prepend_search_path (gtk_icon_theme_get_default (),
                                      icons_dir);
  g_free (icons_dir);

  initialized = TRUE;
}
