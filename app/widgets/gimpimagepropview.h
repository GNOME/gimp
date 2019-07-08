/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpImagePropView
 * Copyright (C) 2005  Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_IMAGE_PROP_VIEW_H__
#define __GIMP_IMAGE_PROP_VIEW_H__


#define GIMP_TYPE_IMAGE_PROP_VIEW            (gimp_image_prop_view_get_type ())
#define GIMP_IMAGE_PROP_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_IMAGE_PROP_VIEW, GimpImagePropView))
#define GIMP_IMAGE_PROP_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_IMAGE_PROP_VIEW, GimpImagePropViewClass))
#define GIMP_IS_IMAGE_PROP_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_IMAGE_PROP_VIEW))
#define GIMP_IS_IMAGE_PROP_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_IMAGE_PROP_VIEW))
#define GIMP_IMAGE_PROP_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_IMAGE_PROP_VIEW, GimpImagePropViewClass))


typedef struct _GimpImagePropViewClass GimpImagePropViewClass;

struct _GimpImagePropView
{
  GtkGrid    parent_instance;

  GimpImage *image;

  GtkWidget *pixel_size_label;
  GtkWidget *print_size_label;
  GtkWidget *resolution_label;
  GtkWidget *colorspace_label;
  GtkWidget *precision_label;
  GtkWidget *filename_label;
  GtkWidget *filesize_label;
  GtkWidget *filetype_label;
  GtkWidget *memsize_label;
  GtkWidget *undo_label;
  GtkWidget *redo_label;
  GtkWidget *pixels_label;
  GtkWidget *layers_label;
  GtkWidget *channels_label;
  GtkWidget *vectors_label;
};

struct _GimpImagePropViewClass
{
  GtkGridClass  parent_class;
};


GType       gimp_image_prop_view_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_image_prop_view_new      (GimpImage *image);


#endif /*  __GIMP_IMAGE_PROP_VIEW_H__  */
