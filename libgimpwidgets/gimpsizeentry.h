/* LIBGIMP - The GIMP Library                                                   
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
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
} GimpSizeEntryUP;

typedef struct _GimpSizeEntryField  GimpSizeEntryField;

struct _GimpSizeEntry
{
  GtkTable         table;

  GSList          *fields;
  gint             number_of_fields;

  GtkWidget       *unitmenu;
  GUnit            unit;
  gboolean         menu_show_pixels;
  gboolean         menu_show_percent;

  gboolean         show_refval;
  GimpSizeEntryUP  update_policy;
};

struct _GimpSizeEntryClass
{
  GtkTableClass parent_class;

  void (* value_changed)  (GimpSizeEntry *gse);
  void (* refval_changed) (GimpSizeEntry *gse);
  void (* unit_changed)   (GimpSizeEntry *gse);
};

GtkType gimp_size_entry_get_type (void);

/* creates a new GimpSizeEntry widget
 * number_of_fields  -- how many spinbuttons to show
 * unit              -- unit to show initially
 * unit_format       -- printf-like unit-format (see GimpUnitMenu)
 * menu_show_pixels  -- should the unit menu contain 'pixels'
 *                      this parameter is ignored if you select an update_policy
 * show_refval       -- TRUE if you want the extra 'reference value' row
 * spinbutton_usize  -- the minimal horizontal size the spinbuttons will have
 * update_policy     -- how calculations should be performed
 *                      GIMP_SIZE_ENTRY_UPDATE_NONE --> no calculations
 *			GIMP_SIZE_ENTRY_UPDATE_SIZE --> consider the values to
 *                        be distances. The reference value equals to pixels
 *			GIMP_SIZE_ENTRY_UPDATE_RESOLUTION --> consider the values
 *                        to be resolutions. The reference value equals to dpi
 *
 * to have all automatic calculations performed correctly, set up the
 * widget in the following order:
 * 1. gimp_size_entry_new
 * 2. (for each additional input field) gimp_size_entry_add_field
 * 3. gimp_size_entry_set_unit
 * for each input field:
 * 4. gimp_size_entry_set_resolution
 * 5. gimp_size_entry_set_value_boundaries (or _set_refval_boundaries)
 * 6. gimp_size_entry_set_size
 * 7. gimp_size_entry_set_value (or _set_refval)
 *
 * the newly created GimpSizeEntry table will have an empty border
 * of one cell width on each side plus an empty column left of the
 * unit menu to allow the caller to add labels
 */
GtkWidget*  gimp_size_entry_new        (gint             number_of_fields,
					GUnit            unit,
					gchar           *unit_format,
					gboolean         menu_show_pixels,
					gboolean         menu_show_percent,
					gboolean         show_refval,
					gint             spinbutton_usize,
					GimpSizeEntryUP  update_policy);

/* add a field to the sizeentry
 * if show_refval if FALSE, then the refval pointers will be ignored
 *
 * the new field will have the index 0
 */
void        gimp_size_entry_add_field  (GimpSizeEntry   *gse,
					GtkSpinButton   *value_spinbutton,
					GtkSpinButton   *refval_spinbutton);

/* this one is just a convenience function if you want to add labels
 * to the empty cells of the widget
 */
void        gimp_size_entry_attach_label          (GimpSizeEntry *gse,
						   gchar         *text,
						   gint           row,
						   gint           column,
						   gfloat         alignment);

/* this one sets the resolution (in dpi)
 *
 * does nothing if update_policy != GIMP_SIZE_ENTRY_UPDATE_SIZE
 *
 * keep_size is a boolean value. If TRUE, the size in pixels will stay
 * the same, otherwise the size in units will stay the same.
 */
void        gimp_size_entry_set_resolution        (GimpSizeEntry *gse,
					           gint           field,
					           gdouble        resolution,
						   guint          keep_size);

/* this one sets the values (in pixels) which will be treated as
 * 0% and 100% when we want "percent" in the unit menu
 *
 * does nothing if update_policy != GIMP_SIZE_ENTRY_UPDATE_SIZE
 */
void        gimp_size_entry_set_size              (GimpSizeEntry *gse,
					           gint           field,
					           gdouble        lower,
						   gdouble        upper);

/* these functions set/return the value in the units the user selected
 * note that in some cases where the caller chooses not to have the
 * reference value row and the user selected the reference unit
 * the both values 'value' and 'refval' will be the same
 */
void        gimp_size_entry_set_value_boundaries  (GimpSizeEntry *gse,
						   gint           field,
						   gdouble        lower,
						   gdouble        upper);
gdouble     gimp_size_entry_get_value             (GimpSizeEntry *gse,
					           gint           field);
void        gimp_size_entry_set_value             (GimpSizeEntry *gse,
					           gint           field,
					           gdouble        value);

/* these functions set/return the value in the 'reference unit' for the
 * current update policy
 * for GIMP_SIZE_ENTRY_UPDATE_SIZE       it's the value in pixels
 * for GIMP_SIZE_ENTRY_UPDATE_RESOLUTION it's the resolution in dpi
 * for GIMP_SIZE_ENTRY_UPDATE_NONE       it's up to the caller as he has to
 *                                       provide a correct value<->refval
 *                                       mapping
 */
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

/* these functions set/return the currently used unit
 * note that for GIMP_SIZE_ENTRY_UPDATE_SIZE a value of UNIT_PIXEL
 * will be silently ignored if we have the extra refvalue line
 */
GUnit       gimp_size_entry_get_unit              (GimpSizeEntry *gse);
void        gimp_size_entry_set_unit              (GimpSizeEntry *gse, 
					           GUnit          unit);

/* this makes the first spinbutton grab the focus
 */
void        gimp_size_entry_grab_focus            (GimpSizeEntry *gse);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GIMP_SIZE_ENTRY_H__ */
