/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include <mypaint-brush.h>

#include "core-types.h"

#include "gimpmybrush.h"
#include "gimpmybrush-load.h"
#include "gimptagged.h"

#include "gimp-intl.h"


struct _GimpMybrushPrivate
{
  gboolean  json_loaded;

  gchar    *brush_json;
  gdouble   radius;
  gdouble   opaque;
  gdouble   hardness;
};


static void          gimp_mybrush_tagged_iface_init     (GimpTaggedInterface  *iface);

static void          gimp_mybrush_finalize              (GObject              *object);
static void          gimp_mybrush_set_property          (GObject              *object,
                                                         guint                 property_id,
                                                         const GValue         *value,
                                                         GParamSpec           *pspec);
static void          gimp_mybrush_get_property          (GObject              *object,
                                                         guint                 property_id,
                                                         GValue               *value,
                                                         GParamSpec           *pspec);

static gint64        gimp_mybrush_get_memsize           (GimpObject           *object,
                                                         gint64               *gui_size);

static gchar       * gimp_mybrush_get_description       (GimpViewable         *viewable,
                                                         gchar               **tooltip);

static void          gimp_mybrush_dirty                 (GimpData             *data);
static const gchar * gimp_mybrush_get_extension         (GimpData             *data);

static gchar       * gimp_mybrush_get_checksum          (GimpTagged           *tagged);


G_DEFINE_TYPE_WITH_CODE (GimpMybrush, gimp_mybrush, GIMP_TYPE_DATA,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_TAGGED,
                                                gimp_mybrush_tagged_iface_init))

#define parent_class gimp_mybrush_parent_class


static void
gimp_mybrush_class_init (GimpMybrushClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  GimpDataClass     *data_class        = GIMP_DATA_CLASS (klass);

  object_class->finalize            = gimp_mybrush_finalize;
  object_class->get_property        = gimp_mybrush_get_property;
  object_class->set_property        = gimp_mybrush_set_property;

  gimp_object_class->get_memsize    = gimp_mybrush_get_memsize;

  viewable_class->default_icon_name = "gimp-tool-mypaint-brush";
  viewable_class->get_description   = gimp_mybrush_get_description;

  data_class->dirty                 = gimp_mybrush_dirty;
  data_class->get_extension         = gimp_mybrush_get_extension;

  g_type_class_add_private (klass, sizeof (GimpMybrushPrivate));
}

static void
gimp_mybrush_tagged_iface_init (GimpTaggedInterface *iface)
{
  iface->get_checksum = gimp_mybrush_get_checksum;
}

static void
gimp_mybrush_init (GimpMybrush *brush)
{
  brush->priv = G_TYPE_INSTANCE_GET_PRIVATE (brush,
                                             GIMP_TYPE_MYBRUSH,
                                             GimpMybrushPrivate);

  brush->priv->radius   = 1.0;
  brush->priv->opaque   = 1.0;
  brush->priv->hardness = 1.0;
}

