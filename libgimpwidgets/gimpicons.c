/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpicons.c
 * Copyright (C) 2001-2015 Michael Natterer <mitch@gimp.org>
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
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#undef GIMP_DISABLE_DEPRECATED
#include "gimpicons.h"

#include "icons/Color/gimp-icon-pixbufs.c"

#include "libgimp/libgimp-intl.h"


/**
 * SECTION: gimpicons
 * @title: GimpIcons
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


#define LIBGIMP_DOMAIN          GETTEXT_PACKAGE "-libgimp"
#define GIMP_TOILET_PAPER       "gimp-toilet-paper"
#define GIMP_DEFAULT_ICON_THEME "Symbolic"


static GtkIconFactory *gimp_stock_factory = NULL;


static const GtkStockItem gimp_stock_items[] =
{
  { GIMP_STOCK_ANCHOR,         N_("Anchor"),          0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_CENTER,         N_("C_enter"),         0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_DUPLICATE,      N_("_Duplicate"),      0, 0, LIBGIMP_DOMAIN },
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
  { GIMP_STOCK_PATTERN,                  NULL,        0, 0, LIBGIMP_DOMAIN },

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
  { GIMP_STOCK_GEGL,                     NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_WEB,                      NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_VIDEO,                    NULL,        0, 0, LIBGIMP_DOMAIN },

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
  { GIMP_STOCK_TOOL_SHEAR,        N_("_Shear"),       0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_SMUDGE,              NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_TEXT,                NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_THRESHOLD,           NULL,        0, 0, LIBGIMP_DOMAIN },
  { GIMP_STOCK_TOOL_ZOOM,                NULL,        0, 0, LIBGIMP_DOMAIN }
};

static const GtkStockItem gimp_compat_stock_items[] =
{
  { "gimp-indexed-palette", NULL, 0, 0, LIBGIMP_DOMAIN },
  { "gimp-qmask-off",       NULL, 0, 0, LIBGIMP_DOMAIN },
  { "gimp-qmask-on",        NULL, 0, 0, LIBGIMP_DOMAIN }
};


static void
register_stock_icon (GtkIconFactory *factory,
                     const gchar    *stock_id,
                     const gchar    *icon_name)
{
  GtkIconSet    *set    = gtk_icon_set_new ();
  GtkIconSource *source = gtk_icon_source_new ();

  gtk_icon_source_set_direction_wildcarded (source, TRUE);
  gtk_icon_source_set_size_wildcarded (source, TRUE);
  gtk_icon_source_set_state_wildcarded (source, TRUE);
  gtk_icon_source_set_icon_name (source, icon_name);

  gtk_icon_set_add_source (set, source);
  gtk_icon_source_free (source);

  gtk_icon_factory_add (factory, stock_id, set);
  gtk_icon_set_unref (set);
}

static void
register_bidi_stock_icon (GtkIconFactory *factory,
                          const gchar    *stock_id,
                          const gchar    *icon_name_ltr,
                          const gchar    *icon_name_rtl)
{
  GtkIconSet    *set    = gtk_icon_set_new ();
  GtkIconSource *source = gtk_icon_source_new ();

  gtk_icon_source_set_direction (source, GTK_TEXT_DIR_LTR);
  gtk_icon_source_set_direction_wildcarded (source, FALSE);
  gtk_icon_source_set_size_wildcarded (source, TRUE);
  gtk_icon_source_set_state_wildcarded (source, TRUE);
  gtk_icon_source_set_icon_name (source, icon_name_ltr);

  gtk_icon_set_add_source (set, source);
  gtk_icon_source_free (source);

  source = gtk_icon_source_new ();

  gtk_icon_source_set_direction (source, GTK_TEXT_DIR_RTL);
  gtk_icon_source_set_direction_wildcarded (source, FALSE);
  gtk_icon_source_set_size_wildcarded (source, TRUE);
  gtk_icon_source_set_state_wildcarded (source, TRUE);
  gtk_icon_source_set_icon_name (source, icon_name_rtl);

  gtk_icon_set_add_source (set, source);
  gtk_icon_source_free (source);

  gtk_icon_factory_add (factory, stock_id, set);
  gtk_icon_set_unref (set);
}


static GFile *icon_theme_path     = NULL;
static GFile *default_search_path = NULL;


static void
gimp_icons_change_icon_theme (GFile *new_search_path)
{
  GFile *old_search_path = g_file_get_parent (icon_theme_path);

  if (! default_search_path)
    default_search_path = gimp_data_directory_file ("icons", NULL);

  if (! g_file_equal (new_search_path, old_search_path))
    {
      GtkIconTheme *icon_theme = gtk_icon_theme_get_default ();

      if (g_file_equal (old_search_path, default_search_path))
        {
          /*  if the old icon theme is in the default search path,
           *  simply prepend the new theme's path
           */
          gchar *path_str = g_file_get_path (new_search_path);

          gtk_icon_theme_prepend_search_path (icon_theme, path_str);
          g_free (path_str);
        }
      else
        {
          /*  if the old theme is not in the default search path,
           *  we need to deal with the search path's first element
           */
          gchar **paths;
          gint    n_paths;

          gtk_icon_theme_get_search_path (icon_theme, &paths, &n_paths);

          if (g_file_equal (new_search_path, default_search_path))
            {
              /*  when switching to a theme in the default path, remove
               *  the first search path element, the default search path
               *  is still in the search path
               */
              gtk_icon_theme_set_search_path (icon_theme,
                                              (const gchar **) paths + 1,
                                              n_paths - 1);
            }
          else
            {
              /*  when switching between two non-default search paths, replace
               *  the first element of the search path with the new
               *  theme's path
               */
              g_free (paths[0]);
              paths[0] = g_file_get_path (new_search_path);

              gtk_icon_theme_set_search_path (icon_theme,
                                              (const gchar **) paths, n_paths);
            }

          g_strfreev (paths);
        }
    }

  g_object_unref (old_search_path);
}

