/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmasessionmanaged.c
 * Copyright (C) 2011 Martin Nordholts <martinn@src.gnome.org>
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

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "ligmasessionmanaged.h"


G_DEFINE_INTERFACE (LigmaSessionManaged, ligma_session_managed, GTK_TYPE_WIDGET)


/*  private functions  */


static void
ligma_session_managed_default_init (LigmaSessionManagedInterface *iface)
{
}


/*  public functions  */


/**
 * ligma_session_managed_get_aux_info:
 * @session_managed: A #LigmaSessionManaged
 *
 * Returns: A list of #LigmaSessionInfoAux created with
 *          ligma_session_info_aux_new().
 **/
GList *
ligma_session_managed_get_aux_info (LigmaSessionManaged *session_managed)
{
  LigmaSessionManagedInterface *iface;

  g_return_val_if_fail (LIGMA_IS_SESSION_MANAGED (session_managed), NULL);

  iface = LIGMA_SESSION_MANAGED_GET_IFACE (session_managed);

  if (iface->get_aux_info)
    return iface->get_aux_info (session_managed);

  return NULL;
}

/**
 * ligma_session_managed_get_ui_manager:
 * @session_managed: A #LigmaSessionManaged
 * @aux_info         A list of #LigmaSessionInfoAux
 *
 * Sets aux data previously returned from
 * ligma_session_managed_get_aux_info().
 **/
void
ligma_session_managed_set_aux_info (LigmaSessionManaged *session_managed,
                                   GList              *aux_info)
{
  LigmaSessionManagedInterface *iface;

  g_return_if_fail (LIGMA_IS_SESSION_MANAGED (session_managed));

  iface = LIGMA_SESSION_MANAGED_GET_IFACE (session_managed);

  if (iface->set_aux_info)
    iface->set_aux_info (session_managed, aux_info);
}
