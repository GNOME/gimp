/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmasessioninfo-dock.h
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

#ifndef __LIGMA_SESSION_INFO_DOCK_H__
#define __LIGMA_SESSION_INFO_DOCK_H__


/**
 * LigmaSessionInfoDock:
 *
 * Contains information about a dock in the interface.
 */
struct _LigmaSessionInfoDock
{
  /* Type of dock, written to/read from sessionrc. E.g. 'ligma-dock' or
   * 'ligma-toolbox'
   */
  gchar             *dock_type;

  /* What side this dock is in in single-window mode. Either
   * LIGMA_ALIGN_LEFT, LIGMA_ALIGN_RIGHT or -1.
   */
  LigmaAlignmentType  side;

  /* GtkPaned position of this dock */
  gint               position;

  /*  list of LigmaSessionInfoBook  */
  GList             *books;
};

LigmaSessionInfoDock * ligma_session_info_dock_new         (const gchar          *dock_type);
void                  ligma_session_info_dock_free        (LigmaSessionInfoDock  *dock_info);
void                  ligma_session_info_dock_serialize   (LigmaConfigWriter     *writer,
                                                          LigmaSessionInfoDock  *dock);
GTokenType            ligma_session_info_dock_deserialize (GScanner             *scanner,
                                                          gint                  scope,
                                                          LigmaSessionInfoDock **info,
                                                          const gchar          *dock_type);
LigmaSessionInfoDock * ligma_session_info_dock_from_widget (LigmaDock             *dock);
LigmaDock            * ligma_session_info_dock_restore     (LigmaSessionInfoDock  *dock_info,
                                                          LigmaDialogFactory    *factory,
                                                          GdkMonitor           *monitor,
                                                          LigmaDockContainer    *dock_container);


#endif  /* __LIGMA_SESSION_INFO_DOCK_H__ */
