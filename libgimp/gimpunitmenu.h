/* LIBGIMP - The GIMP Library                                                   
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
 *
 * gimpunitmenu.h
 * Copyright (C) 1999 Michael Natterer <mitschel@cs.tu-berlin.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.             
 *                                                                              
 * This library is distributed in the hope that it will be useful,              
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU            
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __GIMP_UNIT_MENU_H__
#define __GIMP_UNIT_MENU_H__

#include <gtk/gtk.h>

#include "libgimp/gimpunit.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GIMP_TYPE_UNIT_MENU            (gimp_unit_menu_get_type ())
#define GIMP_UNIT_MENU(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_UNIT_MENU, GimpUnitMenu))
#define GIMP_UNIT_MENU_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_UNIT_MENU, GimpUnitMenuClass))
#define GIMP_IS_UNIT_MENU(obj)         (GTK_CHECK_TYPE (obj, GIMP_TYPE_UNIT_MENU))
#define GIMP_IS_UNIT_MENU_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_UNIT_MENU))

typedef struct _GimpUnitMenu       GimpUnitMenu;
typedef struct _GimpUnitMenuClass  GimpUnitMenuClass;

struct _GimpUnitMenu
{
  GtkOptionMenu  optionmenu;

  /* private */
  GtkWidget     *selection;
  GtkWidget     *clist;

  /* public */
  gchar         *format;
  GUnit          unit;

  gboolean       show_pixels;
  gboolean       show_percent;
};

struct _GimpUnitMenuClass
{
  GtkOptionMenuClass parent_class;

  void (* unit_changed) (GimpUnitMenu *gum);
};

GtkType gimp_unit_menu_get_type (void);

/*  format       -- a printf-like format string for the menu items
 *  unit         -- the unit selected on widget creation
 *  show_pixels  -- should the menu contain 'pixels' ?
 *  show_percent -- should the menu contain 'percent' ?
 *  show_custom  -- should the menu contain an item 'More...' to pop up
 *                  the custom unit browser (not yet implemented)
 *
 *            the format string supports the following percent expansions:
 *
 *            %f -- factor (how many units make up an inch)
 *            %y -- symbol ("''" for inch)
 *            %a -- abbreviation
 *            %s -- singular
 *            %p -- plural
 *            %% -- literal percent
 */
GtkWidget * gimp_unit_menu_new      (gchar       *format,
				     GUnit        unit,
				     gboolean     show_pixels,
				     gboolean     show_percent,
				     gboolean     show_custom);

void        gimp_unit_menu_set_unit (GimpUnitMenu *gum, 
				     GUnit         unit);

GUnit       gimp_unit_menu_get_unit (GimpUnitMenu *gum);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GIMP_UNIT_MENU_H__ */
