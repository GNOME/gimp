/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainerbox.h
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

#ifndef __GIMP_CONTAINER_BOX_H__
#define __GIMP_CONTAINER_BOX_H__


#include "gimpeditor.h"


#define GIMP_TYPE_CONTAINER_BOX            (gimp_container_box_get_type ())
#define GIMP_CONTAINER_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CONTAINER_BOX, GimpContainerBox))
#define GIMP_CONTAINER_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CONTAINER_BOX, GimpContainerBoxClass))
#define GIMP_IS_CONTAINER_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CONTAINER_BOX))
#define GIMP_IS_CONTAINER_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CONTAINER_BOX))
#define GIMP_CONTAINER_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CONTAINER_BOX, GimpContainerBoxClass))


typedef struct _GimpContainerBoxClass  GimpContainerBoxClass;

struct _GimpContainerBox
{
  GimpEditor  parent_instance;

  GtkWidget  *scrolled_win;
};

struct _GimpContainerBoxClass
{
  GimpEditorClass  parent_class;
};


GType     gimp_container_box_get_type         (void) G_GNUC_CONST;

void      gimp_container_box_set_size_request (GimpContainerBox *box,
                                               gint              width,
                                               gint              height);


#endif  /*  __GIMP_CONTAINER_BOX_H__  */
