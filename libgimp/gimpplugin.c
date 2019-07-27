/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpplugin.c
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

#include <string.h>

#include "gimp.h"


/**
 * SECTION: gimpplugin
 * @title: GimpPlugIn
 * @short_description: The base class for plug-ins to derive from
 *
 * The base class for plug-ins to derive from. Manages the plug-in's
 * #GimpProcedure objects.
 *
 * Since: 3.0
 **/


struct _GimpPlugInPrivate
{
  GList *procedures;
};


static void   gimp_plug_in_finalize (GObject *object);


G_DEFINE_TYPE_WITH_PRIVATE (GimpPlugIn, gimp_plug_in, G_TYPE_OBJECT)

#define parent_class gimp_plug_in_parent_class


static void
gimp_plug_in_class_init (GimpPlugInClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gimp_plug_in_finalize;
}

static void
gimp_plug_in_init (GimpPlugIn *plug_in)
{
  plug_in->priv = gimp_plug_in_get_instance_private (plug_in);
}

static void
gimp_plug_in_finalize (GObject *object)
{
  GimpPlugIn *plug_in = GIMP_PLUG_IN (object);

  if (plug_in->priv->procedures)
    {
      g_list_free_full (plug_in->priv->procedures, g_object_unref);
      plug_in->priv->procedures = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


/*  public functions  */

GimpProcedure *
gimp_plug_in_create_procedure (GimpPlugIn    *plug_in,
                               const gchar   *name)
{
  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  if (GIMP_PLUG_IN_GET_CLASS (plug_in)->create_procedure)
    return GIMP_PLUG_IN_GET_CLASS (plug_in)->create_procedure (plug_in,
                                                               name);

  return NULL;
}

void
gimp_plug_in_add_procedure (GimpPlugIn    *plug_in,
                            GimpProcedure *procedure)
{
  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));

  plug_in->priv->procedures = g_list_prepend (plug_in->priv->procedures,
                                              g_object_ref (procedure));
}

void
gimp_plug_in_remove_procedure (GimpPlugIn  *plug_in,
                               const gchar *name)
{
  GimpProcedure *procedure;

  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));
  g_return_if_fail (name != NULL);

  procedure = gimp_plug_in_get_procedure (plug_in, name);

  if (procedure)
    {
      plug_in->priv->procedures = g_list_remove (plug_in->priv->procedures,
                                                 procedure);
      g_object_unref (procedure);
    }
}

GList *
gimp_plug_in_get_procedures (GimpPlugIn *plug_in)
{
  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), NULL);

  return plug_in->priv->procedures;
}

GimpProcedure *
gimp_plug_in_get_procedure (GimpPlugIn  *plug_in,
                            const gchar *name)
{
  GList *list;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  for (list = plug_in->priv->procedures; list; list = g_list_next (list))
    {
      GimpProcedure *procedure = list->data;

      if (! strcmp (name, gimp_procedure_get_name (procedure)))
        return procedure;
    }

  return NULL;
}


/*  unrelated old API  */

gboolean
gimp_plugin_icon_register (const gchar  *procedure_name,
                           GimpIconType  icon_type,
                           const guint8 *icon_data)
{
  gint icon_data_length;

  g_return_val_if_fail (procedure_name != NULL, FALSE);
  g_return_val_if_fail (icon_data != NULL, FALSE);

  switch (icon_type)
    {
    case GIMP_ICON_TYPE_ICON_NAME:
    case GIMP_ICON_TYPE_IMAGE_FILE:
      icon_data_length = strlen ((const gchar *) icon_data) + 1;
      break;

    case GIMP_ICON_TYPE_INLINE_PIXBUF:
      g_return_val_if_fail (g_ntohl (*((gint32 *) icon_data)) == 0x47646b50,
                            FALSE);

      icon_data_length = g_ntohl (*((gint32 *) (icon_data + 4)));
      break;

    default:
      g_return_val_if_reached (FALSE);
    }

  return _gimp_plugin_icon_register (procedure_name,
                                     icon_type, icon_data_length, icon_data);
}
