/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpbrowser.h
 * Copyright (C) 2005 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_BROWSER_H__
#define __GIMP_BROWSER_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define GIMP_TYPE_BROWSER            (gimp_browser_get_type ())
#define GIMP_BROWSER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_BROWSER, GimpBrowser))
#define GIMP_BROWSER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_BROWSER, GimpBrowserClass))
#define GIMP_IS_BROWSER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_BROWSER))
#define GIMP_IS_BROWSER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_BROWSER))
#define GIMP_BROWSER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_BROWSER, GimpBrowserClass))


typedef struct _GimpBrowserClass GimpBrowserClass;

struct _GimpBrowser
{
  GtkPaned  parent_instance;
};

struct _GimpBrowserClass
{
  GtkPanedClass  parent_class;

  void (* search) (GimpBrowser *browser,
                   const gchar *search_string,
                   gint         search_type);

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
};


GType       gimp_browser_get_type         (void) G_GNUC_CONST;

GtkWidget * gimp_browser_new                (void);

void        gimp_browser_add_search_types   (GimpBrowser *browser,
                                             const gchar *first_type_label,
                                             gint         first_type_id,
                                             ...) G_GNUC_NULL_TERMINATED;

GtkWidget * gimp_browser_get_left_vbox      (GimpBrowser *browser);
GtkWidget * gimp_browser_get_right_vbox     (GimpBrowser *browser);

void        gimp_browser_set_search_summary (GimpBrowser *browser,
                                             const gchar *summary);
void        gimp_browser_set_widget         (GimpBrowser *browser,
                                             GtkWidget   *widget);
void        gimp_browser_show_message       (GimpBrowser *browser,
                                             const gchar *message);


G_END_DECLS

#endif  /*  __GIMP_BROWSER_H__  */
