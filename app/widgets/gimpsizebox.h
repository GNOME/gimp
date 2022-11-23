/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmasizebox.h
 * Copyright (C) 2004 Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_SIZE_BOX_H__
#define __LIGMA_SIZE_BOX_H__

G_BEGIN_DECLS


#define LIGMA_TYPE_SIZE_BOX            (ligma_size_box_get_type ())
#define LIGMA_SIZE_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_SIZE_BOX, LigmaSizeBox))
#define LIGMA_SIZE_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_SIZE_BOX, LigmaSizeBoxClass))
#define LIGMA_IS_SIZE_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_SIZE_BOX))
#define LIGMA_IS_SIZE_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_SIZE_BOX))
#define LIGMA_SIZE_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_SIZE_BOX, LigmaSizeBoxClass))


typedef struct _LigmaSizeBoxClass  LigmaSizeBoxClass;

struct _LigmaSizeBox
{
  GtkBox        parent_instance;

  GtkSizeGroup *size_group;

  gint          width;
  gint          height;
  LigmaUnit      unit;
  gdouble       xresolution;
  gdouble       yresolution;
  LigmaUnit      resolution_unit;

  gboolean      edit_resolution;
};

struct _LigmaSizeBoxClass
{
  GtkBoxClass  parent_class;
};


GType       ligma_size_box_get_type (void) G_GNUC_CONST;


G_END_DECLS

#endif /* __LIGMA_SIZE_BOX_H__ */
