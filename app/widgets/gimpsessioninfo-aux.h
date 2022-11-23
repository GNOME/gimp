/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmasessioninfo-aux.h
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

#ifndef __LIGMA_SESSION_INFO_AUX_H__
#define __LIGMA_SESSION_INFO_AUX_H__


/**
 * LigmaSessionInfoAux:
 *
 * Contains arbitrary data in the session management system, used for
 * example by dockables to manage dockable-specific data.
 */
struct _LigmaSessionInfoAux
{
  gchar *name;
  gchar *value;
};


LigmaSessionInfoAux *
             ligma_session_info_aux_new            (const gchar         *name,
                                                   const gchar         *value);
void         ligma_session_info_aux_free           (LigmaSessionInfoAux  *aux);

GList      * ligma_session_info_aux_new_from_props (GObject             *object,
                                                   ...) G_GNUC_NULL_TERMINATED;
void         ligma_session_info_aux_set_props      (GObject             *object,
                                                   GList               *aux,
                                                   ...) G_GNUC_NULL_TERMINATED;

void         ligma_session_info_aux_serialize      (LigmaConfigWriter    *writer,
                                                   GList               *aux_info);
GTokenType   ligma_session_info_aux_deserialize    (GScanner            *scanner,
                                                   GList              **aux_list);


#endif  /* __LIGMA_SESSION_INFO_AUX_H__ */
