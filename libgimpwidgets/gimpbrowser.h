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
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_BROWSER_H__
#define __GIMP_BROWSER_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define GIMP_TYPE_BROWSER (gimp_browser_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpBrowser, gimp_browser, GIMP, BROWSER, GtkPaned)

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
  void (* _gimp_reserved5) (void);
  void (* _gimp_reserved6) (void);
  void (* _gimp_reserved7) (void);
  void (* _gimp_reserved8) (void);
};


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
