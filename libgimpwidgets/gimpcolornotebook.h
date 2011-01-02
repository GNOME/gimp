/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcolornotebook.h
 * Copyright (C) 2002 Michael Natterer <mitch@gimp.org>
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
 * <http://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_COLOR_NOTEBOOK_H__
#define __GIMP_COLOR_NOTEBOOK_H__

#include <libgimpwidgets/gimpcolorselector.h>

G_BEGIN_DECLS


#define GIMP_TYPE_COLOR_NOTEBOOK            (gimp_color_notebook_get_type ())
#define GIMP_COLOR_NOTEBOOK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_COLOR_NOTEBOOK, GimpColorNotebook))
#define GIMP_COLOR_NOTEBOOK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_COLOR_NOTEBOOK, GimpColorNotebookClass))
#define GIMP_IS_COLOR_NOTEBOOK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_COLOR_NOTEBOOK))
#define GIMP_IS_COLOR_NOTEBOOK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_COLOR_NOTEBOOK))
#define GIMP_COLOR_NOTEBOOK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_COLOR_NOTEBOOK, GimpColorNotebookClass))


typedef struct _GimpColorNotebookPrivate GimpColorNotebookPrivate;
typedef struct _GimpColorNotebookClass   GimpColorNotebookClass;

struct _GimpColorNotebook
{
  GimpColorSelector         parent_instance;

  GimpColorNotebookPrivate *priv;
};

struct _GimpColorNotebookClass
{
  GimpColorSelectorClass  parent_class;

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
};


GType               gimp_color_notebook_get_type             (void) G_GNUC_CONST;

GtkWidget         * gimp_color_notebook_set_has_page         (GimpColorNotebook *notebook,
                                                              GType              page_type,
                                                              gboolean           has_page);

GtkWidget         * gimp_color_notebook_get_notebook         (GimpColorNotebook *notebook);
GList             * gimp_color_notebook_get_selectors        (GimpColorNotebook *notebook);
GimpColorSelector * gimp_color_notebook_get_current_selector (GimpColorNotebook *notebook);


G_END_DECLS

#endif /* __GIMP_COLOR_NOTEBOOK_H__ */
