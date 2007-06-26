/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpImageProfileView
 * Copyright (C) 2006  Sven Neumann <sven@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpimage.h"

#include "plug-in/plug-in-icc-profile.h"

#include "gimpimageprofileview.h"

#include "gimp-intl.h"


static void  gimp_image_profile_view_dispose (GObject               *object);

static void  gimp_image_profile_view_update  (GimpImageParasiteView *view);


G_DEFINE_TYPE (GimpImageProfileView,
               gimp_image_profile_view, GIMP_TYPE_IMAGE_PARASITE_VIEW)

#define parent_class gimp_image_profile_view_parent_class


static void
gimp_image_profile_view_class_init (GimpImageProfileViewClass *klass)
{
  GObjectClass               *object_class = G_OBJECT_CLASS (klass);
  GimpImageParasiteViewClass *view_class;

  view_class = GIMP_IMAGE_PARASITE_VIEW_CLASS (klass);

  object_class->dispose = gimp_image_profile_view_dispose;

  view_class->update    = gimp_image_profile_view_update;
}

static void
gimp_image_profile_view_init (GimpImageProfileView *view)
{
  GtkWidget *scrolled_window;
  GtkWidget *text_view;

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 2);
  gtk_container_add (GTK_CONTAINER (view), scrolled_window);
  gtk_widget_show (scrolled_window);

  text_view = gtk_text_view_new ();
  gtk_text_view_set_editable (GTK_TEXT_VIEW (text_view), FALSE);
  gtk_container_add (GTK_CONTAINER (scrolled_window), text_view);
  gtk_widget_show (text_view);

  view->buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));

  gtk_text_buffer_create_tag (view->buffer, "strong",
                              "weight", PANGO_WEIGHT_BOLD,
                              "scale",  PANGO_SCALE_LARGE,
                              NULL);
  gtk_text_buffer_create_tag (view->buffer, "emphasis",
                              "style",  PANGO_STYLE_OBLIQUE,
                              NULL);

  view->idle_id = 0;
}

static void
gimp_image_profile_view_dispose (GObject *object)
{
  GimpImageProfileView *view = GIMP_IMAGE_PROFILE_VIEW (object);

  if (view->idle_id)
    {
      g_source_remove (view->idle_id);
      view->idle_id = 0;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
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

static gboolean
gimp_image_profile_view_query (GimpImageProfileView *view)
{
  GimpImage   *image;
  gchar       *name  = NULL;
  gchar       *desc  = NULL;
  gchar       *info  = NULL;
  GError      *error = NULL;
  GtkTextIter  iter;

  gtk_text_buffer_set_text (view->buffer, "", 0);
  gtk_text_buffer_get_start_iter (view->buffer, &iter);

  image = gimp_image_parasite_view_get_image (GIMP_IMAGE_PARASITE_VIEW (view));

  if (plug_in_icc_profile_info (image,
                                gimp_get_user_context (image->gimp),
                                NULL,
                                &name, &desc, &info,
                                &error))
    {
      if (desc || name)
        {
          const gchar *title;

          if (desc && strlen (desc))
            title = desc;
          else
            title = name;

          gtk_text_buffer_insert_with_tags_by_name (view->buffer, &iter,
                                                    title, -1,
                                                    "strong", NULL);
          gtk_text_buffer_insert (view->buffer, &iter, "\n\n", -1);
        }

      if (info)
        gtk_text_buffer_insert (view->buffer, &iter, info, -1);
    }
  else
    {
      gtk_text_buffer_insert_with_tags_by_name (view->buffer, &iter,
                                                error->message, -1,
                                                "emphasis", NULL);
      g_error_free (error);
    }

  g_free (name);
  g_free (desc);
  g_free (info);

  return FALSE;
}

static void
gimp_image_profile_view_update (GimpImageParasiteView *view)
{
  GimpImageProfileView *profile_view = GIMP_IMAGE_PROFILE_VIEW (view);
  GtkTextIter           iter;

  gtk_text_buffer_set_text (profile_view->buffer, "", 0);
  gtk_text_buffer_get_start_iter (profile_view->buffer, &iter);
  gtk_text_buffer_insert_with_tags_by_name (profile_view->buffer, &iter,
                                            _("Querying..."), -1,
                                            "emphasis", NULL);

  if (profile_view->idle_id)
    g_source_remove (profile_view->idle_id);

  profile_view->idle_id =
    g_idle_add ((GSourceFunc) gimp_image_profile_view_query, profile_view);
}
