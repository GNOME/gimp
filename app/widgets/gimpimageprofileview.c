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

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpconfig/gimpconfig.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpimage.h"
#include "core/gimpimage-color-profile.h"

#include "gimpimageprofileview.h"

#include "gimp-intl.h"


static void   gimp_image_profile_view_update (GimpImageParasiteView *view);


G_DEFINE_TYPE (GimpImageProfileView,
               gimp_image_profile_view, GIMP_TYPE_IMAGE_PARASITE_VIEW)

#define parent_class gimp_image_profile_view_parent_class


static void
gimp_image_profile_view_class_init (GimpImageProfileViewClass *klass)
{
  GimpImageParasiteViewClass *view_class;

  view_class = GIMP_IMAGE_PARASITE_VIEW_CLASS (klass);

  view_class->update = gimp_image_profile_view_update;
}

static void
gimp_image_profile_view_init (GimpImageProfileView *view)
{
  GtkWidget *scrolled_window;
  GtkWidget *profile_view;

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 2);
  gtk_box_pack_start (GTK_BOX (view), scrolled_window, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_window);

  profile_view = gimp_color_profile_view_new ();
  gtk_container_add (GTK_CONTAINER (scrolled_window), profile_view);
  gtk_widget_show (profile_view);

  view->profile_view = GIMP_COLOR_PROFILE_VIEW (profile_view);
}


/*  public functions  */

GtkWidget *
gimp_image_profile_view_new (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return g_object_new (GIMP_TYPE_IMAGE_PROFILE_VIEW,
                       "image",    image,
                       "parasite", GIMP_ICC_PROFILE_PARASITE_NAME,
                       NULL);
}


/*  private functions  */

static void
gimp_image_profile_view_update (GimpImageParasiteView *view)
{
  GimpImageProfileView *profile_view = GIMP_IMAGE_PROFILE_VIEW (view);
  GimpImage            *image;
  GimpColorManaged     *managed;
  GimpColorProfile     *profile;

  image   = gimp_image_parasite_view_get_image (view);
  managed = GIMP_COLOR_MANAGED (image);

  profile = gimp_color_managed_get_color_profile (managed);

  gimp_color_profile_view_set_profile (profile_view->profile_view, profile);
}
