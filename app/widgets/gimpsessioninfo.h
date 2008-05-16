/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsessioninfo.h
 * Copyright (C) 2001-2008 Michael Natterer <mitch@gimp.org>
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


#include "core/gimpobject.h"


#define GIMP_TYPE_SESSION_INFO            (gimp_session_info_get_type ())
#define GIMP_SESSION_INFO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_SESSION_INFO, GimpSessionInfo))
#define GIMP_SESSION_INFO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_SESSION_INFO, GimpSessionInfoClass))
#define GIMP_IS_SESSION_INFO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_SESSION_INFO))
#define GIMP_IS_SESSION_INFO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_SESSION_INFO))
#define GIMP_SESSION_INFO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_SESSION_INFO, GimpSessionInfoClass))


typedef struct _GimpSessionInfoClass  GimpSessionInfoClass;

struct _GimpSessionInfo
{
  GimpObject              parent_instance;

  gint                    x;
  gint                    y;
  gint                    width;
  gint                    height;
  gboolean                right_align;
  gboolean                bottom_align;

  /*  only valid while restoring and saving the session  */
  gboolean                open;
  gint                    screen;

  /*  dialog specific list of GimpSessionInfoAux  */
  GList                  *aux_info;

  GtkWidget              *widget;

  /*  only one of these is valid  */
  GimpDialogFactoryEntry *toplevel_entry;
  GimpDialogFactoryEntry *dockable_entry;

  /*  list of GimpSessionInfoBook  */
  GList                  *books;
};

struct _GimpSessionInfoClass
{
  GimpObjectClass  parent_class;
};


GType             gimp_session_info_get_type     (void) G_GNUC_CONST;

GimpSessionInfo * gimp_session_info_new          (void);

void              gimp_session_info_restore      (GimpSessionInfo   *info,
                                                  GimpDialogFactory *factory);

void              gimp_session_info_set_geometry (GimpSessionInfo   *info);
void              gimp_session_info_get_geometry (GimpSessionInfo   *info);

void              gimp_session_info_get_info     (GimpSessionInfo   *info);
void              gimp_session_info_clear_info   (GimpSessionInfo   *info);


#endif  /* __GIMP_SESSION_INFO_H__ */
