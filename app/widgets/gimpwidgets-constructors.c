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
gimp_paint_mode_menu_new (GtkSignalFunc    callback,
			  gpointer         data,
			  gboolean         with_behind_mode,
			  LayerModeEffects initial)
{
  GtkWidget *menu;

  if (with_behind_mode)
    {
      menu = gimp_option_menu_new2
        (FALSE, callback, data, (gpointer) initial,

         _("Normal"),       (gpointer) NORMAL_MODE,       NULL,
         _("Dissolve"),     (gpointer) DISSOLVE_MODE,     NULL,
         _("Behind"),       (gpointer) BEHIND_MODE,       NULL,
         _("Color Erase"),  (gpointer) COLOR_ERASE_MODE,  NULL,
	 "---",             (gpointer) NULL,              NULL,
         _("Multiply"),     (gpointer) MULTIPLY_MODE,     NULL,
         _("Divide"),       (gpointer) DIVIDE_MODE,       NULL,
         _("Screen"),       (gpointer) SCREEN_MODE,       NULL,
         _("Overlay"),      (gpointer) OVERLAY_MODE,      NULL,
	 "---",             (gpointer) NULL,              NULL,
         _("Dodge"),        (gpointer) DODGE_MODE,        NULL,
         _("Burn"),         (gpointer) BURN_MODE,         NULL,
         _("Hard Light"),   (gpointer) HARDLIGHT_MODE,    NULL,
	 "---",             (gpointer) NULL,              NULL,
         _("Difference"),   (gpointer) DIFFERENCE_MODE,   NULL,
         _("Addition"),     (gpointer) ADDITION_MODE,     NULL,
         _("Subtract"),     (gpointer) SUBTRACT_MODE,     NULL,
         _("Darken Only"),  (gpointer) DARKEN_ONLY_MODE,  NULL,
         _("Lighten Only"), (gpointer) LIGHTEN_ONLY_MODE, NULL,
	 "---",             (gpointer) NULL,              NULL,
         _("Hue"),          (gpointer) HUE_MODE,          NULL,
         _("Saturation"),   (gpointer) SATURATION_MODE,   NULL,
         _("Color"),        (gpointer) COLOR_MODE,        NULL,
         _("Value"),        (gpointer) VALUE_MODE,        NULL,

         NULL);
    }
  else
    {
      menu = gimp_option_menu_new2
        (FALSE, callback, data, (gpointer) initial,

         _("Normal"),       (gpointer) NORMAL_MODE,       NULL,
         _("Dissolve"),     (gpointer) DISSOLVE_MODE,     NULL,
	 "---",             (gpointer) NULL,              NULL,
         _("Multiply"),     (gpointer) MULTIPLY_MODE,     NULL,
         _("Divide"),       (gpointer) DIVIDE_MODE,       NULL,
         _("Screen"),       (gpointer) SCREEN_MODE,       NULL,
         _("Overlay"),      (gpointer) OVERLAY_MODE,      NULL,
	 "---",             (gpointer) NULL,              NULL,
         _("Dodge"),        (gpointer) DODGE_MODE,        NULL,
         _("Burn"),         (gpointer) BURN_MODE,         NULL,
         _("Hard Light"),   (gpointer) HARDLIGHT_MODE,    NULL,
	 "---",             (gpointer) NULL,              NULL,
         _("Difference"),   (gpointer) DIFFERENCE_MODE,   NULL,
         _("Addition"),     (gpointer) ADDITION_MODE,     NULL,
         _("Subtract"),     (gpointer) SUBTRACT_MODE,     NULL,
         _("Darken Only"),  (gpointer) DARKEN_ONLY_MODE,  NULL,
         _("Lighten Only"), (gpointer) LIGHTEN_ONLY_MODE, NULL,
	 "---",             (gpointer) NULL,              NULL,
         _("Hue"),          (gpointer) HUE_MODE,          NULL,
         _("Saturation"),   (gpointer) SATURATION_MODE,   NULL,
         _("Color"),        (gpointer) COLOR_MODE,        NULL,
         _("Value"),        (gpointer) VALUE_MODE,        NULL,

         NULL);
    }

  return menu;
}
