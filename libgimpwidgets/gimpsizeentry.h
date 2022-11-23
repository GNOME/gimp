/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmasizeentry.h
 * Copyright (C) 1999-2000 Sven Neumann <sven@ligma.org>
 *                         Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_SIZE_ENTRY_H__
#define __LIGMA_SIZE_ENTRY_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define LIGMA_TYPE_SIZE_ENTRY            (ligma_size_entry_get_type ())
#define LIGMA_SIZE_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_SIZE_ENTRY, LigmaSizeEntry))
#define LIGMA_SIZE_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_SIZE_ENTRY, LigmaSizeEntryClass))
#define LIGMA_IS_SIZE_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, LIGMA_TYPE_SIZE_ENTRY))
#define LIGMA_IS_SIZE_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_SIZE_ENTRY))
#define LIGMA_SIZE_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_SIZE_ENTRY, LigmaSizeEntryClass))


typedef struct _LigmaSizeEntryPrivate LigmaSizeEntryPrivate;
typedef struct _LigmaSizeEntryClass   LigmaSizeEntryClass;

typedef struct _LigmaSizeEntryField  LigmaSizeEntryField;

struct _LigmaSizeEntry
{
  GtkGrid               parent_instance;

  LigmaSizeEntryPrivate *priv;
};

struct _LigmaSizeEntryClass
{
  GtkGridClass  parent_class;

  void (* value_changed)  (LigmaSizeEntry *gse);
  void (* refval_changed) (LigmaSizeEntry *gse);
  void (* unit_changed)   (LigmaSizeEntry *gse);

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


/* For information look into the C source or the html documentation */

GType       ligma_size_entry_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_size_entry_new (gint                       number_of_fields,
                                 LigmaUnit                   unit,
                                 const gchar               *unit_format,
                                 gboolean                   menu_show_pixels,
                                 gboolean                   menu_show_percent,
                                 gboolean                   show_refval,
                                 gint                       spinbutton_width,
                                 LigmaSizeEntryUpdatePolicy  update_policy);

void        ligma_size_entry_add_field  (LigmaSizeEntry   *gse,
                                        GtkSpinButton   *value_spinbutton,
                                        GtkSpinButton   *refval_spinbutton);

LigmaSizeEntryUpdatePolicy
            ligma_size_entry_get_update_policy     (LigmaSizeEntry *gse);
gint        ligma_size_entry_get_n_fields          (LigmaSizeEntry *gse);
GtkWidget * ligma_size_entry_get_unit_combo        (LigmaSizeEntry *gse);

GtkWidget * ligma_size_entry_attach_label          (LigmaSizeEntry *gse,
                                                   const gchar   *text,
                                                   gint           row,
                                                   gint           column,
                                                   gfloat         alignment);

void        ligma_size_entry_set_resolution        (LigmaSizeEntry *gse,
                                                   gint           field,
                                                   gdouble        resolution,
                                                   gboolean       keep_size);

void        ligma_size_entry_set_size              (LigmaSizeEntry *gse,
                                                   gint           field,
                                                   gdouble        lower,
                                                   gdouble        upper);

void        ligma_size_entry_set_value_boundaries  (LigmaSizeEntry *gse,
                                                   gint           field,
                                                   gdouble        lower,
                                                   gdouble        upper);

gdouble     ligma_size_entry_get_value             (LigmaSizeEntry *gse,
                                                   gint           field);
void        ligma_size_entry_set_value             (LigmaSizeEntry *gse,
                                                   gint           field,
                                                   gdouble        value);

void        ligma_size_entry_set_refval_boundaries (LigmaSizeEntry *gse,
                                                   gint           field,
                                                   gdouble        lower,
                                                   gdouble        upper);
void        ligma_size_entry_set_refval_digits     (LigmaSizeEntry *gse,
                                                   gint           field,
                                                   gint           digits);

gdouble     ligma_size_entry_get_refval            (LigmaSizeEntry *gse,
                                                   gint           field);
void        ligma_size_entry_set_refval            (LigmaSizeEntry *gse,
                                                   gint           field,
                                                   gdouble        refval);

LigmaUnit    ligma_size_entry_get_unit              (LigmaSizeEntry *gse);
void        ligma_size_entry_set_unit              (LigmaSizeEntry *gse,
                                                   LigmaUnit       unit);
void        ligma_size_entry_show_unit_menu        (LigmaSizeEntry *gse,
                                                   gboolean       show);

void        ligma_size_entry_set_pixel_digits      (LigmaSizeEntry *gse,
                                                   gint           digits);

void        ligma_size_entry_grab_focus            (LigmaSizeEntry *gse);
void        ligma_size_entry_set_activates_default (LigmaSizeEntry *gse,
                                                   gboolean       setting);
GtkWidget * ligma_size_entry_get_help_widget       (LigmaSizeEntry *gse,
                                                   gint           field);


G_END_DECLS

#endif /* __LIGMA_SIZE_ENTRY_H__ */