static void
gimp_icons_notify_system_icon_theme (GObject    *settings,
                                     GParamSpec *param,
                                     gpointer    unused)
{
  GdkScreen *screen = gdk_screen_get_default ();
  GValue     value  = G_VALUE_INIT;

  g_value_init (&value, G_TYPE_STRING);

  if (gdk_screen_get_setting (screen, "gtk-icon-theme-name", &value))
    {
      const gchar *new_system_icon_theme = g_value_get_string (&value);
      gchar *cur_system_icon_theme = NULL;

      g_object_get (settings,
                    "gtk-fallback-icon-theme", &cur_system_icon_theme,
                    NULL);
      if (g_strcmp0 (cur_system_icon_theme, new_system_icon_theme))
        {
          g_object_set (settings,
                        "gtk-fallback-icon-theme", new_system_icon_theme,
                        NULL);

          g_object_notify (settings, "gtk-icon-theme-name");
        }

      g_free (cur_system_icon_theme);
    }

  g_value_unset (&value);
}

static gboolean
gimp_icons_sanity_check (GFile       *path,
                         const gchar *theme_name)
{
  gboolean exists = FALSE;
  GFile *child = g_file_get_child (path, theme_name);

  if (g_file_query_exists (child, NULL))
    {
      GFile *index = g_file_get_child (child, "index.theme");

      if (g_file_query_exists (index, NULL))
        exists = TRUE;
      else
        g_printerr ("%s: Icon theme path has no '%s/index.theme': %s\n",
                    G_STRFUNC, theme_name, gimp_file_get_utf8_name (path));

      g_object_unref (index);
    }
  else
    g_printerr ("%s: Icon theme path has no '%s' subdirectory: %s\n",
                G_STRFUNC, theme_name, gimp_file_get_utf8_name (path));

  g_object_unref (child);

  return exists;
}

void
gimp_icons_set_icon_theme (GFile *path)
{
  gchar *icon_theme_name;
  GFile *search_path;

  g_return_if_fail (path == NULL || G_IS_FILE (path));

  if (path)
    path = g_object_ref (path);
  else
    path = gimp_data_directory_file ("icons", GIMP_DEFAULT_ICON_THEME, NULL);

  search_path = g_file_get_parent (path);
  icon_theme_name = g_file_get_basename (path);

  if (gimp_icons_sanity_check (search_path, "hicolor") &&
      gimp_icons_sanity_check (search_path, icon_theme_name))
    {
      if (icon_theme_path)
        {
          /*  this is an icon theme change  */
          gimp_icons_change_icon_theme (search_path);

          if (! g_file_equal (icon_theme_path, path))
            {
              g_object_unref (icon_theme_path);
              icon_theme_path = g_object_ref (path);
            }

          g_object_set (gtk_settings_get_for_screen (gdk_screen_get_default ()),
                        "gtk-icon-theme-name", icon_theme_name,
                        NULL);
        }
      else
        {
          /*  this is the first call upon initialization  */
          icon_theme_path = g_object_ref (path);
        }
    }

  g_free (icon_theme_name);
  g_object_unref (search_path);
  g_object_unref (path);
}

