/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#include "widgets-types.h"

#include "gimpwidgets-constructors.h"

#include "gimp-intl.h"


GtkWidget *
gimp_paint_mode_menu_new (GCallback            callback,
			  gpointer             data,
			  gboolean             with_behind_mode,
			  GimpLayerModeEffects initial)
{
  GtkWidget *menu;

  if (with_behind_mode)
    {
      menu = gimp_int_option_menu_new
        (FALSE, callback, data, initial,

         _("Normal"),        GIMP_NORMAL_MODE,        NULL,
         _("Dissolve"),      GIMP_DISSOLVE_MODE,      NULL,
         _("Behind"),        GIMP_BEHIND_MODE,        NULL,
         _("Color Erase"),   GIMP_COLOR_ERASE_MODE,   NULL,
	 "---",              0,                       NULL,
         _("Multiply"),      GIMP_MULTIPLY_MODE,      NULL,
         _("Divide"),        GIMP_DIVIDE_MODE,        NULL,
         _("Screen"),        GIMP_SCREEN_MODE,        NULL,
         _("Overlay"),       GIMP_OVERLAY_MODE,       NULL,
	 "---",              0,                       NULL,
         _("Dodge"),         GIMP_DODGE_MODE,         NULL,
         _("Burn"),          GIMP_BURN_MODE,          NULL,
         _("Hard Light"),    GIMP_HARDLIGHT_MODE,     NULL,
         _("Soft Light"),    GIMP_SOFTLIGHT_MODE,     NULL,
         _("Grain Extract"), GIMP_GRAIN_EXTRACT_MODE, NULL,
         _("Grain Merge"),   GIMP_GRAIN_MERGE_MODE,   NULL,
	 "---",              0,                       NULL,
         _("Difference"),    GIMP_DIFFERENCE_MODE,    NULL,
         _("Addition"),      GIMP_ADDITION_MODE,      NULL,
         _("Subtract"),      GIMP_SUBTRACT_MODE,      NULL,
         _("Darken Only"),   GIMP_DARKEN_ONLY_MODE,   NULL,
         _("Lighten Only"),  GIMP_LIGHTEN_ONLY_MODE,  NULL,
	 "---",              0,                       NULL,
         _("Hue"),           GIMP_HUE_MODE,           NULL,
         _("Saturation"),    GIMP_SATURATION_MODE,    NULL,
         _("Color"),         GIMP_COLOR_MODE,         NULL,
         _("Value"),         GIMP_VALUE_MODE,         NULL,

         NULL);
    }
  else
    {
      menu = gimp_int_option_menu_new
        (FALSE, callback, data, initial,

         _("Normal"),        GIMP_NORMAL_MODE,        NULL,
         _("Dissolve"),      GIMP_DISSOLVE_MODE,      NULL,
	 "---",              0,                       NULL,
         _("Multiply"),      GIMP_MULTIPLY_MODE,      NULL,
         _("Divide"),        GIMP_DIVIDE_MODE,        NULL,
         _("Screen"),        GIMP_SCREEN_MODE,        NULL,
         _("Overlay"),       GIMP_OVERLAY_MODE,       NULL,
	 "---",              0,                       NULL,
         _("Dodge"),         GIMP_DODGE_MODE,         NULL,
         _("Burn"),          GIMP_BURN_MODE,          NULL,
         _("Hard Light"),    GIMP_HARDLIGHT_MODE,     NULL,
         _("Soft Light"),    GIMP_SOFTLIGHT_MODE,     NULL,
         _("Grain Extract"), GIMP_GRAIN_EXTRACT_MODE, NULL,
         _("Grain Merge"),   GIMP_GRAIN_MERGE_MODE,   NULL,
	 "---",              0,                       NULL,
         _("Difference"),    GIMP_DIFFERENCE_MODE,    NULL,
         _("Addition"),      GIMP_ADDITION_MODE,      NULL,
         _("Subtract"),      GIMP_SUBTRACT_MODE,      NULL,
         _("Darken Only"),   GIMP_DARKEN_ONLY_MODE,   NULL,
         _("Lighten Only"),  GIMP_LIGHTEN_ONLY_MODE,  NULL,
	 "---",              0,                       NULL,
         _("Hue"),           GIMP_HUE_MODE,           NULL,
         _("Saturation"),    GIMP_SATURATION_MODE,    NULL,
         _("Color"),         GIMP_COLOR_MODE,         NULL,
         _("Value"),         GIMP_VALUE_MODE,         NULL,

         NULL);
    }

  return menu;
}
