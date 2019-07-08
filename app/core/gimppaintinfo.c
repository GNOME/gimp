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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "core-types.h"

#include "paint/gimppaintoptions.h"

#include "gimp.h"
#include "gimppaintinfo.h"


static void    gimp_paint_info_dispose         (GObject       *object);
static void    gimp_paint_info_finalize        (GObject       *object);
static gchar * gimp_paint_info_get_description (GimpViewable  *viewable,
                                                gchar        **tooltip);


G_DEFINE_TYPE (GimpPaintInfo, gimp_paint_info, GIMP_TYPE_VIEWABLE)

#define parent_class gimp_paint_info_parent_class


static void
gimp_paint_info_class_init (GimpPaintInfoClass *klass)
{
  GObjectClass      *object_class   = G_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class = GIMP_VIEWABLE_CLASS (klass);

  object_class->dispose           = gimp_paint_info_dispose;
  object_class->finalize          = gimp_paint_info_finalize;

  viewable_class->get_description = gimp_paint_info_get_description;
}

static void
gimp_paint_info_init (GimpPaintInfo *paint_info)
{
  paint_info->gimp          = NULL;
  paint_info->paint_type    = G_TYPE_NONE;
  paint_info->blurb         = NULL;
  paint_info->paint_options = NULL;
}

static void
gimp_paint_info_dispose (GObject *object)
{
  GimpPaintInfo *paint_info = GIMP_PAINT_INFO (object);

  if (paint_info->paint_options)
    {
      g_object_run_dispose (G_OBJECT (paint_info->paint_options));
      g_clear_object (&paint_info->paint_options);
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_paint_info_finalize (GObject *object)
{
  GimpPaintInfo *paint_info = GIMP_PAINT_INFO (object);

  g_clear_pointer (&paint_info->blurb, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gchar *
gimp_paint_info_get_description (GimpViewable  *viewable,
                                 gchar        **tooltip)
{
  GimpPaintInfo *paint_info = GIMP_PAINT_INFO (viewable);

  return g_strdup (paint_info->blurb);
}

GimpPaintInfo *
gimp_paint_info_new (Gimp        *gimp,
                     GType        paint_type,
                     GType        paint_options_type,
                     const gchar *identifier,
                     const gchar *blurb,
                     const gchar *icon_name)
{
  GimpPaintInfo *paint_info;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (identifier != NULL, NULL);
  g_return_val_if_fail (blurb != NULL, NULL);
  g_return_val_if_fail (icon_name != NULL, NULL);

  paint_info = g_object_new (GIMP_TYPE_PAINT_INFO,
                             "name",      identifier,
                             "icon-name", icon_name,
                             NULL);

  paint_info->gimp               = gimp;
  paint_info->paint_type         = paint_type;
  paint_info->paint_options_type = paint_options_type;
  paint_info->blurb              = g_strdup (blurb);

  paint_info->paint_options      = gimp_paint_options_new (paint_info);

  return paint_info;
}

void
gimp_paint_info_set_standard (Gimp          *gimp,
                              GimpPaintInfo *paint_info)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (! paint_info || GIMP_IS_PAINT_INFO (paint_info));

  g_set_object (&gimp->standard_paint_info, paint_info);
}

GimpPaintInfo *
gimp_paint_info_get_standard (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return gimp->standard_paint_info;
}
