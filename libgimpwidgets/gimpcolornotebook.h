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
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_COLOR_NOTEBOOK_H__
#define __GIMP_COLOR_NOTEBOOK_H__

#include <libgimpwidgets/gimpcolorselector.h>

G_BEGIN_DECLS


#define GIMP_TYPE_COLOR_NOTEBOOK (gimp_color_notebook_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpColorNotebook, gimp_color_notebook, GIMP, COLOR_NOTEBOOK, GimpColorSelector)

struct _GimpColorNotebookClass
{
  GimpColorSelectorClass  parent_class;

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


GtkWidget         * gimp_color_notebook_set_has_page         (GimpColorNotebook *notebook,
                                                              GType              page_type,
                                                              gboolean           has_page);

GtkWidget         * gimp_color_notebook_get_notebook         (GimpColorNotebook *notebook);
GList             * gimp_color_notebook_get_selectors        (GimpColorNotebook *notebook);
GimpColorSelector * gimp_color_notebook_get_current_selector (GimpColorNotebook *notebook);
void                gimp_color_notebook_set_format           (GimpColorNotebook *notebook,
                                                              const Babl        *format);
void                gimp_color_notebook_set_simulation       (GimpColorNotebook *notebook,
                                                              GimpColorProfile  *profile,
                                                              GimpColorRenderingIntent intent,
                                                              gboolean           bpc);
void                gimp_color_notebook_enable_simulation    (GimpColorNotebook *notebook,
                                                              gboolean           enabled);


G_END_DECLS

#endif /* __GIMP_COLOR_NOTEBOOK_H__ */
