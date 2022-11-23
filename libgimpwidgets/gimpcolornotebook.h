/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmacolornotebook.h
 * Copyright (C) 2002 Michael Natterer <mitch@ligma.org>
 *
 * based on color_notebook module
 * Copyright (C) 1998 Austin Donnelly <austin@greenend.org.uk>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__LIGMA_WIDGETS_H_INSIDE__) && !defined (LIGMA_WIDGETS_COMPILATION)
#error "Only <libligmawidgets/ligmawidgets.h> can be included directly."
#endif

#ifndef __LIGMA_COLOR_NOTEBOOK_H__
#define __LIGMA_COLOR_NOTEBOOK_H__

#include <libligmawidgets/ligmacolorselector.h>

G_BEGIN_DECLS


#define LIGMA_TYPE_COLOR_NOTEBOOK            (ligma_color_notebook_get_type ())
#define LIGMA_COLOR_NOTEBOOK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_COLOR_NOTEBOOK, LigmaColorNotebook))
#define LIGMA_COLOR_NOTEBOOK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_COLOR_NOTEBOOK, LigmaColorNotebookClass))
#define LIGMA_IS_COLOR_NOTEBOOK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_COLOR_NOTEBOOK))
#define LIGMA_IS_COLOR_NOTEBOOK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_COLOR_NOTEBOOK))
#define LIGMA_COLOR_NOTEBOOK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_COLOR_NOTEBOOK, LigmaColorNotebookClass))


typedef struct _LigmaColorNotebookPrivate LigmaColorNotebookPrivate;
typedef struct _LigmaColorNotebookClass   LigmaColorNotebookClass;

struct _LigmaColorNotebook
{
  LigmaColorSelector         parent_instance;

  LigmaColorNotebookPrivate *priv;
};

struct _LigmaColorNotebookClass
{
  LigmaColorSelectorClass  parent_class;

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


GType               ligma_color_notebook_get_type             (void) G_GNUC_CONST;

GtkWidget         * ligma_color_notebook_set_has_page         (LigmaColorNotebook *notebook,
                                                              GType              page_type,
                                                              gboolean           has_page);

GtkWidget         * ligma_color_notebook_get_notebook         (LigmaColorNotebook *notebook);
GList             * ligma_color_notebook_get_selectors        (LigmaColorNotebook *notebook);
LigmaColorSelector * ligma_color_notebook_get_current_selector (LigmaColorNotebook *notebook);
void                ligma_color_notebook_set_simulation       (LigmaColorNotebook *notebook,
                                                              LigmaColorProfile  *profile,
                                                              LigmaColorRenderingIntent intent,
                                                              gboolean           bpc);


G_END_DECLS

#endif /* __LIGMA_COLOR_NOTEBOOK_H__ */
