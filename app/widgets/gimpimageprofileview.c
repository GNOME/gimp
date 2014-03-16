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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <glib.h>  /* lcms.h uses the "inline" keyword */

#include <lcms2.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpconfig/gimpconfig.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpimage.h"

#include "gimpimageprofileview.h"

#include "gimp-intl.h"


static void  gimp_image_profile_view_update  (GimpImageParasiteView *view);


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
                       "parasite", "icc-profile",
                       NULL);
}


/*  private functions  */

static void
gimp_image_profile_view_update (GimpImageParasiteView *view)
{
  GimpImageProfileView *profile_view = GIMP_IMAGE_PROFILE_VIEW (view);
  GimpImage            *image;
  GimpColorConfig      *config;
  const GimpParasite   *parasite;
  GimpColorProfile     *profile = NULL;
  GError               *error   = NULL;

  image    = gimp_image_parasite_view_get_image (view);
  parasite = gimp_image_parasite_view_get_parasite (view);

  config = image->gimp->config->color_management;

  if (parasite)
    {
      profile = gimp_lcms_profile_open_from_data (gimp_parasite_data (parasite),
                                                  gimp_parasite_data_size (parasite),
                                                  NULL, &error);
      if (! profile)
        {
          g_message (_("Error parsing 'icc-profile': %s"), error->message);
          g_clear_error (&error);
        }
    }
  else if (config->rgb_profile)
    {
      profile = gimp_lcms_profile_open_from_file (config->rgb_profile,
                                                  NULL, &error);
      if (! profile)
        {
          g_message ("%s", error->message);
          g_clear_error (&error);
        }
    }

  if (profile && ! gimp_lcms_profile_is_rgb (profile))
    {
      g_message (_("Color profile is not for RGB color space (skipping)"));
      cmsCloseProfile (profile);
      profile = NULL;
    }

  if (! profile)
    profile = gimp_lcms_create_srgb_profile ();

  gimp_color_profile_view_set_profile (profile_view->profile_view, profile);

  cmsCloseProfile (profile);
}
