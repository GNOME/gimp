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

#include "libgimp/gimpintl.h"


GtkWidget *
gimp_paint_mode_menu_new (GtkSignalFunc        callback,
			  gpointer             data,
			  gboolean             with_behind_mode,
			  GimpLayerModeEffects initial)
{
  GtkWidget *menu;

  if (with_behind_mode)
    {
      menu = gimp_option_menu_new2
        (FALSE, callback, data, (gpointer) initial,

         _("Normal"),       (gpointer) GIMP_NORMAL_MODE,       NULL,
         _("Dissolve"),     (gpointer) GIMP_DISSOLVE_MODE,     NULL,
         _("Behind"),       (gpointer) GIMP_BEHIND_MODE,       NULL,
         _("Color Erase"),  (gpointer) GIMP_COLOR_ERASE_MODE,  NULL,
	 "---",             (gpointer) NULL,              NULL,
         _("Multiply"),     (gpointer) GIMP_MULTIPLY_MODE,     NULL,
         _("Divide"),       (gpointer) GIMP_DIVIDE_MODE,       NULL,
         _("Screen"),       (gpointer) GIMP_SCREEN_MODE,       NULL,
         _("Overlay"),      (gpointer) GIMP_OVERLAY_MODE,      NULL,
	 "---",             (gpointer) NULL,              NULL,
         _("Dodge"),        (gpointer) GIMP_DODGE_MODE,        NULL,
         _("Burn"),         (gpointer) GIMP_BURN_MODE,         NULL,
         _("Hard Light"),   (gpointer) GIMP_HARDLIGHT_MODE,    NULL,
	 "---",             (gpointer) NULL,              NULL,
         _("Difference"),   (gpointer) GIMP_DIFFERENCE_MODE,   NULL,
         _("Addition"),     (gpointer) GIMP_ADDITION_MODE,     NULL,
         _("Subtract"),     (gpointer) GIMP_SUBTRACT_MODE,     NULL,
         _("Darken Only"),  (gpointer) GIMP_DARKEN_ONLY_MODE,  NULL,
         _("Lighten Only"), (gpointer) GIMP_LIGHTEN_ONLY_MODE, NULL,
	 "---",             (gpointer) NULL,              NULL,
         _("Hue"),          (gpointer) GIMP_HUE_MODE,          NULL,
         _("Saturation"),   (gpointer) GIMP_SATURATION_MODE,   NULL,
         _("Color"),        (gpointer) GIMP_COLOR_MODE,        NULL,
         _("Value"),        (gpointer) GIMP_VALUE_MODE,        NULL,

         NULL);
    }
  else
    {
      menu = gimp_option_menu_new2
        (FALSE, callback, data, (gpointer) initial,

         _("Normal"),       (gpointer) GIMP_NORMAL_MODE,       NULL,
         _("Dissolve"),     (gpointer) GIMP_DISSOLVE_MODE,     NULL,
	 "---",             (gpointer) NULL,              NULL,
         _("Multiply"),     (gpointer) GIMP_MULTIPLY_MODE,     NULL,
         _("Divide"),       (gpointer) GIMP_DIVIDE_MODE,       NULL,
         _("Screen"),       (gpointer) GIMP_SCREEN_MODE,       NULL,
         _("Overlay"),      (gpointer) GIMP_OVERLAY_MODE,      NULL,
	 "---",             (gpointer) NULL,              NULL,
         _("Dodge"),        (gpointer) GIMP_DODGE_MODE,        NULL,
         _("Burn"),         (gpointer) GIMP_BURN_MODE,         NULL,
         _("Hard Light"),   (gpointer) GIMP_HARDLIGHT_MODE,    NULL,
	 "---",             (gpointer) NULL,              NULL,
         _("Difference"),   (gpointer) GIMP_DIFFERENCE_MODE,   NULL,
         _("Addition"),     (gpointer) GIMP_ADDITION_MODE,     NULL,
         _("Subtract"),     (gpointer) GIMP_SUBTRACT_MODE,     NULL,
         _("Darken Only"),  (gpointer) GIMP_DARKEN_ONLY_MODE,  NULL,
         _("Lighten Only"), (gpointer) GIMP_LIGHTEN_ONLY_MODE, NULL,
	 "---",             (gpointer) NULL,              NULL,
         _("Hue"),          (gpointer) GIMP_HUE_MODE,          NULL,
         _("Saturation"),   (gpointer) GIMP_SATURATION_MODE,   NULL,
         _("Color"),        (gpointer) GIMP_COLOR_MODE,        NULL,
         _("Value"),        (gpointer) GIMP_VALUE_MODE,        NULL,

         NULL);
    }

  return menu;
}
