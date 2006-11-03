/* The GIMP -- an image manipulation program
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


static void        gimp_image_profile_view_dispose   (GObject     *object);

static GtkWidget * gimp_image_profile_view_add_label (GtkTable    *table,
                                                      gint         row,
                                                      const gchar *text);

static void        gimp_image_profile_view_update    (GimpImageParasiteView *view);


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
  gint row = 0;

  view->table = gtk_table_new (3, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (view->table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (view->table), 6);
  gtk_container_add (GTK_CONTAINER (view), view->table);

  view->name_label =
    gimp_image_profile_view_add_label (GTK_TABLE (view->table), row++,
                                       _("Name:"));
  view->desc_label =
    gimp_image_profile_view_add_label (GTK_TABLE (view->table), row++,
                                       _("Description:"));
  view->info_label =
    gimp_image_profile_view_add_label (GTK_TABLE (view->table), row++,
                                       _("Info:"));

  view->message = g_object_new (GTK_TYPE_LABEL,
                                "xalign", 0.5,
                                "yalign", 0.5,
                                NULL);
  gtk_container_add (GTK_CONTAINER (view), view->message);

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
                       "parasite", "icc-profile",
                       "image",    image,
                       NULL);
}


/*  private functions  */

static GtkWidget *
gimp_image_profile_view_add_label (GtkTable    *table,
                                   gint         row,
                                   const gchar *text)
{
  GtkWidget *label;
  GtkWidget *desc;

  desc = g_object_new (GTK_TYPE_LABEL,
                       "label",  text,
                       "xalign", 1.0,
                       "yalign", 0.0,
                       NULL);
  gimp_label_set_attributes (GTK_LABEL (desc),
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_table_attach (table, desc,
                    0, 1, row, row + 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (desc);

  label = g_object_new (GTK_TYPE_LABEL,
                        "xalign",     0.0,
                        "yalign",     0.0,
                        "selectable", TRUE,
                        NULL);
  gtk_table_attach (table, label,
                    1, 2, row, row + 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  return label;
}

static gboolean
gimp_image_profile_view_query (GimpImageProfileView *view)
{
  GimpImage *image;
  gchar     *name  = NULL;
  gchar     *desc  = NULL;
  gchar     *info  = NULL;
  GError    *error = NULL;

  image = gimp_image_parasite_view_get_image (GIMP_IMAGE_PARASITE_VIEW (view));

  if (plug_in_icc_profile_info (image,
                                gimp_get_user_context (image->gimp),
                                NULL,
                                &name, &desc, &info,
                                &error))
    {
      gtk_label_set_text (GTK_LABEL (view->message), NULL);
      gtk_widget_hide (view->message);

      gtk_label_set_text (GTK_LABEL (view->name_label), name);
      gtk_label_set_text (GTK_LABEL (view->desc_label), desc);
      gtk_label_set_text (GTK_LABEL (view->info_label), info);
      gtk_widget_show (view->table);
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (view->message), error->message);
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

  gtk_label_set_text (GTK_LABEL (profile_view->name_label), NULL);
  gtk_label_set_text (GTK_LABEL (profile_view->desc_label), NULL);
  gtk_label_set_text (GTK_LABEL (profile_view->info_label), NULL);
  gtk_widget_hide (profile_view->table);

  gtk_label_set_text (GTK_LABEL (profile_view->message), _("Querying..."));
  gtk_widget_show (profile_view->message);

  if (profile_view->idle_id)
    g_source_remove (profile_view->idle_id);

  profile_view->idle_id =
    g_idle_add ((GSourceFunc) gimp_image_profile_view_query, profile_view);
}
