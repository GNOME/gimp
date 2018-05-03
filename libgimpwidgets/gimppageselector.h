/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppageselector.h
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

#ifndef __GIMP_PAGE_SELECTOR_H__
#define __GIMP_PAGE_SELECTOR_H__

G_BEGIN_DECLS

#define GIMP_TYPE_PAGE_SELECTOR            (gimp_page_selector_get_type ())
#define GIMP_PAGE_SELECTOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PAGE_SELECTOR, GimpPageSelector))
#define GIMP_PAGE_SELECTOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PAGE_SELECTOR, GimpPageSelectorClass))
#define GIMP_IS_PAGE_SELECTOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PAGE_SELECTOR))
#define GIMP_IS_PAGE_SELECTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PAGE_SELECTOR))
#define GIMP_PAGE_SELECTOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PAGE_SELECTOR, GimpPageSelectorClass))


typedef struct _GimpPageSelectorPrivate GimpPageSelectorPrivate;
typedef struct _GimpPageSelectorClass   GimpPageSelectorClass;

struct _GimpPageSelector
{
  GtkBox                   parent_instance;

  GimpPageSelectorPrivate *priv;
};

struct _GimpPageSelectorClass
{
  GtkBoxClass  parent_class;

  void (* selection_changed) (GimpPageSelector *selector);
  void (* activate)          (GimpPageSelector *selector);

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


GType       gimp_page_selector_get_type           (void) G_GNUC_CONST;

GtkWidget * gimp_page_selector_new                (void);

void        gimp_page_selector_set_n_pages        (GimpPageSelector *selector,
                                                   gint              n_pages);
gint        gimp_page_selector_get_n_pages        (GimpPageSelector *selector);

void        gimp_page_selector_set_target   (GimpPageSelector       *selector,
                                             GimpPageSelectorTarget  target);
GimpPageSelectorTarget
            gimp_page_selector_get_target   (GimpPageSelector       *selector);

void        gimp_page_selector_set_page_thumbnail (GimpPageSelector *selector,
                                                   gint              page_no,
                                                   GdkPixbuf        *thumbnail);
GdkPixbuf * gimp_page_selector_get_page_thumbnail (GimpPageSelector *selector,
                                                   gint              page_no);

void        gimp_page_selector_set_page_label     (GimpPageSelector *selector,
                                                   gint              page_no,
                                                   const gchar      *label);
gchar     * gimp_page_selector_get_page_label     (GimpPageSelector *selector,
                                                   gint              page_no);

void        gimp_page_selector_select_all         (GimpPageSelector *selector);
void        gimp_page_selector_unselect_all       (GimpPageSelector *selector);
void        gimp_page_selector_select_page        (GimpPageSelector *selector,
                                                   gint              page_no);
void        gimp_page_selector_unselect_page      (GimpPageSelector *selector,
                                                   gint              page_no);
gboolean    gimp_page_selector_page_is_selected   (GimpPageSelector *selector,
                                                   gint              page_no);
gint      * gimp_page_selector_get_selected_pages (GimpPageSelector *selector,
                                                   gint             *n_selected_pages);

void        gimp_page_selector_select_range       (GimpPageSelector *selector,
                                                   const gchar      *range);
gchar     * gimp_page_selector_get_selected_range (GimpPageSelector *selector);

G_END_DECLS

#endif /* __GIMP_PAGE_SELECTOR_H__ */
