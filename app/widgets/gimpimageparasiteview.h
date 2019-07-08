/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpImageParasiteView
 * Copyright (C) 2006  Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_IMAGE_PARASITE_VIEW_H__
#define __GIMP_IMAGE_PARASITE_VIEW_H__


#define GIMP_TYPE_IMAGE_PARASITE_VIEW            (gimp_image_parasite_view_get_type ())
#define GIMP_IMAGE_PARASITE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_IMAGE_PARASITE_VIEW, GimpImageParasiteView))
#define GIMP_IMAGE_PARASITE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_IMAGE_PARASITE_VIEW, GimpImageParasiteViewClass))
#define GIMP_IS_IMAGE_PARASITE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_IMAGE_PARASITE_VIEW))
#define GIMP_IS_IMAGE_PARASITE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_IMAGE_PARASITE_VIEW))
#define GIMP_IMAGE_PARASITE_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_IMAGE_PARASITE_VIEW, GimpImageParasiteViewClass))


typedef struct _GimpImageParasiteViewClass GimpImageParasiteViewClass;

struct _GimpImageParasiteView
{
  GtkBox     parent_instance;

  GimpImage *image;
  gchar     *parasite;
};

struct _GimpImageParasiteViewClass
{
  GtkBoxClass  parent_class;

  /*  signals  */
  void (* update) (GimpImageParasiteView *view);
};


GType                gimp_image_parasite_view_get_type     (void) G_GNUC_CONST;

GtkWidget          * gimp_image_parasite_view_new          (GimpImage   *image,
                                                            const gchar *parasite);
GimpImage          * gimp_image_parasite_view_get_image    (GimpImageParasiteView *view);
const GimpParasite * gimp_image_parasite_view_get_parasite (GimpImageParasiteView *view);


#endif /*  __GIMP_IMAGE_PARASITE_VIEW_H__  */
