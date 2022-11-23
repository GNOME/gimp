/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaImageProfileView
 * Copyright (C) 2006  Sven Neumann <sven@ligma.org>
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

#include "libligmaconfig/ligmaconfig.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligmaimage.h"
#include "core/ligmaimage-color-profile.h"

#include "ligmaimageprofileview.h"

#include "ligma-intl.h"


static void   ligma_image_profile_view_update (LigmaImageParasiteView *view);


G_DEFINE_TYPE (LigmaImageProfileView,
               ligma_image_profile_view, LIGMA_TYPE_IMAGE_PARASITE_VIEW)

#define parent_class ligma_image_profile_view_parent_class


static void
ligma_image_profile_view_class_init (LigmaImageProfileViewClass *klass)
{
  LigmaImageParasiteViewClass *view_class;

  view_class = LIGMA_IMAGE_PARASITE_VIEW_CLASS (klass);

  view_class->update = ligma_image_profile_view_update;
}

static void
ligma_image_profile_view_init (LigmaImageProfileView *view)
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

  profile_view = ligma_color_profile_view_new ();
  gtk_container_add (GTK_CONTAINER (scrolled_window), profile_view);
  gtk_widget_show (profile_view);

  view->profile_view = LIGMA_COLOR_PROFILE_VIEW (profile_view);
}


/*  public functions  */

GtkWidget *
ligma_image_profile_view_new (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return g_object_new (LIGMA_TYPE_IMAGE_PROFILE_VIEW,
                       "image",    image,
                       "parasite", LIGMA_ICC_PROFILE_PARASITE_NAME,
                       NULL);
}


/*  private functions  */

static void
ligma_image_profile_view_update (LigmaImageParasiteView *view)
{
  LigmaImageProfileView *profile_view = LIGMA_IMAGE_PROFILE_VIEW (view);
  LigmaImage            *image;
  LigmaColorManaged     *managed;
  LigmaColorProfile     *profile;

  image   = ligma_image_parasite_view_get_image (view);
  managed = LIGMA_COLOR_MANAGED (image);

  profile = ligma_color_managed_get_color_profile (managed);

  ligma_color_profile_view_set_profile (profile_view->profile_view, profile);
}
