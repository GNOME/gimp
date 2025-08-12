/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpImageProfileView
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

#pragma once

#include "gimpimageparasiteview.h"


#define GIMP_TYPE_IMAGE_PROFILE_VIEW            (gimp_image_profile_view_get_type ())
#define GIMP_IMAGE_PROFILE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_IMAGE_PROFILE_VIEW, GimpImageProfileView))
#define GIMP_IMAGE_PROFILE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_IMAGE_PROFILE_VIEW, GimpImageProfileViewClass))
#define GIMP_IS_IMAGE_PROFILE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_IMAGE_PROFILE_VIEW))
#define GIMP_IS_IMAGE_PROFILE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_IMAGE_PROFILE_VIEW))
#define GIMP_IMAGE_PROFILE_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_IMAGE_PROFILE_VIEW, GimpImageProfileViewClass))


typedef struct _GimpImageProfileViewClass GimpImageProfileViewClass;

struct _GimpImageProfileView
{
  GimpImageParasiteView  parent_instance;

  GimpColorProfileView  *profile_view;
};

struct _GimpImageProfileViewClass
{
  GimpImageParasiteViewClass  parent_class;
};


GType       gimp_image_profile_view_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_image_profile_view_new      (GimpImage *image);
