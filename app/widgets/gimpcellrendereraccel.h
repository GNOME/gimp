/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcellrendereraccel.h
 * Copyright (C) 2004  Michael Natterer <mitch@gimp.org>
 *
 * Derived from: libegg/libegg/treeviewutils/eggcellrendererkeys.h
 * Copyright (C) 2000  Red Hat, Inc.,  Jonathan Blandford <jrb@redhat.com>
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

#ifndef __GIMP_CELL_RENDERER_ACCEL_H__
#define __GIMP_CELL_RENDERER_ACCEL_H__


#include <gtk/gtkcellrenderertext.h>


#define GIMP_TYPE_CELL_RENDERER_ACCEL            (gimp_cell_renderer_accel_get_type ())
#define GIMP_CELL_RENDERER_ACCEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CELL_RENDERER_ACCEL, GimpCellRendererAccel))
#define GIMP_CELL_RENDERER_ACCEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CELL_RENDERER_ACCEL, GimpCellRendererAccelClass))
#define GIMP_IS_CELL_RENDERER_ACCEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CELL_RENDERER_ACCEL))
#define GIMP_IS_CELL_RENDERER_ACCEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CELL_RENDERER_ACCEL))
#define GIMP_CELL_RENDERER_ACCEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CELL_RENDERER_ACCEL, GimpCellRendererAccelClass))


typedef struct _GimpCellRendererAccelClass GimpCellRendererAccelClass;

struct _GimpCellRendererAccel
{
  GtkCellRendererText  parent_instance;

  guint                accel_key;
  GdkModifierType      accel_mask;

  GtkWidget           *edit_widget;
  GtkWidget           *grab_widget;
  guint                edit_key;

  GtkWidget           *sizing_label;
};

struct _GimpCellRendererAccelClass
{
  GtkCellRendererTextClass  parent_class;

  void (* accel_edited) (GimpCellRendererAccel *accel,
                         const char            *path_string,
                         gboolean               delete,
                         guint                  accel_key,
                         GdkModifierType        accel_mask);
};


GType             gimp_cell_renderer_accel_get_type (void) G_GNUC_CONST;

GtkCellRenderer * gimp_cell_renderer_accel_new      (void);

void gimp_cell_renderer_accel_set_accelerator (GimpCellRendererAccel *accel,
                                               guint                  accel_kaey,
                                               GdkModifierType        accel_mask);
void gimp_cell_renderer_accel_get_accelerator (GimpCellRendererAccel *accel,
                                               guint                 *accel_key,
                                               GdkModifierType       *accel_mask);


#endif /* __GIMP_CELL_RENDERER_ACCEL_H__ */
