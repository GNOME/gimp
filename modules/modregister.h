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

struct main_funcs_struc {
  gchar *name;
  void (*func)();
};

typedef GimpColorSelectorID (*color_reg_func)(const char *,
					      const char *,
					      GimpColorSelectorMethods *);
typedef gboolean (*color_unreg_func) (GimpColorSelectorID,
				      void (*)(void *),
				      void *);
GimpColorSelectorID
mod_color_selector_register (const char *name,
			     const char *help_page,
			     GimpColorSelectorMethods *methods);
gboolean
mod_color_selector_unregister (GimpColorSelectorID id,
			       void (*callback)(void *data),
			       void *data);

#endif


#endif   /*  __MODREGISTER_H__  */
