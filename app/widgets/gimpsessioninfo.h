/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsessioninfo.h
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

#ifndef __GIMP_SESSION_INFO_H__
#define __GIMP_SESSION_INFO_H__


struct _GimpSessionInfo
{
  gint       x;
  gint       y;
  gint       width;
  gint       height;
  gboolean   open;   /*  only valid while restoring and saving the session  */
  gint       screen; /*  only valid while restoring and saving the session  */

  /*  dialog specific list of GimpSessionInfoAux  */
  GList     *aux_info;

  GtkWidget *widget;

  /*  only one of these is valid  */
  GimpDialogFactoryEntry *toplevel_entry;
  GimpDialogFactoryEntry *dockable_entry;

  /*  list of GimpSessionInfoBook  */
  GList     *books;
};


GimpSessionInfo * gimp_session_info_new           (void);
void              gimp_session_info_free          (GimpSessionInfo   *info);

void              gimp_session_info_serialize     (GimpConfigWriter  *writer,
                                                   GimpSessionInfo   *info,
                                                   const gchar       *factory_name);
GTokenType        gimp_session_info_deserialize   (GScanner          *scanner,
                                                   gint               scope);

void              gimp_session_info_restore       (GimpSessionInfo   *info,
                                                   GimpDialogFactory *factory);

void              gimp_session_info_set_geometry  (GimpSessionInfo   *info);
void              gimp_session_info_get_geometry  (GimpSessionInfo   *info);


#endif  /* __GIMP_SESSION_INFO_H__ */