/**
 * gimp_stock_init:
 *
 * Initializes the GIMP stock icon factory.
 *
 * You don't need to call this function as gimp_ui_init() already does
 * this for you.
 *
 * Deprecated: 2.10: Use gimp_icons_init() instead.
 */
void
gimp_stock_init (void)
{
  gimp_icons_init ();
}

/**
 * gimp_icons_init:
 *
 * Initializes the GIMP stock icon factory.
 *
 * You don't need to call this function as gimp_ui_init() already does
 * this for you.
 */
void
gimp_icons_init (void)
{
  static gboolean initialized = FALSE;

  GtkSettings *settings;
  GdkPixbuf   *pixbuf;
  GError      *error = NULL;
  gchar       *icons_dir;
  gchar       *system_icon_theme;
  gchar       *gimp_icon_theme;
  gint         i;

  if (initialized)
    return;

  gimp_stock_factory = gtk_icon_factory_new ();

  for (i = 0; i < G_N_ELEMENTS (gimp_stock_items); i++)
    {
      register_stock_icon (gimp_stock_factory,
                           gimp_stock_items[i].stock_id,
                           gimp_stock_items[i].stock_id);
    }

  register_bidi_stock_icon (gimp_stock_factory,
                            GIMP_STOCK_MENU_LEFT,
                            GIMP_STOCK_MENU_LEFT, GIMP_STOCK_MENU_RIGHT);
  register_bidi_stock_icon (gimp_stock_factory,
                            GIMP_STOCK_MENU_RIGHT,
                            GIMP_STOCK_MENU_RIGHT, GIMP_STOCK_MENU_LEFT);

  register_stock_icon (gimp_stock_factory,
                       "gimp-indexed-palette", GIMP_STOCK_COLORMAP);
  register_stock_icon (gimp_stock_factory,
                       "gimp-qmask-off", GIMP_STOCK_QUICK_MASK_OFF);
  register_stock_icon (gimp_stock_factory,
                       "gimp-qmask-on", GIMP_STOCK_QUICK_MASK_ON);
  register_stock_icon (gimp_stock_factory,
                       "gimp-tool-blend", GIMP_STOCK_TOOL_BLEND);

  gtk_icon_factory_add_default (gimp_stock_factory);

  gtk_stock_add_static (gimp_stock_items,
                        G_N_ELEMENTS (gimp_stock_items));
  gtk_stock_add_static (gimp_compat_stock_items,
                        G_N_ELEMENTS (gimp_compat_stock_items));

  /*  always prepend the default icon theme, it's never removed from
   *  the path again and acts as fallback for missing icons in other
   *  themes.
   */
  if (! default_search_path)
    default_search_path = gimp_data_directory_file ("icons",
                                                    NULL);

  icons_dir = g_file_get_path (default_search_path);
  gtk_icon_theme_prepend_search_path (gtk_icon_theme_get_default (),
                                      icons_dir);
  g_free (icons_dir);

  /*  if an icon theme was chosen before init(), change to it  */
  if (icon_theme_path)
    {
      GFile *search_path = g_file_get_parent (icon_theme_path);

      if (!g_file_equal (search_path, default_search_path))
        {
          gchar *icon_dir = g_file_get_path (search_path);

          gtk_icon_theme_prepend_search_path (gtk_icon_theme_get_default (),
                                              icon_dir);
          g_free (icon_dir);
        }
      g_object_unref (search_path);

      gimp_icon_theme = g_file_get_basename (icon_theme_path);
    }
  else
    {
      gimp_icon_theme = g_strdup (GIMP_DEFAULT_ICON_THEME);
    }

  settings = gtk_settings_get_for_screen (gdk_screen_get_default ());

  g_object_get (settings, "gtk-icon-theme-name", &system_icon_theme, NULL);

  g_object_set (settings,
                "gtk-fallback-icon-theme", system_icon_theme,
                "gtk-icon-theme-name", gimp_icon_theme,
                NULL);

  g_free (gimp_icon_theme);
  g_free (system_icon_theme);

  g_signal_connect (settings, "notify::gtk-icon-theme-name",
                    G_CALLBACK (gimp_icons_notify_system_icon_theme), NULL);
  pixbuf = gdk_pixbuf_new_from_resource ("/org/gimp/icons/64/gimp-wilber-eek.png",
                                         &error);

  if (pixbuf)
    {
      gtk_icon_theme_add_builtin_icon (GIMP_STOCK_WILBER_EEK, 64, pixbuf);
      g_object_unref (pixbuf);
    }
  else
    {
      g_critical ("Failed to create icon image: %s", error->message);
      g_clear_error (&error);
    }

  initialized = TRUE;
}
