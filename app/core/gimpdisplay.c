/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"

#include "core-types.h"

#include "config/ligmaguiconfig.h"

#include "core/ligma.h"
#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"

#include "ligmadisplay.h"

#include "ligma-intl.h"

enum
{
  PROP_0,
  PROP_ID,
  PROP_LIGMA
};


struct _LigmaDisplayPrivate
{
  gint id; /* unique identifier for this display */
};


/*  local function prototypes  */

static void     ligma_display_set_property           (GObject             *object,
                                                     guint                property_id,
                                                     const GValue        *value,
                                                     GParamSpec          *pspec);
static void     ligma_display_get_property           (GObject             *object,
                                                     guint                property_id,
                                                     GValue              *value,
                                                     GParamSpec          *pspec);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaDisplay, ligma_display, LIGMA_TYPE_OBJECT)

#define parent_class ligma_display_parent_class


static void
ligma_display_class_init (LigmaDisplayClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = ligma_display_set_property;
  object_class->get_property = ligma_display_get_property;

  g_object_class_install_property (object_class, PROP_ID,
                                   g_param_spec_int ("id",
                                                     NULL, NULL,
                                                     0, G_MAXINT, 0,
                                                     LIGMA_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_LIGMA,
                                   g_param_spec_object ("ligma",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_LIGMA,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_display_init (LigmaDisplay *display)
{
  display->priv = ligma_display_get_instance_private (display);
}

static void
ligma_display_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  LigmaDisplay        *display = LIGMA_DISPLAY (object);
  LigmaDisplayPrivate *private = display->priv;

  switch (property_id)
    {
    case PROP_LIGMA:
      {
        gint id;

        display->ligma = g_value_get_object (value); /* don't ref the ligma */
        display->config = LIGMA_DISPLAY_CONFIG (display->ligma->config);

        do
          {
            id = display->ligma->next_display_id++;

            if (display->ligma->next_display_id == G_MAXINT)
              display->ligma->next_display_id = 1;
          }
        while (ligma_display_get_by_id (display->ligma, id));

        private->id = id;
      }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_display_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  LigmaDisplay *display = LIGMA_DISPLAY (object);

  switch (property_id)
    {
    case PROP_ID:
      g_value_set_int (value, display->priv->id);
      break;

    case PROP_LIGMA:
      g_value_set_object (value, display->ligma);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/*  public functions  */

gint
ligma_display_get_id (LigmaDisplay *display)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY (display), -1);

  return display->priv->id;
}

LigmaDisplay *
ligma_display_get_by_id (Ligma *ligma,
                        gint  id)
{
  GList *list;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  for (list = ligma_get_display_iter (ligma);
       list;
       list = g_list_next (list))
    {
      LigmaDisplay *display = list->data;

      if (ligma_display_get_id (display) == id)
        return display;
    }

  return NULL;
}

gboolean
ligma_display_present (LigmaDisplay *display)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY (display), FALSE);

  if (LIGMA_DISPLAY_GET_CLASS (display)->present)
    return LIGMA_DISPLAY_GET_CLASS (display)->present (display);

  return FALSE;
}

gboolean
ligma_display_grab_focus (LigmaDisplay *display)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY (display), FALSE);

  if (LIGMA_DISPLAY_GET_CLASS (display)->grab_focus)
    return LIGMA_DISPLAY_GET_CLASS (display)->grab_focus (display);

  return FALSE;
}

Ligma *
ligma_display_get_ligma (LigmaDisplay *display)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY (display), NULL);

  return display->ligma;
}
