/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsessioninfo-private.h
 * Copyright (C) 2001-2008 Michael Natterer <mitch@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_SESSION_INFO_PRIVATE_H__
#define __GIMP_SESSION_INFO_PRIVATE_H__


struct _GimpSessionInfoPrivate
{
  /*  the dialog factory entry for object we have session info for
   *  note that pure "dock" entries don't have any factory entry
   */
  GimpDialogFactoryEntry *factory_entry;

  gint                    x;
  gint                    y;
  gint                    width;
  gint                    height;
  gboolean                right_align;
  gboolean                bottom_align;
  GdkMonitor             *monitor;

  /*  only valid while restoring and saving the session  */
  gboolean                open;

  /*  dialog specific list of GimpSessionInfoAux  */
  GList                  *aux_info;

  GtkWidget              *widget;

  /*  list of GimpSessionInfoDock  */
  GList                  *docks;
};


#endif /* __GIMP_SESSION_INFO_PRIVATE_H__ */
