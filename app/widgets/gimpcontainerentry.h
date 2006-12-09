/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainerentry.h
 * Copyright (C) 2004  Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_CONTAINER_ENTRY_H__
#define __GIMP_CONTAINER_ENTRY_H__


#include <gtk/gtkentry.h>


enum
{
  GIMP_CONTAINER_ENTRY_COLUMN_RENDERER,
  GIMP_CONTAINER_ENTRY_COLUMN_NAME,
  GIMP_CONTAINER_ENTRY_NUM_COLUMNS
};


#define GIMP_TYPE_CONTAINER_ENTRY            (gimp_container_entry_get_type ())
#define GIMP_CONTAINER_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CONTAINER_ENTRY, GimpContainerEntry))
#define GIMP_CONTAINER_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CONTAINER_ENTRY, GimpContainerEntryClass))
#define GIMP_IS_CONTAINER_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CONTAINER_ENTRY))
#define GIMP_IS_CONTAINER_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CONTAINER_ENTRY))
#define GIMP_CONTAINER_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CONTAINER_ENTRY, GimpContainerEntryClass))


typedef struct _GimpContainerEntryClass  GimpContainerEntryClass;

struct _GimpContainerEntry
{
  GtkEntry        parent_instance;
};

struct _GimpContainerEntryClass
{
  GtkEntryClass   parent_class;
};


GType       gimp_container_entry_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_container_entry_new      (GimpContainer *container,
                                           GimpContext   *context,
                                           gint           view_size,
                                           gint           view_border_width);


#endif  /*  __GIMP_CONTAINER_ENTRY_H__  */
