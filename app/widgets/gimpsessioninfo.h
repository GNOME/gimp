/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsessioninfo.h
 * Copyright (C) 2001-2003 Michael Natterer <mitch@gimp.org>
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

  GtkWidget *widget;

  /*  only valid while restoring and saving the session  */
  gboolean   open;

  /*  GList of gchar* of optional additional dialog specific info  */
  GList     *aux_info;

  /*  only one of these is valid  */
  GimpDialogFactoryEntry *toplevel_entry;
  GimpDialogFactoryEntry *dockable_entry;
  GList                  *books;  /*  GList of GimpSessionInfoBook  */
};

struct _GimpSessionInfoBook
{
  gint       position;
  GList     *dockables; /*  GList of GimpSessionInfoDockable  */

  GtkWidget *widget; /*  only used while restoring the session  */
};

struct _GimpSessionInfoDockable
{
  gchar        *identifier;
  GimpTabStyle  tab_style;
  gint          preview_size;

  /*  GList of gchar* of optional additional dockable specific info  */
  GList        *aux_info;
};


void       gimp_session_info_free          (GimpSessionInfo         *info);
void       gimp_session_info_book_free     (GimpSessionInfoBook     *book);
void       gimp_session_info_dockable_free (GimpSessionInfoDockable *dockable);

void       gimp_session_info_save          (GimpSessionInfo         *info,
                                            const gchar             *factory_name,
                                            GimpConfigWriter        *writer);
GTokenType gimp_session_info_deserialize   (GScanner                *scanner,
                                            gint                     old_scope);
void       gimp_session_info_restore       (GimpSessionInfo         *info,
                                            GimpDialogFactory       *factory);

void       gimp_session_info_set_geometry  (GimpSessionInfo         *info);
void       gimp_session_info_get_geometry  (GimpSessionInfo         *info);


#endif  /*  __GIMP_SESSION_INFO_H__  */
