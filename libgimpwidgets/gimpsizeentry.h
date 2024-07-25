/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpsizeentry.h
 * Copyright (C) 1999-2000 Sven Neumann <sven@gimp.org>
 *                         Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_SIZE_ENTRY_H__
#define __GIMP_SIZE_ENTRY_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define GIMP_TYPE_SIZE_ENTRY (gimp_size_entry_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpSizeEntry, gimp_size_entry, GIMP, SIZE_ENTRY, GtkGrid)

struct _GimpSizeEntryClass
{
  GtkGridClass  parent_class;

  void (* value_changed)  (GimpSizeEntry *gse);
  void (* refval_changed) (GimpSizeEntry *gse);
  void (* unit_changed)   (GimpSizeEntry *gse);

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


/* For information look into the C source or the html documentation */

GtkWidget * gimp_size_entry_new (gint                       number_of_fields,
                                 GimpUnit                  *unit,
                                 const gchar               *unit_format,
                                 gboolean                   menu_show_pixels,
                                 gboolean                   menu_show_percent,
                                 gboolean                   show_refval,
                                 gint                       spinbutton_width,
                                 GimpSizeEntryUpdatePolicy  update_policy);

void        gimp_size_entry_add_field  (GimpSizeEntry   *gse,
                                        GtkSpinButton   *value_spinbutton,
                                        GtkSpinButton   *refval_spinbutton);

GimpSizeEntryUpdatePolicy
            gimp_size_entry_get_update_policy     (GimpSizeEntry *gse);
gint        gimp_size_entry_get_n_fields          (GimpSizeEntry *gse);
GtkWidget * gimp_size_entry_get_unit_combo        (GimpSizeEntry *gse);

GtkWidget * gimp_size_entry_attach_label          (GimpSizeEntry *gse,
                                                   const gchar   *text,
                                                   gint           row,
                                                   gint           column,
                                                   gfloat         alignment);

void        gimp_size_entry_set_resolution        (GimpSizeEntry *gse,
                                                   gint           field,
                                                   gdouble        resolution,
                                                   gboolean       keep_size);

void        gimp_size_entry_set_size              (GimpSizeEntry *gse,
                                                   gint           field,
                                                   gdouble        lower,
                                                   gdouble        upper);

void        gimp_size_entry_set_value_boundaries  (GimpSizeEntry *gse,
                                                   gint           field,
                                                   gdouble        lower,
                                                   gdouble        upper);

gdouble     gimp_size_entry_get_value             (GimpSizeEntry *gse,
                                                   gint           field);
void        gimp_size_entry_set_value             (GimpSizeEntry *gse,
                                                   gint           field,
                                                   gdouble        value);

void        gimp_size_entry_set_refval_boundaries (GimpSizeEntry *gse,
                                                   gint           field,
                                                   gdouble        lower,
                                                   gdouble        upper);
void        gimp_size_entry_set_refval_digits     (GimpSizeEntry *gse,
                                                   gint           field,
                                                   gint           digits);

gdouble     gimp_size_entry_get_refval            (GimpSizeEntry *gse,
                                                   gint           field);
void        gimp_size_entry_set_refval            (GimpSizeEntry *gse,
                                                   gint           field,
                                                   gdouble        refval);

GimpUnit  * gimp_size_entry_get_unit              (GimpSizeEntry *gse);
void        gimp_size_entry_set_unit              (GimpSizeEntry *gse,
                                                   GimpUnit      *unit);
void        gimp_size_entry_show_unit_menu        (GimpSizeEntry *gse,
                                                   gboolean       show);

void        gimp_size_entry_set_pixel_digits      (GimpSizeEntry *gse,
                                                   gint           digits);

void        gimp_size_entry_grab_focus            (GimpSizeEntry *gse);
void        gimp_size_entry_set_activates_default (GimpSizeEntry *gse,
                                                   gboolean       setting);
GtkWidget * gimp_size_entry_get_help_widget       (GimpSizeEntry *gse,
                                                   gint           field);


G_END_DECLS

#endif /* __GIMP_SIZE_ENTRY_H__ */
