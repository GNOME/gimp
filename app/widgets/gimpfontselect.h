/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpfontselect.h
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_FONT_SELECT_H__
#define __GIMP_FONT_SELECT_H__

#include "gimppdbdialog.h"

G_BEGIN_DECLS


#define GIMP_TYPE_FONT_SELECT            (gimp_font_select_get_type ())
#define GIMP_FONT_SELECT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_FONT_SELECT, GimpFontSelect))
#define GIMP_FONT_SELECT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_FONT_SELECT, GimpFontSelectClass))
#define GIMP_IS_FONT_SELECT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_FONT_SELECT))
#define GIMP_IS_FONT_SELECT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_FONT_SELECT))
#define GIMP_FONT_SELECT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_FONT_SELECT, GimpFontSelectClass))


typedef struct _GimpFontSelectClass  GimpFontSelectClass;

struct _GimpFontSelect
{
  GimpPdbDialog  parent_instance;
};

struct _GimpFontSelectClass
{
  GimpPdbDialogClass  parent_class;
};


GType  gimp_font_select_get_type (void) G_GNUC_CONST;


G_END_DECLS

#endif /* __GIMP_FONT_SELECT_H__ */
