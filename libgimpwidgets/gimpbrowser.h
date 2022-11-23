/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmabrowser.h
 * Copyright (C) 2005 Michael Natterer <mitch@ligma.org>
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

#if !defined (__LIGMA_WIDGETS_H_INSIDE__) && !defined (LIGMA_WIDGETS_COMPILATION)
#error "Only <libligmawidgets/ligmawidgets.h> can be included directly."
#endif

#ifndef __LIGMA_BROWSER_H__
#define __LIGMA_BROWSER_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define LIGMA_TYPE_BROWSER            (ligma_browser_get_type ())
#define LIGMA_BROWSER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_BROWSER, LigmaBrowser))
#define LIGMA_BROWSER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_BROWSER, LigmaBrowserClass))
#define LIGMA_IS_BROWSER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_BROWSER))
#define LIGMA_IS_BROWSER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_BROWSER))
#define LIGMA_BROWSER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_BROWSER, LigmaBrowserClass))


typedef struct _LigmaBrowserPrivate LigmaBrowserPrivate;
typedef struct _LigmaBrowserClass   LigmaBrowserClass;

struct _LigmaBrowser
{
  GtkPaned            parent_instance;

  LigmaBrowserPrivate *priv;
};

struct _LigmaBrowserClass
{
  GtkPanedClass  parent_class;

  void (* search) (LigmaBrowser *browser,
                   const gchar *search_string,
                   gint         search_type);

  /* Padding for future expansion */
  void (* _ligma_reserved1) (void);
  void (* _ligma_reserved2) (void);
  void (* _ligma_reserved3) (void);
  void (* _ligma_reserved4) (void);
  void (* _ligma_reserved5) (void);
  void (* _ligma_reserved6) (void);
  void (* _ligma_reserved7) (void);
  void (* _ligma_reserved8) (void);
};


GType       ligma_browser_get_type         (void) G_GNUC_CONST;

GtkWidget * ligma_browser_new                (void);

void        ligma_browser_add_search_types   (LigmaBrowser *browser,
                                             const gchar *first_type_label,
                                             gint         first_type_id,
                                             ...) G_GNUC_NULL_TERMINATED;

GtkWidget * ligma_browser_get_left_vbox      (LigmaBrowser *browser);
GtkWidget * ligma_browser_get_right_vbox     (LigmaBrowser *browser);

void        ligma_browser_set_search_summary (LigmaBrowser *browser,
                                             const gchar *summary);
void        ligma_browser_set_widget         (LigmaBrowser *browser,
                                             GtkWidget   *widget);
void        ligma_browser_show_message       (LigmaBrowser *browser,
                                             const gchar *message);


G_END_DECLS

#endif  /*  __LIGMA_BROWSER_H__  */
