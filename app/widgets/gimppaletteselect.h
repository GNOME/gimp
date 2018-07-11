/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppaletteselect.h
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

#ifndef __GIMP_PALETTE_SELECT_H__
#define __GIMP_PALETTE_SELECT_H__

#include "gimppdbdialog.h"

G_BEGIN_DECLS


#define GIMP_TYPE_PALETTE_SELECT            (gimp_palette_select_get_type ())
#define GIMP_PALETTE_SELECT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PALETTE_SELECT, GimpPaletteSelect))
#define GIMP_PALETTE_SELECT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PALETTE_SELECT, GimpPaletteSelectClass))
#define GIMP_IS_PALETTE_SELECT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PALETTE_SELECT))
#define GIMP_IS_PALETTE_SELECT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PALETTE_SELECT))
#define GIMP_PALETTE_SELECT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PALETTE_SELECT, GimpPaletteSelectClass))


typedef struct _GimpPaletteSelectClass  GimpPaletteSelectClass;

struct _GimpPaletteSelect
{
  GimpPdbDialog  parent_instance;
};

struct _GimpPaletteSelectClass
{
  GimpPdbDialogClass  parent_class;
};


GType  gimp_palette_select_get_type (void) G_GNUC_CONST;


G_END_DECLS

#endif /* __GIMP_PALETTE_SELECT_H__ */
