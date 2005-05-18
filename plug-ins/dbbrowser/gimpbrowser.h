/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbrowser.h
 * Copyright (C) 2005 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_BROWSER_H__
#define __GIMP_BROWSER_H__


#include <gtk/gtkhpaned.h>


#define GIMP_TYPE_BROWSER            (gimp_browser_get_type ())
#define GIMP_BROWSER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_BROWSER, GimpBrowser))
#define GIMP_BROWSER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_BROWSER, GimpBrowserClass))
#define GIMP_IS_BROWSER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_BROWSER))
#define GIMP_IS_BROWSER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_BROWSER))
#define GIMP_BROWSER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_BROWSER, GimpBrowserClass))


typedef struct _GimpBrowser       GimpBrowser;
typedef struct _GimpBrowserClass  GimpBrowserClass;

struct _GimpBrowser
{
  GtkHPaned  parent_instance;

  GtkWidget *left_vbox;

  GtkWidget *search_entry;
  guint      search_timeout_id;

  GtkWidget *search_type_combo;
  gint       search_type;

  GtkWidget *count_label;

  GtkWidget *right_vbox;
};

struct _GimpBrowserClass
{
  GtkHPanedClass  parent_class;

  void (* search) (GimpBrowser *browser,
                   gint         search_type);
};


GType       gimp_browser_get_type         (void) G_GNUC_CONST;

GtkWidget * gimp_browser_new              (void);

void        gimp_browser_add_search_types (GimpBrowser *browser,
                                           const gchar *first_type_label,
                                           gint         first_type_id,
                                           ...);

void        gimp_browser_set_widget       (GimpBrowser *browser,
                                           GtkWidget   *widget);
void        gimp_browser_show_message     (GimpBrowser *browser,
                                           const gchar *message);


#endif  /*  __GIMP_BROWSER_H__  */
