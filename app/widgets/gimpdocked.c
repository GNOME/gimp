/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdocked.c
 * Copyright (C) 2003  Michael Natterer <mitch@gimp.org>
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

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "core/gimpcontext.h"

#include "gimpdocked.h"


static void  gimp_docked_iface_init (GimpDockedInterface *docked_iface);


GType
gimp_docked_interface_get_type (void)
{
  static GType docked_iface_type = 0;

  if (!docked_iface_type)
    {
      static const GTypeInfo docked_iface_info =
      {
        sizeof (GimpDockedInterface),
	(GBaseInitFunc)     gimp_docked_iface_init,
	(GBaseFinalizeFunc) NULL,
      };

      docked_iface_type = g_type_register_static (G_TYPE_INTERFACE,
                                                  "GimpDockedInterface",
                                                  &docked_iface_info,
                                                  0);

      g_type_interface_add_prerequisite (docked_iface_type, GTK_TYPE_WIDGET);
    }

  return docked_iface_type;
}

static void
gimp_docked_iface_init (GimpDockedInterface *docked_iface)
{
  docked_iface->set_aux_info = NULL;
  docked_iface->get_aux_info = NULL;
  docked_iface->get_preview  = NULL;
  docked_iface->set_context  = NULL;
  docked_iface->get_menu     = NULL;
}

void
gimp_docked_set_aux_info (GimpDocked *docked,
                          GList      *aux_info)
{
  GimpDockedInterface *docked_iface;

  g_return_if_fail (GIMP_IS_DOCKED (docked));

  docked_iface = GIMP_DOCKED_GET_INTERFACE (docked);

  if (docked_iface->set_aux_info)
    docked_iface->set_aux_info (docked, aux_info);
}

GList *
gimp_docked_get_aux_info (GimpDocked *docked)
{
  GimpDockedInterface *docked_iface;

  g_return_val_if_fail (GIMP_IS_DOCKED (docked), NULL);

  docked_iface = GIMP_DOCKED_GET_INTERFACE (docked);

  if (docked_iface->get_aux_info)
    return docked_iface->get_aux_info (docked);

  return NULL;
}

GtkWidget *
gimp_docked_get_preview (GimpDocked  *docked,
                         GimpContext *context,
                         GtkIconSize  size)
{
  GimpDockedInterface *docked_iface;

  g_return_val_if_fail (GIMP_IS_DOCKED (docked), NULL);

  docked_iface = GIMP_DOCKED_GET_INTERFACE (docked);

  if (docked_iface->get_preview)
    return docked_iface->get_preview (docked, context, size);

  return NULL;
}

void
gimp_docked_set_context (GimpDocked  *docked,
                         GimpContext *context,
                         GimpContext *prev_context)
{
  GimpDockedInterface *docked_iface;

  g_return_if_fail (GIMP_IS_DOCKED (docked));
  g_return_if_fail (context == NULL || GIMP_IS_CONTEXT (context));
  g_return_if_fail (prev_context == NULL || GIMP_IS_CONTEXT (prev_context));

  docked_iface = GIMP_DOCKED_GET_INTERFACE (docked);

  if (docked_iface->set_context)
    docked_iface->set_context (docked, context, prev_context);
}

GimpItemFactory *
gimp_docked_get_menu (GimpDocked *docked,
                      gpointer   *item_factory_data)
{
  GimpDockedInterface *docked_iface;

  g_return_val_if_fail (GIMP_IS_DOCKED (docked), NULL);
  g_return_val_if_fail (item_factory_data != NULL, NULL);

  docked_iface = GIMP_DOCKED_GET_INTERFACE (docked);

  if (docked_iface->get_menu)
    return docked_iface->get_menu (docked, item_factory_data);

  return NULL;
}
