/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadocked.c
 * Copyright (C) 2003  Michael Natterer <mitch@ligma.org>
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "widgets-types.h"

#include "core/ligmacontext.h"

#include "ligmadocked.h"
#include "ligmasessioninfo-aux.h"


enum
{
  TITLE_CHANGED,
  LAST_SIGNAL
};


/*  local function prototypes  */

static void    ligma_docked_iface_set_aux_info (LigmaDocked *docked,
                                               GList      *aux_info);
static GList * ligma_docked_iface_get_aux_info (LigmaDocked *docked);


G_DEFINE_INTERFACE (LigmaDocked, ligma_docked, GTK_TYPE_WIDGET)


static guint docked_signals[LAST_SIGNAL] = { 0 };


/*  private functions  */


static void
ligma_docked_default_init (LigmaDockedInterface *iface)
{
  docked_signals[TITLE_CHANGED] =
    g_signal_new ("title-changed",
                  LIGMA_TYPE_DOCKED,
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaDockedInterface, title_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  iface->get_aux_info = ligma_docked_iface_get_aux_info;
  iface->set_aux_info = ligma_docked_iface_set_aux_info;
}

#define AUX_INFO_SHOW_BUTTON_BAR "show-button-bar"

static void
ligma_docked_iface_set_aux_info (LigmaDocked *docked,
                                GList      *aux_info)
{
  GList *list;

  for (list = aux_info; list; list = g_list_next (list))
    {
      LigmaSessionInfoAux *aux = list->data;

      if (strcmp (aux->name, AUX_INFO_SHOW_BUTTON_BAR) == 0)
        {
          gboolean show = g_ascii_strcasecmp (aux->value, "false");

          ligma_docked_set_show_button_bar (docked, show);
        }
    }
}

static GList *
ligma_docked_iface_get_aux_info (LigmaDocked *docked)
{
  if (ligma_docked_has_button_bar (docked))
    {
      gboolean show = ligma_docked_get_show_button_bar (docked);

      return g_list_append (NULL,
                            ligma_session_info_aux_new (AUX_INFO_SHOW_BUTTON_BAR,
                                                       show ? "true" : "false"));
    }

  return NULL;
}


/*  public functions  */


void
ligma_docked_title_changed (LigmaDocked *docked)
{
  g_return_if_fail (LIGMA_IS_DOCKED (docked));

  g_signal_emit (docked, docked_signals[TITLE_CHANGED], 0);
}

void
ligma_docked_set_aux_info (LigmaDocked *docked,
                          GList      *aux_info)
{
  LigmaDockedInterface *docked_iface;

  g_return_if_fail (LIGMA_IS_DOCKED (docked));

  docked_iface = LIGMA_DOCKED_GET_IFACE (docked);

  if (docked_iface->set_aux_info)
    docked_iface->set_aux_info (docked, aux_info);
}

GList *
ligma_docked_get_aux_info (LigmaDocked *docked)
{
  LigmaDockedInterface *docked_iface;

  g_return_val_if_fail (LIGMA_IS_DOCKED (docked), NULL);

  docked_iface = LIGMA_DOCKED_GET_IFACE (docked);

  if (docked_iface->get_aux_info)
    return docked_iface->get_aux_info (docked);

  return NULL;
}

GtkWidget *
ligma_docked_get_preview (LigmaDocked  *docked,
                         LigmaContext *context,
                         GtkIconSize  size)
{
  LigmaDockedInterface *docked_iface;

  g_return_val_if_fail (LIGMA_IS_DOCKED (docked), NULL);

  docked_iface = LIGMA_DOCKED_GET_IFACE (docked);

  if (docked_iface->get_preview)
    return docked_iface->get_preview (docked, context, size);

  return NULL;
}

gboolean
ligma_docked_get_prefer_icon (LigmaDocked *docked)
{
  LigmaDockedInterface *docked_iface;

  g_return_val_if_fail (LIGMA_IS_DOCKED (docked), FALSE);

  docked_iface = LIGMA_DOCKED_GET_IFACE (docked);

  if (docked_iface->get_prefer_icon)
    return docked_iface->get_prefer_icon (docked);

  return FALSE;
}

LigmaUIManager *
ligma_docked_get_menu (LigmaDocked     *docked,
                      const gchar   **ui_path,
                      gpointer       *popup_data)
{
  LigmaDockedInterface *docked_iface;

  g_return_val_if_fail (LIGMA_IS_DOCKED (docked), NULL);
  g_return_val_if_fail (ui_path != NULL, NULL);
  g_return_val_if_fail (popup_data != NULL, NULL);

  docked_iface = LIGMA_DOCKED_GET_IFACE (docked);

  if (docked_iface->get_menu)
    return docked_iface->get_menu (docked, ui_path, popup_data);

  return NULL;
}

gchar *
ligma_docked_get_title (LigmaDocked *docked)
{
  LigmaDockedInterface *docked_iface;

  g_return_val_if_fail (LIGMA_IS_DOCKED (docked), NULL);

  docked_iface = LIGMA_DOCKED_GET_IFACE (docked);

  if (docked_iface->get_title)
    return docked_iface->get_title (docked);

  return NULL;
}

void
ligma_docked_set_context (LigmaDocked  *docked,
                         LigmaContext *context)
{
  LigmaDockedInterface *docked_iface;

  g_return_if_fail (LIGMA_IS_DOCKED (docked));
  g_return_if_fail (context == NULL || LIGMA_IS_CONTEXT (context));

  docked_iface = LIGMA_DOCKED_GET_IFACE (docked);

  if (docked_iface->set_context)
    docked_iface->set_context (docked, context);
}

gboolean
ligma_docked_has_button_bar (LigmaDocked *docked)
{
  LigmaDockedInterface *docked_iface;

  g_return_val_if_fail (LIGMA_IS_DOCKED (docked), FALSE);

  docked_iface = LIGMA_DOCKED_GET_IFACE (docked);

  if (docked_iface->has_button_bar)
    return docked_iface->has_button_bar (docked);

  return FALSE;
}

void
ligma_docked_set_show_button_bar (LigmaDocked *docked,
                                 gboolean    show)
{
  LigmaDockedInterface *docked_iface;

  g_return_if_fail (LIGMA_IS_DOCKED (docked));

  docked_iface = LIGMA_DOCKED_GET_IFACE (docked);

  if (docked_iface->set_show_button_bar)
    docked_iface->set_show_button_bar (docked, show ? TRUE : FALSE);
}

gboolean
ligma_docked_get_show_button_bar (LigmaDocked *docked)
{
  LigmaDockedInterface *docked_iface;

  g_return_val_if_fail (LIGMA_IS_DOCKED (docked), FALSE);

  docked_iface = LIGMA_DOCKED_GET_IFACE (docked);

  if (docked_iface->get_show_button_bar)
    return docked_iface->get_show_button_bar (docked);

  return FALSE;
}
