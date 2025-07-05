/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpmybrush.c
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

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "core-types.h"

#include "gimp-memsize.h"
#include "gimpmybrush.h"
#include "gimpmybrush-load.h"
#include "gimpmybrush-private.h"
#include "gimptagged.h"

#include "gimp-intl.h"


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
                         G_ADD_PRIVATE (GimpMybrush)
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
}

static void
gimp_mybrush_tagged_iface_init (GimpTaggedInterface *iface)
{
  iface->get_checksum = gimp_mybrush_get_checksum;
}

static void
gimp_mybrush_init (GimpMybrush *brush)
{
  brush->priv = gimp_mybrush_get_instance_private (brush);

  brush->priv->radius        = 1.0;
  brush->priv->opaque        = 1.0;
  brush->priv->hardness      = 1.0;
  brush->priv->gain          = 0.0;
  brush->priv->pigment       = 0.0;
  brush->priv->posterize     = 0.0;
  brush->priv->posterize_num = 1.0;
  brush->priv->eraser        = FALSE;
}

static void
gimp_mybrush_finalize (GObject *object)
{
  GimpMybrush *brush = GIMP_MYBRUSH (object);

  g_clear_pointer (&brush->priv->brush_json, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_mybrush_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
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

  memsize += gimp_string_get_memsize (brush->priv->brush_json);

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

  if (brush->priv->brush_json)
    {
      GChecksum *checksum = g_checksum_new (G_CHECKSUM_MD5);

      g_checksum_update (checksum,
                         (const guchar *) brush->priv->brush_json,
                         strlen (brush->priv->brush_json));

      checksum_string = g_strdup (g_checksum_get_string (checksum));

      g_checksum_free (checksum);
    }

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
      g_set_weak_pointer (&standard_mybrush,
                          gimp_mybrush_new (context, "Standard"));

      gimp_data_clean (standard_mybrush);
      gimp_data_make_internal (standard_mybrush, "gimp-mybrush-standard");
    }

  return standard_mybrush;
}

const gchar *
gimp_mybrush_get_brush_json (GimpMybrush *brush)
{
  g_return_val_if_fail (GIMP_IS_MYBRUSH (brush), NULL);

  return brush->priv->brush_json;
}

gdouble
gimp_mybrush_get_radius (GimpMybrush *brush)
{
  g_return_val_if_fail (GIMP_IS_MYBRUSH (brush), 1.0);

  return brush->priv->radius;
}

gdouble
gimp_mybrush_get_opaque (GimpMybrush *brush)
{
  g_return_val_if_fail (GIMP_IS_MYBRUSH (brush), 1.0);

  return brush->priv->opaque;
}

gdouble
gimp_mybrush_get_hardness (GimpMybrush *brush)
{
  g_return_val_if_fail (GIMP_IS_MYBRUSH (brush), 1.0);

  return brush->priv->hardness;
}

gdouble
gimp_mybrush_get_gain (GimpMybrush *brush)
{
  g_return_val_if_fail (GIMP_IS_MYBRUSH (brush), 0.0);

  return brush->priv->gain;
}


gdouble
gimp_mybrush_get_pigment (GimpMybrush *brush)
{
  g_return_val_if_fail (GIMP_IS_MYBRUSH (brush), 0.0);

  return brush->priv->pigment;
}

gdouble
gimp_mybrush_get_posterize (GimpMybrush *brush)
{
  g_return_val_if_fail (GIMP_IS_MYBRUSH (brush), 0.0);

  return brush->priv->posterize;
}

gdouble
gimp_mybrush_get_posterize_num (GimpMybrush *brush)
{
  g_return_val_if_fail (GIMP_IS_MYBRUSH (brush), 0.05);

  return brush->priv->posterize_num;
}

gdouble
gimp_mybrush_get_offset_by_random (GimpMybrush *brush)
{
  g_return_val_if_fail (GIMP_IS_MYBRUSH (brush), 1.0);

  return brush->priv->offset_by_random;
}

gboolean
gimp_mybrush_get_is_eraser (GimpMybrush *brush)
{
  g_return_val_if_fail (GIMP_IS_MYBRUSH (brush), FALSE);

  return brush->priv->eraser;
}
