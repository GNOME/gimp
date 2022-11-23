/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmapageselector.h
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

#ifndef __LIGMA_PAGE_SELECTOR_H__
#define __LIGMA_PAGE_SELECTOR_H__

G_BEGIN_DECLS

#define LIGMA_TYPE_PAGE_SELECTOR            (ligma_page_selector_get_type ())
#define LIGMA_PAGE_SELECTOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PAGE_SELECTOR, LigmaPageSelector))
#define LIGMA_PAGE_SELECTOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PAGE_SELECTOR, LigmaPageSelectorClass))
#define LIGMA_IS_PAGE_SELECTOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PAGE_SELECTOR))
#define LIGMA_IS_PAGE_SELECTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PAGE_SELECTOR))
#define LIGMA_PAGE_SELECTOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PAGE_SELECTOR, LigmaPageSelectorClass))


typedef struct _LigmaPageSelectorPrivate LigmaPageSelectorPrivate;
typedef struct _LigmaPageSelectorClass   LigmaPageSelectorClass;

struct _LigmaPageSelector
{
  GtkBox                   parent_instance;

  LigmaPageSelectorPrivate *priv;
};

struct _LigmaPageSelectorClass
{
  GtkBoxClass  parent_class;

  void (* selection_changed) (LigmaPageSelector *selector);
  void (* activate)          (LigmaPageSelector *selector);

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


GType       ligma_page_selector_get_type           (void) G_GNUC_CONST;

GtkWidget * ligma_page_selector_new                (void);

void        ligma_page_selector_set_n_pages        (LigmaPageSelector *selector,
                                                   gint              n_pages);
gint        ligma_page_selector_get_n_pages        (LigmaPageSelector *selector);

void        ligma_page_selector_set_target   (LigmaPageSelector       *selector,
                                             LigmaPageSelectorTarget  target);
LigmaPageSelectorTarget
            ligma_page_selector_get_target   (LigmaPageSelector       *selector);

void        ligma_page_selector_set_page_thumbnail (LigmaPageSelector *selector,
                                                   gint              page_no,
                                                   GdkPixbuf        *thumbnail);
GdkPixbuf * ligma_page_selector_get_page_thumbnail (LigmaPageSelector *selector,
                                                   gint              page_no);

void        ligma_page_selector_set_page_label     (LigmaPageSelector *selector,
                                                   gint              page_no,
                                                   const gchar      *label);
gchar     * ligma_page_selector_get_page_label     (LigmaPageSelector *selector,
                                                   gint              page_no);

void        ligma_page_selector_select_all         (LigmaPageSelector *selector);
void        ligma_page_selector_unselect_all       (LigmaPageSelector *selector);
void        ligma_page_selector_select_page        (LigmaPageSelector *selector,
                                                   gint              page_no);
void        ligma_page_selector_unselect_page      (LigmaPageSelector *selector,
                                                   gint              page_no);
gboolean    ligma_page_selector_page_is_selected   (LigmaPageSelector *selector,
                                                   gint              page_no);
gint      * ligma_page_selector_get_selected_pages (LigmaPageSelector *selector,
                                                   gint             *n_selected_pages);

void        ligma_page_selector_select_range       (LigmaPageSelector *selector,
                                                   const gchar      *range);
gchar     * ligma_page_selector_get_selected_range (LigmaPageSelector *selector);

G_END_DECLS

#endif /* __LIGMA_PAGE_SELECTOR_H__ */
