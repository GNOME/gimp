/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpdocked.h
 * Copyright (C) 2003  Michael Natterer <mitch@gimp.org>
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

#pragma once


#define GIMP_TYPE_DOCKED (gimp_docked_get_type ())
G_DECLARE_INTERFACE (GimpDocked,
                     gimp_docked,
                     GIMP, DOCKED,
                     GtkWidget)


/**
 * GimpDockedInterface:
 *
 * Interface with common methods for stuff that is docked.
 */
struct _GimpDockedInterface
{
  GTypeInterface base_iface;

  /*  signals  */
  void            (* title_changed)       (GimpDocked   *docked);

  /*  virtual functions  */
  void            (* set_aux_info)        (GimpDocked   *docked,
                                           GList        *aux_info);
  GList         * (* get_aux_info)        (GimpDocked   *docked);

  GtkWidget     * (* get_preview)         (GimpDocked   *docked,
                                           GimpContext  *context,
                                           GtkIconSize   size);
  gboolean        (* get_prefer_icon)     (GimpDocked   *docked);
  GimpUIManager * (* get_menu)            (GimpDocked   *docked,
                                           const gchar **ui_path,
                                           gpointer     *popup_data);
  gchar         * (* get_title)           (GimpDocked   *docked);

  void            (* set_context)         (GimpDocked   *docked,
                                           GimpContext  *context);

  gboolean        (* has_button_bar)      (GimpDocked   *docked);
  void            (* set_show_button_bar) (GimpDocked   *docked,
                                           gboolean      show);
  gboolean        (* get_show_button_bar) (GimpDocked   *docked);
};


void            gimp_docked_title_changed       (GimpDocked   *docked);

void            gimp_docked_set_aux_info        (GimpDocked   *docked,
                                                 GList        *aux_info);
GList         * gimp_docked_get_aux_info        (GimpDocked   *docked);

GtkWidget     * gimp_docked_get_preview         (GimpDocked   *docked,
                                                 GimpContext  *context,
                                                 GtkIconSize   size);
gboolean        gimp_docked_get_prefer_icon     (GimpDocked   *docked);
GimpUIManager * gimp_docked_get_menu            (GimpDocked   *docked,
                                                 const gchar **ui_path,
                                                 gpointer     *popup_data);
gchar         * gimp_docked_get_title           (GimpDocked   *docked);

void            gimp_docked_set_context         (GimpDocked   *docked,
                                                 GimpContext  *context);

gboolean        gimp_docked_has_button_bar      (GimpDocked   *docked);
void            gimp_docked_set_show_button_bar (GimpDocked   *docked,
                                                 gboolean      show);
gboolean        gimp_docked_get_show_button_bar (GimpDocked   *docked);
