/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpColorProfileView
 * Copyright (C) 2014 Michael Natterer <mitch@gimp.org>
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

#include "libgimpcolor/gimpcolor.h"

#include "gimpwidgetstypes.h"

#include "gimpcolorprofileview.h"

#include "libgimp/libgimp-intl.h"


/**
 * SECTION: gimpcolorprofileview
 * @title: GimpColorProfileView
 * @short_description: A widget for viewing color profile properties
 *
 * A widget for viewing the properties of a #GimpColorProfile.
 **/


struct _GimpColorProfileViewPrivate
{
  GimpColorProfile *profile;
};


static void   gimp_color_profile_view_constructed  (GObject *object);
static void   gimp_color_profile_view_finalize     (GObject *object);


G_DEFINE_TYPE_WITH_PRIVATE (GimpColorProfileView, gimp_color_profile_view,
                            GTK_TYPE_TEXT_VIEW)

#define parent_class gimp_color_profile_view_parent_class


static void
gimp_color_profile_view_class_init (GimpColorProfileViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = gimp_color_profile_view_constructed;
  object_class->finalize    = gimp_color_profile_view_finalize;
}

static void
gimp_color_profile_view_init (GimpColorProfileView *view)
{
  view->priv = gimp_color_profile_view_get_instance_private (view);
}

static void
gimp_color_profile_view_constructed (GObject *object)
{
  GtkTextBuffer *buffer;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (object));

  gtk_text_buffer_create_tag (buffer, "text",
                              NULL);
  gtk_text_buffer_create_tag (buffer, "title",
                              "weight", PANGO_WEIGHT_BOLD,
                              "scale",  PANGO_SCALE_LARGE,
                              NULL);
  gtk_text_buffer_create_tag (buffer, "header",
                              "weight", PANGO_WEIGHT_BOLD,
                              NULL);
  gtk_text_buffer_create_tag (buffer, "error",
                              "style",  PANGO_STYLE_OBLIQUE,
                              NULL);

  gtk_text_view_set_editable (GTK_TEXT_VIEW (object), FALSE);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (object), GTK_WRAP_WORD);

  gtk_text_view_set_pixels_above_lines (GTK_TEXT_VIEW (object), 6);
  gtk_text_view_set_left_margin (GTK_TEXT_VIEW (object), 6);
  gtk_text_view_set_right_margin (GTK_TEXT_VIEW (object), 6);
}

static void
gimp_color_profile_view_finalize (GObject *object)
{
  GimpColorProfileView *view = GIMP_COLOR_PROFILE_VIEW (object);

  g_clear_object (&view->priv->profile);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

GtkWidget *
gimp_color_profile_view_new (void)
{
  return g_object_new (GIMP_TYPE_COLOR_PROFILE_VIEW, NULL);
}

void
gimp_color_profile_view_set_profile (GimpColorProfileView *view,
                                     GimpColorProfile     *profile)
{
  GtkTextBuffer *buffer;

  g_return_if_fail (GIMP_IS_COLOR_PROFILE_VIEW (view));
  g_return_if_fail (profile == NULL || GIMP_IS_COLOR_PROFILE (profile));

  if (profile == view->priv->profile)
    return;

  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

  gtk_text_buffer_set_text (buffer, "", 0);

  if (g_set_object (&view->priv->profile, profile) && profile)
    {
      GtkTextIter  iter;
      const gchar *text;

      gtk_text_buffer_get_start_iter (buffer, &iter);

      text = gimp_color_profile_get_label (profile);
      if (text && strlen (text))
        {
          gtk_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                                    text, -1,
                                                    "title", NULL);
          gtk_text_buffer_insert (buffer, &iter, "\n", 1);
        }

      text = gimp_color_profile_get_model (profile);
      if (text && strlen (text))
        {
          gtk_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                                    text, -1,
                                                    "text", NULL);
          gtk_text_buffer_insert (buffer, &iter, "\n", 1);
        }

      text = gimp_color_profile_get_manufacturer (profile);
      if (text && strlen (text))
        {
          gtk_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                                    _("Manufacturer: "), -1,
                                                    "header", NULL);
          gtk_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                                    text, -1,
                                                    "text", NULL);
          gtk_text_buffer_insert (buffer, &iter, "\n", 1);
        }

      text = gimp_color_profile_get_copyright (profile);
      if (text && strlen (text))
        {
          gtk_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                                    _("Copyright: "), -1,
                                                    "header", NULL);
          gtk_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                                    text, -1,
                                                    "text", NULL);
          gtk_text_buffer_insert (buffer, &iter, "\n", 1);
        }
    }
}

void
gimp_color_profile_view_set_error (GimpColorProfileView *view,
                                   const gchar          *message)
{
  GtkTextBuffer *buffer;
  GtkTextIter    iter;

  g_return_if_fail (GIMP_IS_COLOR_PROFILE_VIEW (view));
  g_return_if_fail (message != NULL);

  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

  gtk_text_buffer_set_text (buffer, "", 0);

  gtk_text_buffer_get_start_iter (buffer, &iter);

  gtk_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            message, -1,
                                            "error", NULL);
}
