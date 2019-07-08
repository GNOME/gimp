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

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpmarshal.h"

#include "gimpimageparasiteview.h"


enum
{
  PROP_0,
  PROP_IMAGE,
  PROP_PARASITE
};

enum
{
  UPDATE,
  LAST_SIGNAL
};


static void   gimp_image_parasite_view_constructed  (GObject     *object);
static void   gimp_image_parasite_view_finalize     (GObject     *object);
static void   gimp_image_parasite_view_set_property (GObject     *object,
                                                     guint         property_id,
                                                     const GValue *value,
                                                     GParamSpec   *pspec);
static void   gimp_image_parasite_view_get_property (GObject     *object,
                                                     guint         property_id,
                                                     GValue       *value,
                                                     GParamSpec   *pspec);

static void   gimp_image_parasite_view_parasite_changed (GimpImageParasiteView *view,
                                                         const gchar          *name);
static void   gimp_image_parasite_view_update           (GimpImageParasiteView *view);


G_DEFINE_TYPE (GimpImageParasiteView, gimp_image_parasite_view, GTK_TYPE_BOX)

#define parent_class gimp_image_parasite_view_parent_class

static guint view_signals[LAST_SIGNAL] = { 0 };


static void
gimp_image_parasite_view_class_init (GimpImageParasiteViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  view_signals[UPDATE] =
    g_signal_new ("update",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageParasiteViewClass, update),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  object_class->constructed  = gimp_image_parasite_view_constructed;
  object_class->finalize     = gimp_image_parasite_view_finalize;
  object_class->set_property = gimp_image_parasite_view_set_property;
  object_class->get_property = gimp_image_parasite_view_get_property;

  klass->update              = NULL;

  g_object_class_install_property (object_class, PROP_IMAGE,
                                   g_param_spec_object ("image", NULL, NULL,
                                                        GIMP_TYPE_IMAGE,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_PARASITE,
                                   g_param_spec_string ("parasite", NULL, NULL,
                                                        NULL,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_image_parasite_view_init (GimpImageParasiteView *view)
{
  gtk_orientable_set_orientation (GTK_ORIENTABLE (view),
                                  GTK_ORIENTATION_VERTICAL);

  view->parasite = NULL;
}

static void
gimp_image_parasite_view_constructed (GObject *object)
{
  GimpImageParasiteView *view = GIMP_IMAGE_PARASITE_VIEW (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (view->parasite != NULL);
  gimp_assert (view->image != NULL);

  g_signal_connect_object (view->image, "parasite-attached",
                           G_CALLBACK (gimp_image_parasite_view_parasite_changed),
                           G_OBJECT (view),
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (view->image, "parasite-detached",
                           G_CALLBACK (gimp_image_parasite_view_parasite_changed),
                           G_OBJECT (view),
                           G_CONNECT_SWAPPED);

  gimp_image_parasite_view_update (view);
}

static void
gimp_image_parasite_view_finalize (GObject *object)
{
  GimpImageParasiteView *view = GIMP_IMAGE_PARASITE_VIEW (object);

  g_clear_pointer (&view->parasite, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_image_parasite_view_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  GimpImageParasiteView *view = GIMP_IMAGE_PARASITE_VIEW (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      view->image = g_value_get_object (value);
      break;
    case PROP_PARASITE:
      view->parasite = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_image_parasite_view_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  GimpImageParasiteView *view = GIMP_IMAGE_PARASITE_VIEW (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      g_value_set_object (value, view->image);
      break;
    case PROP_PARASITE:
      g_value_set_string (value, view->parasite);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
gimp_image_parasite_view_new (GimpImage   *image,
                              const gchar *parasite)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (parasite != NULL, NULL);

  return g_object_new (GIMP_TYPE_IMAGE_PARASITE_VIEW,
                       "image",    image,
                       "parasite", parasite,
                       NULL);
}


GimpImage *
gimp_image_parasite_view_get_image (GimpImageParasiteView *view)
{
  g_return_val_if_fail (GIMP_IS_IMAGE_PARASITE_VIEW (view), NULL);

  return view->image;
}

const GimpParasite *
gimp_image_parasite_view_get_parasite (GimpImageParasiteView *view)
{
  g_return_val_if_fail (GIMP_IS_IMAGE_PARASITE_VIEW (view), NULL);

  return gimp_image_parasite_find (view->image, view->parasite);
}


/*  private functions  */

static void
gimp_image_parasite_view_parasite_changed (GimpImageParasiteView *view,
                                           const gchar           *name)
{
  if (name && view->parasite && strcmp (name, view->parasite) == 0)
    gimp_image_parasite_view_update (view);
}

static void
gimp_image_parasite_view_update (GimpImageParasiteView *view)
{
  g_signal_emit (view, view_signals[UPDATE], 0);
}
