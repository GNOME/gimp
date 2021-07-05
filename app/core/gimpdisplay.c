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

#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"

#include "gimpdisplay.h"

#include "gimp-intl.h"

enum
{
  PROP_0,
  PROP_ID,
  PROP_GIMP
};


struct _GimpDisplayPrivate
{
  gint id; /* unique identifier for this display */
};


/*  local function prototypes  */

static void     gimp_display_set_property           (GObject             *object,
                                                     guint                property_id,
                                                     const GValue        *value,
                                                     GParamSpec          *pspec);
static void     gimp_display_get_property           (GObject             *object,
                                                     guint                property_id,
                                                     GValue              *value,
                                                     GParamSpec          *pspec);


G_DEFINE_TYPE_WITH_PRIVATE (GimpDisplay, gimp_display, GIMP_TYPE_OBJECT)

#define parent_class gimp_display_parent_class


static void
gimp_display_class_init (GimpDisplayClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_display_set_property;
  object_class->get_property = gimp_display_get_property;

  g_object_class_install_property (object_class, PROP_ID,
                                   g_param_spec_int ("id",
                                                     NULL, NULL,
                                                     0, G_MAXINT, 0,
                                                     GIMP_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_GIMP,
                                   g_param_spec_object ("gimp",
                                                        NULL, NULL,
                                                        GIMP_TYPE_GIMP,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_display_init (GimpDisplay *display)
{
  display->priv = gimp_display_get_instance_private (display);
}

static void
gimp_display_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  GimpDisplay        *display = GIMP_DISPLAY (object);
  GimpDisplayPrivate *private = display->priv;

  switch (property_id)
    {
    case PROP_GIMP:
      {
        gint id;

        display->gimp = g_value_get_object (value); /* don't ref the gimp */
        display->config = GIMP_DISPLAY_CONFIG (display->gimp->config);

        do
          {
            id = display->gimp->next_display_id++;

            if (display->gimp->next_display_id == G_MAXINT)
              display->gimp->next_display_id = 1;
          }
        while (gimp_display_get_by_id (display->gimp, id));

        private->id = id;
      }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_display_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  GimpDisplay *display = GIMP_DISPLAY (object);

  switch (property_id)
    {
    case PROP_ID:
      g_value_set_int (value, display->priv->id);
      break;

    case PROP_GIMP:
      g_value_set_object (value, display->gimp);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/*  public functions  */

gint
gimp_display_get_id (GimpDisplay *display)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY (display), -1);

  return display->priv->id;
}

GimpDisplay *
gimp_display_get_by_id (Gimp *gimp,
                        gint  id)
{
  GList *list;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  for (list = gimp_get_display_iter (gimp);
       list;
       list = g_list_next (list))
    {
      GimpDisplay *display = list->data;

      if (gimp_display_get_id (display) == id)
        return display;
    }

  return NULL;
}

gboolean
gimp_display_present (GimpDisplay *display)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY (display), FALSE);

  if (GIMP_DISPLAY_GET_CLASS (display)->present)
    return GIMP_DISPLAY_GET_CLASS (display)->present (display);

  return FALSE;
}

gboolean
gimp_display_grab_focus (GimpDisplay *display)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY (display), FALSE);

  if (GIMP_DISPLAY_GET_CLASS (display)->grab_focus)
    return GIMP_DISPLAY_GET_CLASS (display)->grab_focus (display);

  return FALSE;
}

Gimp *
gimp_display_get_gimp (GimpDisplay *display)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY (display), NULL);

  return display->gimp;
}
