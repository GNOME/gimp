/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmasessioninfo-dockable.h
 * Copyright (C) 2001-2007 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_SESSION_INFO_DOCKABLE_H__
#define __LIGMA_SESSION_INFO_DOCKABLE_H__


/**
 * LigmaSessionInfoDockable:
 *
 * Contains information about a dockable in the interface.
 */
struct _LigmaSessionInfoDockable
{
  gchar        *identifier;
  gboolean      locked;
  LigmaTabStyle  tab_style;
  gint          view_size;

  /*  dialog specific list of LigmaSessionInfoAux  */
  GList        *aux_info;
};


LigmaSessionInfoDockable *
               ligma_session_info_dockable_new         (void);
void           ligma_session_info_dockable_free        (LigmaSessionInfoDockable  *info);

void           ligma_session_info_dockable_serialize   (LigmaConfigWriter         *writer,
                                                       LigmaSessionInfoDockable  *dockable);
GTokenType     ligma_session_info_dockable_deserialize (GScanner                 *scanner,
                                                       gint                      scope,
                                                       LigmaSessionInfoDockable **dockable);

LigmaSessionInfoDockable *
               ligma_session_info_dockable_from_widget (LigmaDockable             *dockable);

LigmaDockable * ligma_session_info_dockable_restore     (LigmaSessionInfoDockable  *info,
                                                       LigmaDock                 *dock);


#endif  /* __LIGMA_SESSION_INFO_DOCKABLE_H__ */
