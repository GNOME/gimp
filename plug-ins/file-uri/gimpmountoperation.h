/* GTK - The GIMP Toolkit
 * Copyright (C) Christian Kellner <gicmo@gnome.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __GIMP_MOUNT_OPERATION_H__
#define __GIMP_MOUNT_OPERATION_H__

#include <gio/gio.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GIMP_TYPE_MOUNT_OPERATION         (gimp_mount_operation_get_type ())
#define GIMP_MOUNT_OPERATION(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GIMP_TYPE_MOUNT_OPERATION, GimpMountOperation))
#define GIMP_MOUNT_OPERATION_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GIMP_TYPE_MOUNT_OPERATION, GimpMountOperationClass))
#define GIMP_IS_MOUNT_OPERATION(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GIMP_TYPE_MOUNT_OPERATION))
#define GIMP_IS_MOUNT_OPERATION_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GIMP_TYPE_MOUNT_OPERATION))
#define GIMP_MOUNT_OPERATION_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GIMP_TYPE_MOUNT_OPERATION, GimpMountOperationClass))

typedef struct GimpMountOperation         GimpMountOperation;
typedef struct GimpMountOperationClass    GimpMountOperationClass;
typedef struct GimpMountOperationPrivate  GimpMountOperationPrivate;

struct GimpMountOperation
{
  GMountOperation parent_instance;

  GimpMountOperationPrivate *priv;
};

struct GimpMountOperationClass
{
  GMountOperationClass parent_class;

  /* Padding for future expansion */
  void (*_gtk_reserved1) (void);
  void (*_gtk_reserved2) (void);
  void (*_gtk_reserved3) (void);
  void (*_gtk_reserved4) (void);
};


GType            gimp_mount_operation_get_type   (void);
GMountOperation *gimp_mount_operation_new        (GtkWindow          *parent);
gboolean         gimp_mount_operation_is_showing (GimpMountOperation *op);
void             gimp_mount_operation_set_parent (GimpMountOperation *op,
                                                  GtkWindow          *parent);
GtkWindow *      gimp_mount_operation_get_parent (GimpMountOperation *op);
void             gimp_mount_operation_set_screen (GimpMountOperation *op,
                                                  GdkScreen          *screen);
GdkScreen       *gimp_mount_operation_get_screen (GimpMountOperation *op);

G_END_DECLS

#endif /* __GIMP_MOUNT_OPERATION_H__ */
