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
#include "core/gimpparamspecs.h"

#include "pdb/gimppdb.h"

#include "gimpimageprofileview.h"

#include "gimp-intl.h"


#define ICC_PROFILE_INFO_PROC  "plug-in-icc-profile-info"


enum
{
  PROP_0,
  PROP_IMAGE
};


static GObject * gimp_image_profile_view_constructor  (GType              type,
                                                       guint              n_params,
                                                       GObjectConstructParam *params);
static void      gimp_image_profile_view_set_property (GObject           *object,
                                                       guint              property_id,
                                                       const GValue      *value,
                                                       GParamSpec        *pspec);
static void      gimp_image_profile_view_get_property (GObject           *object,
                                                       guint              property_id,
                                                       GValue            *value,
                                                       GParamSpec        *pspec);
static void      gimp_image_profile_view_dispose      (GObject           *object);

static GtkWidget * gimp_image_profile_view_add_label      (GtkTable    *table,
                                                           gint         row,
                                                           const gchar *text);

static void      gimp_image_profile_view_parasite_changed (GimpImageProfileView *view,
                                                           const gchar          *name);
static void      gimp_image_profile_view_update            (GimpImageProfileView *view);


G_DEFINE_TYPE (GimpImageProfileView, gimp_image_profile_view, GTK_TYPE_VBOX)

#define parent_class gimp_image_profile_view_parent_class


static void
gimp_image_profile_view_class_init (GimpImageProfileViewClass *klass)
{
  GObjectClass   *object_class     = G_OBJECT_CLASS (klass);

  object_class->constructor  = gimp_image_profile_view_constructor;
  object_class->set_property = gimp_image_profile_view_set_property;
  object_class->get_property = gimp_image_profile_view_get_property;
  object_class->dispose      = gimp_image_profile_view_dispose;

  g_object_class_install_property (object_class, PROP_IMAGE,
                                   g_param_spec_object ("image", NULL, NULL,
                                                        GIMP_TYPE_IMAGE,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
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
gimp_image_profile_view_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  GimpImageProfileView *view = GIMP_IMAGE_PROFILE_VIEW (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      view->image = GIMP_IMAGE (g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_image_profile_view_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  GimpImageProfileView *view = GIMP_IMAGE_PROFILE_VIEW (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      g_value_set_object (value, view->image);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static GObject *
gimp_image_profile_view_constructor (GType                  type,
                                     guint                  n_params,
                                     GObjectConstructParam *params)
{
  GimpImageProfileView *view;
  GObject              *object;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  view = GIMP_IMAGE_PROFILE_VIEW (object);

  g_assert (view->image != NULL);

  g_signal_connect_object (view->image, "parasite-attached",
                           G_CALLBACK (gimp_image_profile_view_parasite_changed),
                           G_OBJECT (view),
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (view->image, "parasite-detached",
                           G_CALLBACK (gimp_image_profile_view_parasite_changed),
                           G_OBJECT (view),
                           G_CONNECT_SWAPPED);

  gimp_image_profile_view_update (view);

  return object;
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
                       "image", image,
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

static void
gimp_image_profile_view_parasite_changed (GimpImageProfileView *view,
                                          const gchar          *name)
{
  if (name && strcmp (name, "icc-profile") == 0)
    gimp_image_profile_view_update (view);
}

static void
gimp_image_profile_view_set_value (GtkLabel    *label,
                                   GValueArray *values,
                                   gint         index)
{
  if (values->n_values > index)
    {
      GValue *value = g_value_array_get_nth (values, index);

      if (G_VALUE_HOLDS_STRING (value))
        {
          gtk_label_set_text (label, g_value_get_string (value));
          return;
        }
    }

  gtk_label_set_text (label, NULL);
}


static gboolean
gimp_image_profile_view_query (GimpImageProfileView *view)
{
  Gimp              *gimp = view->image->gimp;
  GValueArray       *return_vals;
  GimpPDBStatusType  status;

  return_vals =
    gimp_pdb_execute_procedure_by_name (gimp->pdb,
                                        gimp_get_user_context (gimp),
                                        NULL,
                                        ICC_PROFILE_INFO_PROC,
                                        GIMP_TYPE_INT32,
                                        GIMP_RUN_NONINTERACTIVE,
                                        GIMP_TYPE_IMAGE_ID,
                                        gimp_image_get_ID (view->image),
                                        G_TYPE_NONE);

  status = g_value_get_enum (return_vals->values);

  switch (status)
    {
    case GIMP_PDB_SUCCESS:
      gtk_label_set_text (GTK_LABEL (view->message), NULL);
      gtk_widget_hide (view->message);

      gimp_image_profile_view_set_value (GTK_LABEL (view->name_label),
                                         return_vals, 1);
      gimp_image_profile_view_set_value (GTK_LABEL (view->desc_label),
                                         return_vals, 2);
      gimp_image_profile_view_set_value (GTK_LABEL (view->info_label),
                                         return_vals, 3);
      gtk_widget_show (view->table);
      break;

    default:
      gtk_label_set_text (GTK_LABEL (view->message), "Query failed.");
      break;
    }

  g_value_array_free (return_vals);

  return FALSE;
}

static void
gimp_image_profile_view_update (GimpImageProfileView *view)
{
  Gimp          *gimp = view->image->gimp;
  GimpProcedure *procedure;

  gtk_label_set_text (GTK_LABEL (view->name_label), NULL);
  gtk_label_set_text (GTK_LABEL (view->desc_label), NULL);
  gtk_label_set_text (GTK_LABEL (view->info_label), NULL);
  gtk_widget_hide (view->table);

  /* FIXME: do this from an idle handler, or even asynchronously */

  procedure = gimp_pdb_lookup_procedure (gimp->pdb, ICC_PROFILE_INFO_PROC);

  if (procedure)
    {
      gtk_label_set_text (GTK_LABEL (view->message), "Querying...");
      gtk_widget_show (view->message);

      if (view->idle_id)
        g_source_remove (view->idle_id);

      view->idle_id = g_idle_add ((GSourceFunc) gimp_image_profile_view_query,
                                  view);
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (view->message), "Plug-In is missing.");
      gtk_widget_show (view->message);
    }
}
