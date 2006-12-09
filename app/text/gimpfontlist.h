/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpfontlist.h
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
 *                    Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_FONT_LIST_H__
#define __GIMP_FONT_LIST_H__


#include "core/gimplist.h"


#define GIMP_TYPE_FONT_LIST            (gimp_font_list_get_type ())
#define GIMP_FONT_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_FONT_LIST, GimpFontList))
#define GIMP_FONT_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_FONT_LIST, GimpFontListClass))
#define GIMP_IS_FONT_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_FONT_LIST))
#define GIMP_IS_FONT_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_FONT_LIST))
#define GIMP_FONT_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_FONT_LIST, GimpFontListClass))


typedef struct _GimpFontListClass GimpFontListClass;

struct _GimpFontList
{
  GimpList  parent_instance;

  gdouble   xresolution;
  gdouble   yresolution;
};

struct _GimpFontListClass
{
  GimpListClass  parent_class;
};


GType           gimp_font_list_get_type (void) G_GNUC_CONST;

GimpContainer * gimp_font_list_new      (gdouble       xresolution,
                                         gdouble       yresolution);
void            gimp_font_list_restore  (GimpFontList *list);


#endif  /*  __GIMP_FONT_LIST_H__  */
