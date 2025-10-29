/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpnotebook.h
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

#ifndef __GIMP_NOTEBOOK_H__
#define __GIMP_NOTEBOOK_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define GIMP_TYPE_NOTEBOOK (gimp_notebook_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpNotebook, gimp_notebook, GIMP, NOTEBOOK, GtkNotebook)

struct _GimpNotebookClass
{
  GtkNotebookClass  parent_class;

  /* Padding for future expansion */
  void (* _gimp_reserved0) (void);
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
  void (* _gimp_reserved5) (void);
  void (* _gimp_reserved6) (void);
  void (* _gimp_reserved7) (void);
  void (* _gimp_reserved8) (void);
  void (* _gimp_reserved9) (void);
};


GtkWidget * gimp_notebook_new              (void);

gint        gimp_notebook_append_page      (GimpNotebook *notebook,
                                            GtkWidget    *child,
                                            GtkWidget    *tab_label);

G_END_DECLS

#endif /* __GIMP_NOTEBOOK_H__ */
