/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * ligmadisplay.c
 * Copyright (C) Jehan
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "ligma.h"

#include "libligmabase/ligmawire.h" /* FIXME kill this include */

#include "ligmaplugin-private.h"
#include "ligmaprocedure-private.h"


enum
{
  PROP_0,
  PROP_ID,
  N_PROPS
};


struct _LigmaDisplayPrivate
{
  gint id;
};


static void   ligma_display_set_property  (GObject      *object,
                                          guint         property_id,
                                          const GValue *value,
                                          GParamSpec   *pspec);
static void   ligma_display_get_property  (GObject      *object,
                                          guint         property_id,
                                          GValue       *value,
                                          GParamSpec   *pspec);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaDisplay, ligma_display, G_TYPE_OBJECT)

#define parent_class ligma_display_parent_class

static GParamSpec *props[N_PROPS] = { NULL, };


static void
ligma_display_class_init (LigmaDisplayClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = ligma_display_set_property;
  object_class->get_property = ligma_display_get_property;

  props[PROP_ID] =
    g_param_spec_int ("id",
                      "The display id",
                      "The display id for internal use",
                      0, G_MAXINT32, 0,
                      LIGMA_PARAM_READWRITE |
                      G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, N_PROPS, props);
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
  LigmaDisplay *display = LIGMA_DISPLAY (object);

  switch (property_id)
    {
    case PROP_ID:
      display->priv->id = g_value_get_int (value);
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

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/* Public API */

/**
 * ligma_display_get_id:
 * @display: The display.
 *
 * Returns: the display ID.
 *
 * Since: 3.0
 **/
gint32
ligma_display_get_id (LigmaDisplay *display)
{
  return display ? display->priv->id : -1;
}

/**
 * ligma_display_get_by_id:
 * @display_id: The display id.
 *
 * Returns a #LigmaDisplay representing @display_id.
 *
 * Returns: (nullable) (transfer none): a #LigmaDisplay for @display_id or
 *          %NULL if @display_id does not represent a valid display.
 *          The object belongs to libligma and you must not modify or
 *          unref it.
 *
 * Since: 3.0
 **/
LigmaDisplay *
ligma_display_get_by_id (gint32 display_id)
{
  if (display_id > 0)
    {
      LigmaPlugIn    *plug_in   = ligma_get_plug_in ();
      LigmaProcedure *procedure = _ligma_plug_in_get_procedure (plug_in);

      return _ligma_procedure_get_display (procedure, display_id);
    }

  return NULL;
}

/**
 * ligma_display_is_valid:
 * @display: The display to check.
 *
 * Returns TRUE if the display is valid.
 *
 * This procedure checks if the given display is valid and refers to
 * an existing display.
 *
 * Returns: Whether the display is valid.
 *
 * Since: 2.4
 **/
gboolean
ligma_display_is_valid (LigmaDisplay *display)
{
  return ligma_display_id_is_valid (ligma_display_get_id (display));
}
