/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmadocked.h
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

#ifndef __LIGMA_DOCKED_H__
#define __LIGMA_DOCKED_H__


#define LIGMA_TYPE_DOCKED (ligma_docked_get_type ())
G_DECLARE_INTERFACE (LigmaDocked, ligma_docked, LIGMA, DOCKED, GtkWidget)


/**
 * LigmaDockedInterface:
 *
 * Interface with common methods for stuff that is docked.
 */
struct _LigmaDockedInterface
{
  GTypeInterface base_iface;

  /*  signals  */
  void            (* title_changed)       (LigmaDocked   *docked);

  /*  virtual functions  */
  void            (* set_aux_info)        (LigmaDocked   *docked,
                                           GList        *aux_info);
  GList         * (* get_aux_info)        (LigmaDocked   *docked);

  GtkWidget     * (* get_preview)         (LigmaDocked   *docked,
                                           LigmaContext  *context,
                                           GtkIconSize   size);
  gboolean        (* get_prefer_icon)     (LigmaDocked   *docked);
  LigmaUIManager * (* get_menu)            (LigmaDocked   *docked,
                                           const gchar **ui_path,
                                           gpointer     *popup_data);
  gchar         * (* get_title)           (LigmaDocked   *docked);

  void            (* set_context)         (LigmaDocked   *docked,
                                           LigmaContext  *context);

  gboolean        (* has_button_bar)      (LigmaDocked   *docked);
  void            (* set_show_button_bar) (LigmaDocked   *docked,
                                           gboolean      show);
  gboolean        (* get_show_button_bar) (LigmaDocked   *docked);
};


void            ligma_docked_title_changed       (LigmaDocked   *docked);

void            ligma_docked_set_aux_info        (LigmaDocked   *docked,
                                                 GList        *aux_info);
GList         * ligma_docked_get_aux_info        (LigmaDocked   *docked);

GtkWidget     * ligma_docked_get_preview         (LigmaDocked   *docked,
                                                 LigmaContext  *context,
                                                 GtkIconSize   size);
gboolean        ligma_docked_get_prefer_icon     (LigmaDocked   *docked);
LigmaUIManager * ligma_docked_get_menu            (LigmaDocked   *docked,
                                                 const gchar **ui_path,
                                                 gpointer     *popup_data);
gchar         * ligma_docked_get_title           (LigmaDocked   *docked);

void            ligma_docked_set_context         (LigmaDocked   *docked,
                                                 LigmaContext  *context);

gboolean        ligma_docked_has_button_bar      (LigmaDocked   *docked);
void            ligma_docked_set_show_button_bar (LigmaDocked   *docked,
                                                 gboolean      show);
gboolean        ligma_docked_get_show_button_bar (LigmaDocked   *docked);


#endif  /* __LIGMA_DOCKED_H__ */
