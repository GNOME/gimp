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
#ifndef __MODREGISTER_H__
#define __MODREGISTER_H__


#ifdef __EMX__

#include <libgimp/gimpcolordisplay.h>
#include <libgimp/gimpcolorselector.h>

struct main_funcs_struc {
  gchar *name;
  void (*func)();
};

typedef GimpColorSelectorID (*color_reg_func)(const char *,
					      const char *,
					      GimpColorSelectorMethods *);
typedef G_MODULE_EXPORT gboolean (*display_reg_func)
                 (const char *,GimpColorDisplayMethods *);

typedef gboolean (*color_unreg_func) (GimpColorSelectorID,
				      void (*)(void *),
				      void *);
typedef G_MODULE_EXPORT gboolean (*display_unreg_func) (const char *name);

typedef void (*dialog_reg_func) (GtkWidget *dialog);

void mod_dialog_register (GtkWidget *dialog);
void mod_dialog_unregister (GtkWidget *dialog);
#define dialog_register mod_dialog_register 
#define dialog_unregister mod_dialog_unregister 

GimpColorSelectorID
mod_color_selector_register (const char *name,
			     const char *help_page,
			     GimpColorSelectorMethods *methods);
gboolean
mod_color_selector_unregister (GimpColorSelectorID id,
			       void (*callback)(void *data),
			       void *data);

G_MODULE_EXPORT gboolean
mod_color_display_register (const char              *name,
    			     GimpColorDisplayMethods *methods);

G_MODULE_EXPORT gboolean
mod_color_display_unregister (const char *name);

#endif


#endif   /*  __MODREGISTER_H__  */
