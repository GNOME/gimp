/* LIBGIMP - The GIMP Library       
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball 
 *
 * gimpsizeentry.h
 * Copyright (C) 1999-2000 Sven Neumann <sven@gimp.org>
 *                         Michael Natterer <mitch@gimp.org> 
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __GIMP_SIZE_ENTRY_H__
#define __GIMP_SIZE_ENTRY_H__

#include <gtk/gtk.h>

#include "gimpunit.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GIMP_TYPE_SIZE_ENTRY            (gimp_size_entry_get_type ())
#define GIMP_SIZE_ENTRY(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_SIZE_ENTRY, GimpSizeEntry))
#define GIMP_SIZE_ENTRY_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_SIZE_ENTRY, GimpSizeEntryClass))
#define GIMP_IS_SIZE_ENTRY(obj)         (GTK_CHECK_TYPE (obj, GIMP_TYPE_SIZE_ENTRY))
#define GIMP_IS_SIZE_ENTRY_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_SIZE_ENTRY))

typedef struct _GimpSizeEntry       GimpSizeEntry;
typedef struct _GimpSizeEntryClass  GimpSizeEntryClass;

typedef enum
{
  GIMP_SIZE_ENTRY_UPDATE_NONE       = 0,
  GIMP_SIZE_ENTRY_UPDATE_SIZE       = 1,
  GIMP_SIZE_ENTRY_UPDATE_RESOLUTION = 2
} GimpSizeEntryUpdatePolicy;

typedef struct _GimpSizeEntryField  GimpSizeEntryField;

struct _GimpSizeEntry
{
  GtkTable   table;

  GSList    *fields;
  gint       number_of_fields;

  GtkWidget *unitmenu;
  GimpUnit   unit;
  gboolean   menu_show_pixels;
  gboolean   menu_show_percent;

  gboolean                   show_refval;
  GimpSizeEntryUpdatePolicy  update_policy;
};

struct _GimpSizeEntryClass
{
  GtkTableClass parent_class;

  void (* value_changed)  (GimpSizeEntry *gse);
  void (* refval_changed) (GimpSizeEntry *gse);
  void (* unit_changed)   (GimpSizeEntry *gse);
};

/* For information look into the C source or the html documentation */

GtkType     gimp_size_entry_get_type (void);

GtkWidget * gimp_size_entry_new (gint                       number_of_fields,
				 GimpUnit                   unit,
				 gchar                     *unit_format,
				 gboolean                   menu_show_pixels,
				 gboolean                   menu_show_percent,
				 gboolean                   show_refval,
				 gint                       spinbutton_usize,
				 GimpSizeEntryUpdatePolicy  update_policy);

void        gimp_size_entry_add_field  (GimpSizeEntry   *gse,
					GtkSpinButton   *value_spinbutton,
					GtkSpinButton   *refval_spinbutton);

void        gimp_size_entry_attach_label          (GimpSizeEntry *gse,
						   gchar         *text,
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

GimpUnit    gimp_size_entry_get_unit              (GimpSizeEntry *gse);
void        gimp_size_entry_set_unit              (GimpSizeEntry *gse, 
					           GimpUnit       unit);

void        gimp_size_entry_grab_focus            (GimpSizeEntry *gse);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GIMP_SIZE_ENTRY_H__ */
