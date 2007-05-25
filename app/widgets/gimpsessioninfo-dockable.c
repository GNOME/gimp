/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsessioninfo-dockable.c
 * Copyright (C) 2001-2007 Michael Natterer <mitch@gimp.org>
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

#include "libgimpconfig/gimpconfig.h"

#include "widgets-types.h"

#include "gimpcontainerview.h"
#include "gimpcontainerview-utils.h"
#include "gimpdialogfactory.h"
#include "gimpdock.h"
#include "gimpdockable.h"
#include "gimpsessioninfo-aux.h"
#include "gimpsessioninfo-book.h"
#include "gimpsessioninfo-dockable.h"


enum
{
  SESSION_INFO_DOCKABLE_TAB_STYLE,
  SESSION_INFO_DOCKABLE_VIEW_SIZE,
  SESSION_INFO_DOCKABLE_AUX
};


/*  public functions  */

GimpSessionInfoDockable *
gimp_session_info_dockable_new (void)
{
  return g_slice_new0 (GimpSessionInfoDockable);
}

void
gimp_session_info_dockable_free (GimpSessionInfoDockable *info)
{
  g_return_if_fail (info != NULL);

  if (info->identifier)
    g_free (info->identifier);

  if (info->aux_info)
    {
      g_list_foreach (info->aux_info, (GFunc) gimp_session_info_aux_free, NULL);
      g_list_free (info->aux_info);
    }

  g_slice_free (GimpSessionInfoDockable, info);
}

void
gimp_session_info_dockable_serialize (GimpConfigWriter *writer,
                                      GimpDockable     *dockable)
{
  GimpDialogFactoryEntry *entry;

  g_return_if_fail (writer != NULL);
  g_return_if_fail (GIMP_IS_DOCKABLE (dockable));

  gimp_dialog_factory_from_widget (GTK_WIDGET (dockable), &entry);

  if (entry)
    {
      GimpContainerView *view;
      GEnumClass        *enum_class;
      GEnumValue        *enum_value;
      const gchar       *tab_style = "icon";
      gint               view_size = -1;

      enum_class = g_type_class_ref (GIMP_TYPE_TAB_STYLE);

      gimp_config_writer_open (writer, "dockable");
      gimp_config_writer_string (writer, entry->identifier);

      enum_value = g_enum_get_value (enum_class, dockable->tab_style);

      if (enum_value)
        tab_style = enum_value->value_nick;

      gimp_config_writer_open (writer, "tab-style");
      gimp_config_writer_print (writer, tab_style, -1);
      gimp_config_writer_close (writer);

      view = gimp_container_view_get_by_dockable (dockable);

      if (view)
        view_size = gimp_container_view_get_view_size (view, NULL);

      if (view_size > 0 &&
          view_size != entry->view_size)
        {
          gimp_config_writer_open (writer, "preview-size");
          gimp_config_writer_printf (writer, "%d", view_size);
          gimp_config_writer_close (writer);
        }

      gimp_session_info_aux_serialize (writer, GTK_WIDGET (dockable));

      gimp_config_writer_close (writer);

      g_type_class_unref (enum_class);
    }
}

GTokenType
gimp_session_info_dockable_deserialize (GScanner            *scanner,
                                        gint                 scope,
                                        GimpSessionInfoBook *book)
{
  GimpSessionInfoDockable *dockable;
  GEnumClass              *enum_class;
  GEnumValue              *enum_value;
  GTokenType               token;

  g_return_val_if_fail (scanner != NULL, G_TOKEN_LEFT_PAREN);
  g_return_val_if_fail (book != NULL, G_TOKEN_LEFT_PAREN);

  g_scanner_scope_add_symbol (scanner, scope, "tab-style",
                              GINT_TO_POINTER (SESSION_INFO_DOCKABLE_TAB_STYLE));
  g_scanner_scope_add_symbol (scanner, scope, "preview-size",
                              GINT_TO_POINTER (SESSION_INFO_DOCKABLE_VIEW_SIZE));
  g_scanner_scope_add_symbol (scanner, scope, "aux-info",
                              GINT_TO_POINTER (SESSION_INFO_DOCKABLE_AUX));

  dockable = gimp_session_info_dockable_new ();

  enum_class = g_type_class_ref (GIMP_TYPE_TAB_STYLE);

  token = G_TOKEN_STRING;
  if (! gimp_scanner_parse_string (scanner, &dockable->identifier))
    goto error;

  token = G_TOKEN_LEFT_PAREN;

  while (g_scanner_peek_next_token (scanner) == token)
    {
      token = g_scanner_get_next_token (scanner);

      switch (token)
        {
        case G_TOKEN_LEFT_PAREN:
          token = G_TOKEN_SYMBOL;
          break;

        case G_TOKEN_SYMBOL:
          switch (GPOINTER_TO_INT (scanner->value.v_symbol))
            {
            case SESSION_INFO_DOCKABLE_TAB_STYLE:
              token = G_TOKEN_IDENTIFIER;
              if (g_scanner_peek_next_token (scanner) != token)
                goto error;

              g_scanner_get_next_token (scanner);

              enum_value = g_enum_get_value_by_nick (enum_class,
                                                     scanner->value.v_identifier);

              if (! enum_value)
                enum_value = g_enum_get_value_by_name (enum_class,
                                                       scanner->value.v_identifier);

              if (enum_value)
                dockable->tab_style = enum_value->value;
              break;

            case SESSION_INFO_DOCKABLE_VIEW_SIZE:
              token = G_TOKEN_INT;
              if (! gimp_scanner_parse_int (scanner, &dockable->view_size))
                goto error;
              break;

            case SESSION_INFO_DOCKABLE_AUX:
              token = gimp_session_info_aux_deserialize (scanner,
                                                         &dockable->aux_info);
              if (token != G_TOKEN_LEFT_PAREN)
                goto error;
              break;

            default:
              goto error;
            }
          token = G_TOKEN_RIGHT_PAREN;
          break;

        case G_TOKEN_RIGHT_PAREN:
          token = G_TOKEN_LEFT_PAREN;
          break;

        default:
          break;
        }
    }

  book->dockables = g_list_append (book->dockables, dockable);
  g_type_class_unref (enum_class);

  g_scanner_scope_remove_symbol (scanner, scope, "tab-style");
  g_scanner_scope_remove_symbol (scanner, scope, "preview-size");
  g_scanner_scope_remove_symbol (scanner, scope, "aux-info");

  return token;

 error:
  gimp_session_info_dockable_free (dockable);
  g_type_class_unref (enum_class);

  return token;
}

GimpDockable *
gimp_session_info_dockable_restore (GimpSessionInfoDockable *info,
                                    GimpDock                *dock)
{
  GtkWidget *dockable;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_DOCK (dock), NULL);

  if (info->view_size < GIMP_VIEW_SIZE_TINY ||
      info->view_size > GIMP_VIEW_SIZE_GIGANTIC)
    info->view_size = -1;

  /*  use the new dock's dialog factory to create dockables
   *  because it may be different from the dialog factory
   *  the dock was created from.
   */
  dockable = gimp_dialog_factory_dockable_new (dock->dialog_factory,
                                               dock,
                                               info->identifier,
                                               info->view_size);

  if (dockable)
    {
      gimp_dockable_set_tab_style (GIMP_DOCKABLE (dockable), info->tab_style);

      if (info->aux_info)
        gimp_session_info_aux_set_list (dockable, info->aux_info);
    }

  return GIMP_DOCKABLE (dockable);
}
