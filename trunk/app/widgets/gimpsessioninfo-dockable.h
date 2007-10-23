/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsessioninfo-dockable.h
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

#ifndef __GIMP_SESSION_INFO_DOCKABLE_H__
#define __GIMP_SESSION_INFO_DOCKABLE_H__


struct _GimpSessionInfoDockable
{
  gchar        *identifier;
  GimpTabStyle  tab_style;
  gint          view_size;

  /*  dialog specific list of GimpSessionInfoAux  */
  GList        *aux_info;
};


GimpSessionInfoDockable *
               gimp_session_info_dockable_new         (void);
void           gimp_session_info_dockable_free        (GimpSessionInfoDockable *info);

void           gimp_session_info_dockable_serialize   (GimpConfigWriter        *writer,
                                                       GimpDockable            *dockable);
GTokenType     gimp_session_info_dockable_deserialize (GScanner                *scanner,
                                                       gint                     scope,
                                                       GimpSessionInfoBook     *book);

GimpDockable * gimp_session_info_dockable_restore     (GimpSessionInfoDockable *info,
                                                       GimpDock                *dock);


#endif  /* __GIMP_SESSION_INFO_DOCKABLE_H__ */
