/*
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
 *
 *
 */
#ifdef __EMX__

#include "config.h"

#include <stdio.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <libgimp/color_selector.h>
#include <libgimp/color_display.h>
#include <libgimp/gimpmodule.h>
#include <math.h>
#include "gimpmodregister.h"

struct main_funcs_struc *gimp_main_funcs = NULL;

gpointer get_main_func(gchar *name)
{
  struct main_funcs_struc *x;
  if (gimp_main_funcs == NULL)
    return NULL;
  for (x = gimp_main_funcs; x->name; x++)
  {
    if (!strcmp(x->name, name))
      return (gpointer) x->func;
  }
}

GimpColorSelectorID
mod_color_selector_register (const char *name,
			     const char *help_page,
			     GimpColorSelectorMethods *methods)
{
    GimpColorSelectorID id;
    color_reg_func reg_func;
    
    reg_func = (color_reg_func) get_main_func("gimp_color_selector_register");
    if (!reg_func)
	return 0;
    id = (*reg_func) (name, help_page, methods);
    return (id);
}

G_MODULE_EXPORT gboolean
mod_color_display_register (const char              *name,
    			     GimpColorDisplayMethods *methods)
{
    gboolean  retval;
    display_reg_func reg_func;
    
    reg_func = (display_reg_func) get_main_func("gimp_color_display_register");
    if (!reg_func)
	return 0;
    retval = (*reg_func) (name, methods);
    return (retval);

}

gboolean
mod_color_selector_unregister (GimpColorSelectorID id,
			       void (*callback)(void *data),
			       void *data)
{
    color_unreg_func unreg_func;
    gboolean status;
    
    unreg_func = (color_unreg_func) get_main_func("gimp_color_selector_unregister");
    if (unreg_func)
    {
	status = (*unreg_func) (id, callback, data);
    }
    else
	status = FALSE;
    return (status);
}

G_MODULE_EXPORT gboolean
mod_color_display_unregister (const char *name)
{
    display_unreg_func unreg_func;
    gboolean status;
    
    unreg_func = (display_unreg_func) get_main_func("gimp_color_display_unregister");
    if (unreg_func)
    {
	status = (*unreg_func) (name);
    }
    else
	status = FALSE;
    return (status);
}

void mod_dialog_register (GtkWidget *dialog)
{
    dialog_reg_func reg_func;
    
    reg_func = (dialog_reg_func) get_main_func("dialog_register");
    if (!reg_func)
	return;
    (*reg_func) (dialog);
}

void mod_dialog_unregister (GtkWidget *dialog)
{
    dialog_reg_func reg_func;
    
    reg_func = (dialog_reg_func) get_main_func("dialog_unregister");
    if (!reg_func)
	return;
    (*reg_func) (dialog);
}

#endif
