/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __GIMP_STATUSBAR_H__
#define __GIMP_STATUSBAR_H__

#include <gtk/gtkstatusbar.h>

G_BEGIN_DECLS


/*  maximal length of the format string for the cursor-coordinates  */
#define CURSOR_FORMAT_LENGTH 32


#define GIMP_TYPE_STATUSBAR            (gimp_statusbar_get_type ())
#define GIMP_STATUSBAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_STATUSBAR, GimpStatusbar))
#define GIMP_STATUSBAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_STATUSBAR, GimpStatusbarClass))
#define GIMP_IS_STATUSBAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_STATUSBAR))
#define GIMP_IS_STATUSBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_STATUSBAR))
#define GIMP_STATUSBAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_STATUSBAR, GimpStatusbarClass))


typedef struct _GimpStatusbarClass GimpStatusbarClass;

struct _GimpStatusbar
{
  GtkStatusbar      parent_instance;

  GimpDisplayShell *shell;

  GtkWidget        *cursor_frame;
  GtkWidget        *cursor_label;
  gchar             cursor_format_str[CURSOR_FORMAT_LENGTH];
  GtkWidget        *combo;

  GtkWidget        *progressbar;
  guint             progressid;
  GtkWidget        *cancelbutton;
};

struct _GimpStatusbarClass
{
  GtkStatusbarClass  parent_class;
};


GType       gimp_statusbar_get_type      (void) G_GNUC_CONST;

GtkWidget * gimp_statusbar_new           (GimpDisplayShell *shell);

void        gimp_statusbar_push          (GimpStatusbar    *statusbar,
                                          const gchar      *context_id,
                                          const gchar      *message);
void        gimp_statusbar_push_coords   (GimpStatusbar    *statusbar,
                                          const gchar      *context_id,
                                          const gchar      *title,
                                          gdouble           x,
                                          const gchar      *separator,
                                          gdouble           y);
void        gimp_statusbar_pop           (GimpStatusbar    *statusbar,
                                          const gchar      *context_id);

void	    gimp_statusbar_update_cursor (GimpStatusbar    *statusbar,
                                          gdouble           x,
                                          gdouble           y);
void        gimp_statusbar_resize_cursor (GimpStatusbar    *statusbar);


G_END_DECLS

#endif /* __GIMP_STATUSBAR_H__ */
