/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpprogressbox.h
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_PROGRESS_BOX_H__
#define __GIMP_PROGRESS_BOX_H__

#include <gtk/gtkvbox.h>

G_BEGIN_DECLS


#define GIMP_TYPE_PROGRESS_BOX            (gimp_progress_box_get_type ())
#define GIMP_PROGRESS_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PROGRESS_BOX, GimpProgressBox))
#define GIMP_PROGRESS_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PROGRESS_BOX, GimpProgressBoxClass))
#define GIMP_IS_PROGRESS_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PROGRESS_BOX))
#define GIMP_IS_PROGRESS_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PROGRESS_BOX))
#define GIMP_PROGRESS_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PROGRESS_BOX, GimpProgressBoxClass))


typedef struct _GimpProgressBoxClass  GimpProgressBoxClass;

struct _GimpProgressBox
{
  GtkVBox     parent_instance;

  gboolean    active;
  gboolean    cancelable;

  GtkWidget  *label;
  GtkWidget  *progress;
};

struct _GimpProgressBoxClass
{
  GtkVBoxClass  parent_class;
};


GType       gimp_progress_box_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_progress_box_new      (void);


G_END_DECLS

#endif /* __GIMP_PROGRESS_BOX_H__ */
