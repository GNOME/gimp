/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#pragma once

#include "gimpdock.h"


#define GIMP_TYPE_TOOLBOX            (gimp_toolbox_get_type ())
#define GIMP_TOOLBOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TOOLBOX, GimpToolbox))
#define GIMP_TOOLBOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TOOLBOX, GimpToolboxClass))
#define GIMP_IS_TOOLBOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TOOLBOX))
#define GIMP_IS_TOOLBOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TOOLBOX))
#define GIMP_TOOLBOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TOOLBOX, GimpToolboxClass))


typedef struct _GimpToolboxClass   GimpToolboxClass;
typedef struct _GimpToolboxPrivate GimpToolboxPrivate;

struct _GimpToolbox
{
  GimpDock parent_instance;

  GimpToolboxPrivate *p;
};

struct _GimpToolboxClass
{
  GimpDockClass parent_class;
};


GType               gimp_toolbox_get_type           (void) G_GNUC_CONST;
GtkWidget         * gimp_toolbox_new                (GimpDialogFactory *factory,
                                                     GimpContext       *context,
                                                     GimpUIManager     *ui_manager);
GimpContext       * gimp_toolbox_get_context        (GimpToolbox       *toolbox);
void                gimp_toolbox_set_drag_handler   (GimpToolbox       *toolbox,
                                                     GimpPanedBox      *drag_handler);