static void
gimp_mybrush_finalize (GObject *object)
{
  GimpMybrush *brush = GIMP_MYBRUSH (object);

  if (brush->priv->brush_json)
    {
      g_free (brush->priv->brush_json);
      brush->priv->brush_json = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_mybrush_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  GimpMybrush *brush = GIMP_MYBRUSH (object);

  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_mybrush_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  GimpMybrush *brush = GIMP_MYBRUSH (object);

  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
gimp_mybrush_get_memsize (GimpObject *object,
                          gint64     *gui_size)
{
  GimpMybrush *brush   = GIMP_MYBRUSH (object);
  gint64       memsize = 0;

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gchar *
gimp_mybrush_get_description (GimpViewable  *viewable,
                              gchar        **tooltip)
{
  GimpMybrush *brush = GIMP_MYBRUSH (viewable);

  return g_strdup_printf ("%s",
                          gimp_object_get_name (brush));
}

static void
gimp_mybrush_dirty (GimpData *data)
{
  GimpMybrush *brush = GIMP_MYBRUSH (data);

  GIMP_DATA_CLASS (parent_class)->dirty (data);
}

static const gchar *
gimp_mybrush_get_extension (GimpData *data)
{
  return GIMP_MYBRUSH_FILE_EXTENSION;
}

static gchar *
gimp_mybrush_get_checksum (GimpTagged *tagged)
{
  GimpMybrush *brush           = GIMP_MYBRUSH (tagged);
  gchar       *checksum_string = NULL;

  return checksum_string;
}

/*  public functions  */

GimpData *
gimp_mybrush_new (GimpContext *context,
                  const gchar *name)
{
  g_return_val_if_fail (name != NULL, NULL);

  return g_object_new (GIMP_TYPE_MYBRUSH,
                       "name",      name,
                       "mime-type", "image/x-gimp-myb",
                       NULL);
}

GimpData *
gimp_mybrush_get_standard (GimpContext *context)
{
  static GimpData *standard_mybrush = NULL;

  if (! standard_mybrush)
    {
      standard_mybrush = gimp_mybrush_new (context, "Standard");

      gimp_data_clean (standard_mybrush);
      gimp_data_make_internal (standard_mybrush, "gimp-mybrush-standard");

      g_object_add_weak_pointer (G_OBJECT (standard_mybrush),
                                 (gpointer *) &standard_mybrush);
    }

  return standard_mybrush;
}

static void
gimp_mybrush_load_json (GimpMybrush *brush)
{
  GFile        *file          = gimp_data_get_file (GIMP_DATA (brush));
  MyPaintBrush *mypaint_brush = mypaint_brush_new ();

  mypaint_brush_from_defaults (mypaint_brush);

  if (file)
    {
      gchar *path = g_file_get_path (file);

      if (g_file_get_contents (path, &brush->priv->brush_json, NULL, NULL))
        {
          if (! mypaint_brush_from_string (mypaint_brush,
                                           brush->priv->brush_json))
            {
              g_printerr ("Failed to deserialize MyPaint brush\n");
              g_free (brush->priv->brush_json);
              brush->priv->brush_json = NULL;
            }
        }
      else
        {
          g_printerr ("Failed to load MyPaint brush\n");
        }
    }

  brush->priv->radius =
    mypaint_brush_get_base_value (mypaint_brush,
                                  MYPAINT_BRUSH_SETTING_RADIUS_LOGARITHMIC);

  brush->priv->opaque =
    mypaint_brush_get_base_value (mypaint_brush,
                                  MYPAINT_BRUSH_SETTING_OPAQUE);

  brush->priv->hardness =
    mypaint_brush_get_base_value (mypaint_brush,
                                  MYPAINT_BRUSH_SETTING_HARDNESS);

  mypaint_brush_unref (mypaint_brush);

  brush->priv->json_loaded = TRUE;
}

const gchar *
gimp_mybrush_get_brush_json (GimpMybrush *brush)
{
  g_return_val_if_fail (GIMP_IS_MYBRUSH (brush), NULL);

  if (! brush->priv->json_loaded)
    gimp_mybrush_load_json (brush);

  return brush->priv->brush_json;
}

gdouble
gimp_mybrush_get_radius (GimpMybrush *brush)
{
  g_return_val_if_fail (GIMP_IS_MYBRUSH (brush), 1.0);

  if (! brush->priv->json_loaded)
    gimp_mybrush_load_json (brush);

  return brush->priv->radius;
}

gdouble
gimp_mybrush_get_opaque (GimpMybrush *brush)
{
  g_return_val_if_fail (GIMP_IS_MYBRUSH (brush), 1.0);

  if (! brush->priv->json_loaded)
    gimp_mybrush_load_json (brush);

  return brush->priv->opaque;
}

gdouble
gimp_mybrush_get_hardness (GimpMybrush *brush)
{
  g_return_val_if_fail (GIMP_IS_MYBRUSH (brush), 1.0);

  if (! brush->priv->json_loaded)
    gimp_mybrush_load_json (brush);

  return brush->priv->hardness;
}
