/* LIBGIMP - The GIMP Library 
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * Colour selector module (C) 1999 Austin Donnelly <austin@greenend.org.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __COLOR_SELECTOR_H__
#define __COLOR_SELECTOR_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* For information look at the html documentation */


typedef void        (* GimpColorSelectorCallback) (gpointer   data,
						   gint       r,
						   gint       g,
						   gint       b,
						   gint       a);

typedef GtkWidget * (* GimpColorSelectorNewFunc)  (gint       r,
						   gint       g,
						   gint       b,
						   gint       a,
						   gboolean   show_alpha,
						   GimpColorSelectorCallback cb,
						   gpointer   data,
						   gpointer  *selector_data);

typedef void        (* GimpColorSelectorFreeFunc) (gpointer   selector_data);


typedef void    (* GimpColorSelectorSetColorFunc) (gpointer   selector_data,
						   gint       r,
						   gint       g,
						   gint       b,
						   gint       a,
						   gboolean   set_current);

typedef void      (* GimpColorSelectorFinishedCB) (gpointer   finished_data);


typedef struct _GimpColorSelectorMethods GimpColorSelectorMethods;

struct _GimpColorSelectorMethods
{
  GimpColorSelectorNewFunc      new;
  GimpColorSelectorFreeFunc     free;
  GimpColorSelectorSetColorFunc setcolor;
};

typedef gpointer GimpColorSelectorID;


#ifndef __COLOR_NOTEBOOK_C__

/*  Bypass when compiling the source for these functions.
 */
GimpColorSelectorID
gimp_color_selector_register   (const gchar                 *name,
				const gchar                 *help_page,
				GimpColorSelectorMethods    *methods);

gboolean
gimp_color_selector_unregister (GimpColorSelectorID          id,
				GimpColorSelectorFinishedCB  finished_cb,
				gpointer                     finished_data);

#endif /* !__COLOR_NOTEBOOK_C__ */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __COLOR_SELECTOR_H__ */
