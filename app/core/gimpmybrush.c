/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmamybrush.c
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

#include "ligma-memsize.h"
#include "ligmamybrush.h"
#include "ligmamybrush-load.h"
#include "ligmamybrush-private.h"
#include "ligmatagged.h"

#include "ligma-intl.h"


static void          ligma_mybrush_tagged_iface_init     (LigmaTaggedInterface  *iface);

static void          ligma_mybrush_finalize              (GObject              *object);
static void          ligma_mybrush_set_property          (GObject              *object,
                                                         guint                 property_id,
                                                         const GValue         *value,
                                                         GParamSpec           *pspec);
static void          ligma_mybrush_get_property          (GObject              *object,
                                                         guint                 property_id,
                                                         GValue               *value,
                                                         GParamSpec           *pspec);

static gint64        ligma_mybrush_get_memsize           (LigmaObject           *object,
                                                         gint64               *gui_size);

static gchar       * ligma_mybrush_get_description       (LigmaViewable         *viewable,
                                                         gchar               **tooltip);

static void          ligma_mybrush_dirty                 (LigmaData             *data);
static const gchar * ligma_mybrush_get_extension         (LigmaData             *data);

static gchar       * ligma_mybrush_get_checksum          (LigmaTagged           *tagged);


G_DEFINE_TYPE_WITH_CODE (LigmaMybrush, ligma_mybrush, LIGMA_TYPE_DATA,
                         G_ADD_PRIVATE (LigmaMybrush)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_TAGGED,
                                                ligma_mybrush_tagged_iface_init))

#define parent_class ligma_mybrush_parent_class


static void
ligma_mybrush_class_init (LigmaMybrushClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass   *ligma_object_class = LIGMA_OBJECT_CLASS (klass);
  LigmaViewableClass *viewable_class    = LIGMA_VIEWABLE_CLASS (klass);
  LigmaDataClass     *data_class        = LIGMA_DATA_CLASS (klass);

  object_class->finalize            = ligma_mybrush_finalize;
  object_class->get_property        = ligma_mybrush_get_property;
  object_class->set_property        = ligma_mybrush_set_property;

  ligma_object_class->get_memsize    = ligma_mybrush_get_memsize;

  viewable_class->default_icon_name = "ligma-tool-mypaint-brush";
  viewable_class->get_description   = ligma_mybrush_get_description;

  data_class->dirty                 = ligma_mybrush_dirty;
  data_class->get_extension         = ligma_mybrush_get_extension;
}

static void
ligma_mybrush_tagged_iface_init (LigmaTaggedInterface *iface)
{
  iface->get_checksum = ligma_mybrush_get_checksum;
}

static void
ligma_mybrush_init (LigmaMybrush *brush)
{
  brush->priv = ligma_mybrush_get_instance_private (brush);

  brush->priv->radius   = 1.0;
  brush->priv->opaque   = 1.0;
  brush->priv->hardness = 1.0;
  brush->priv->eraser   = FALSE;
}

static void
ligma_mybrush_finalize (GObject *object)
{
  LigmaMybrush *brush = LIGMA_MYBRUSH (object);

  g_clear_pointer (&brush->priv->brush_json, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_mybrush_set_property (GObject      *object,
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
ligma_mybrush_get_property (GObject    *object,
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
ligma_mybrush_get_memsize (LigmaObject *object,
                          gint64     *gui_size)
{
  LigmaMybrush *brush   = LIGMA_MYBRUSH (object);
  gint64       memsize = 0;

  memsize += ligma_string_get_memsize (brush->priv->brush_json);

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gchar *
ligma_mybrush_get_description (LigmaViewable  *viewable,
                              gchar        **tooltip)
{
  LigmaMybrush *brush = LIGMA_MYBRUSH (viewable);

  return g_strdup_printf ("%s",
                          ligma_object_get_name (brush));
}

static void
ligma_mybrush_dirty (LigmaData *data)
{
  LIGMA_DATA_CLASS (parent_class)->dirty (data);
}

static const gchar *
ligma_mybrush_get_extension (LigmaData *data)
{
  return LIGMA_MYBRUSH_FILE_EXTENSION;
}

static gchar *
ligma_mybrush_get_checksum (LigmaTagged *tagged)
{
  LigmaMybrush *brush           = LIGMA_MYBRUSH (tagged);
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

LigmaData *
ligma_mybrush_new (LigmaContext *context,
                  const gchar *name)
{
  g_return_val_if_fail (name != NULL, NULL);

  return g_object_new (LIGMA_TYPE_MYBRUSH,
                       "name",      name,
                       "mime-type", "image/x-ligma-myb",
                       NULL);
}

LigmaData *
ligma_mybrush_get_standard (LigmaContext *context)
{
  static LigmaData *standard_mybrush = NULL;

  if (! standard_mybrush)
    {
      standard_mybrush = ligma_mybrush_new (context, "Standard");

      ligma_data_clean (standard_mybrush);
      ligma_data_make_internal (standard_mybrush, "ligma-mybrush-standard");

      g_object_add_weak_pointer (G_OBJECT (standard_mybrush),
                                 (gpointer *) &standard_mybrush);
    }

  return standard_mybrush;
}

const gchar *
ligma_mybrush_get_brush_json (LigmaMybrush *brush)
{
  g_return_val_if_fail (LIGMA_IS_MYBRUSH (brush), NULL);

  return brush->priv->brush_json;
}

gdouble
ligma_mybrush_get_radius (LigmaMybrush *brush)
{
  g_return_val_if_fail (LIGMA_IS_MYBRUSH (brush), 1.0);

  return brush->priv->radius;
}

gdouble
ligma_mybrush_get_opaque (LigmaMybrush *brush)
{
  g_return_val_if_fail (LIGMA_IS_MYBRUSH (brush), 1.0);

  return brush->priv->opaque;
}

gdouble
ligma_mybrush_get_hardness (LigmaMybrush *brush)
{
  g_return_val_if_fail (LIGMA_IS_MYBRUSH (brush), 1.0);

  return brush->priv->hardness;
}

gdouble
ligma_mybrush_get_offset_by_random (LigmaMybrush *brush)
{
  g_return_val_if_fail (LIGMA_IS_MYBRUSH (brush), 1.0);

  return brush->priv->offset_by_random;
}

gboolean
ligma_mybrush_get_is_eraser (LigmaMybrush *brush)
{
  g_return_val_if_fail (LIGMA_IS_MYBRUSH (brush), FALSE);

  return brush->priv->eraser;
}
