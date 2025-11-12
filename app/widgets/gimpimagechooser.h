/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpimagechooser.h
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


#define GIMP_TYPE_IMAGE_CHOOSER            (gimp_image_chooser_get_type ())
#define GIMP_IMAGE_CHOOSER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_IMAGE_CHOOSER, GimpImageChooser))
#define GIMP_IMAGE_CHOOSER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_IMAGE_CHOOSER, GimpImageChooserClass))
#define GIMP_IS_IMAGE_CHOOSER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_IMAGE_CHOOSER))
#define GIMP_IS_IMAGE_CHOOSER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_IMAGE_CHOOSER))
#define GIMP_IMAGE_CHOOSER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_IMAGE_CHOOSER, GimpImageChooserClass))


typedef struct _GimpImageChooserPrivate GimpImageChooserPrivate;
typedef struct _GimpImageChooserClass   GimpImageChooserClass;

struct _GimpImageChooser
{
  GtkFrame                    parent_instance;

  GimpImageChooserPrivate *priv;
};

struct _GimpImageChooserClass
{
  GtkFrameClass               parent_instance;

  /* Signals. */

  void     (* activate)      (GimpImageChooser *view,
                              GimpImage        *image);
};


GType          gimp_image_chooser_get_type     (void) G_GNUC_CONST;

GtkWidget    * gimp_image_chooser_new          (GimpContext         *context,
                                                gint                 view_size,
                                                gint                 view_border_width);

GimpImage    * gimp_image_chooser_get_image    (GimpImageChooser    *chooser);
void           gimp_image_chooser_set_image    (GimpImageChooser    *chooser,
                                                GimpImage           *image);
