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
#include "core/gimpmarshal.h"

#include "gimpdocked.h"


enum
{
  TITLE_CHANGED,
  LAST_SIGNAL
};


static void  gimp_docked_iface_base_init (GimpDockedInterface *docked_iface);


static guint docked_signals[LAST_SIGNAL] = { 0 };


GType
gimp_docked_interface_get_type (void)
{
  static GType docked_iface_type = 0;

  if (!docked_iface_type)
    {
      static const GTypeInfo docked_iface_info =
      {
        sizeof (GimpDockedInterface),
	(GBaseInitFunc)     gimp_docked_iface_base_init,
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
gimp_docked_iface_base_init (GimpDockedInterface *docked_iface)
{
  static gboolean initialized = FALSE;

  if (! initialized)
    {
      docked_signals[TITLE_CHANGED] =
        g_signal_new ("title_changed",
                      GIMP_TYPE_DOCKED,
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GimpDockedInterface, title_changed),
                      NULL, NULL,
                      gimp_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

      initialized = TRUE;
    }
}

void
gimp_docked_title_changed (GimpDocked *docked)
{
  g_return_if_fail (GIMP_IS_DOCKED (docked));

  g_signal_emit (docked, docked_signals[TITLE_CHANGED], 0);
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

GimpUIManager *
gimp_docked_get_menu (GimpDocked     *docked,
                      const gchar   **ui_path,
                      gpointer       *popup_data)
{
  GimpDockedInterface *docked_iface;

  g_return_val_if_fail (GIMP_IS_DOCKED (docked), NULL);
  g_return_val_if_fail (ui_path != NULL, NULL);
  g_return_val_if_fail (popup_data != NULL, NULL);

  docked_iface = GIMP_DOCKED_GET_INTERFACE (docked);

  if (docked_iface->get_menu)
    return docked_iface->get_menu (docked, ui_path, popup_data);

  return NULL;
}

gchar *
gimp_docked_get_title (GimpDocked *docked)
{
  GimpDockedInterface *docked_iface;

  g_return_val_if_fail (GIMP_IS_DOCKED (docked), NULL);

  docked_iface = GIMP_DOCKED_GET_INTERFACE (docked);

  if (docked_iface->get_title)
    return docked_iface->get_title (docked);

  return NULL;
}

void
gimp_docked_set_context (GimpDocked  *docked,
                         GimpContext *context)
{
  GimpDockedInterface *docked_iface;

  g_return_if_fail (GIMP_IS_DOCKED (docked));
  g_return_if_fail (context == NULL || GIMP_IS_CONTEXT (context));

  docked_iface = GIMP_DOCKED_GET_INTERFACE (docked);

  if (docked_iface->set_context)
    docked_iface->set_context (docked, context);
}
