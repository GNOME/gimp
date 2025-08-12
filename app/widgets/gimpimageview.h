/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpimageview.h
 * Copyright (C) 2002 Michael Natterer <mitch@gimp.org>
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

#include "gimpcontainereditor.h"


#define GIMP_TYPE_IMAGE_VIEW            (gimp_image_view_get_type ())
#define GIMP_IMAGE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_IMAGE_VIEW, GimpImageView))
#define GIMP_IMAGE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_IMAGE_VIEW, GimpImageViewClass))
#define GIMP_IS_IMAGE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_IMAGE_VIEW))
#define GIMP_IS_IMAGE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_IMAGE_VIEW))
#define GIMP_IMAGE_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_IMAGE_VIEW, GimpImageViewClass))


typedef struct _GimpImageViewClass GimpImageViewClass;

struct _GimpImageView
{
  GimpContainerEditor  parent_instance;

  GtkWidget           *raise_button;
  GtkWidget           *new_button;
  GtkWidget           *delete_button;
};

struct _GimpImageViewClass
{
  GimpContainerEditorClass  parent_class;
};


GType       gimp_image_view_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_image_view_new      (GimpViewType     view_type,
                                      GimpContainer   *container,
                                      GimpContext     *context,
                                      gint             view_size,
                                      gint             view_border_width,
                                      GimpMenuFactory *menu_factory);
