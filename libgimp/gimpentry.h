/* LIBGIMP - The GIMP Library                                                   
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.             
 *                                                                              
 * This library is distributed in the hope that it will be useful,              
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU            
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */                                                                             
#ifndef __GIMP_ENTRY_H__
#define __GIMP_ENTRY_H__

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GIMP_TYPE_SIZE_ENTRY          (gimp_size_entry_get_type ())
#define GIMP_SIZE_ENTRY(obj)          (GTK_CHECK_CAST ((obj), GIMP_TYPE_SIZE_ENTRY, GimpSizeEntry))
#define GIMP_SIZE_ENTRY_CLASS(klass)  (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_SIZE_ENTRY, GimpSizeEntryClass))
#define GIMP_IS_SIZE_ENTRY(obj)       (GTK_CHECK_TYPE (obj, GIMP_TYPE_SIZE_ENTRY)
#define GIMP_IS_ENTRY_CLASS(klass)    (GTK_CHECK_CLASS_TYPE ((klass), GTK_TYPE_ENTRY))

typedef enum
{
  PIXELS = 0,
  INCHES = 1,
  CM     = 2
} GSizeUnit;

typedef struct _GimpSizeEntry       GimpSizeEntry;
typedef struct _GimpSizeEntryClass  GimpSizeEntryClass;

struct _GimpSizeEntry
{
  GtkHBox hbox;
      
  GtkWidget *spinbutton;
  GtkWidget *optionmenu;
  GSizeUnit  unit;
  gfloat     resolution;
  guint      positive_only; 
};

struct _GimpSizeEntryClass
{
  GtkHBoxClass parent_class;

  void (* gimp_size_entry) (GimpSizeEntry *gse);
};

guint          gimp_size_entry_get_type        (void);
GtkWidget*     gimp_size_entry_new             (gfloat value, 
						GSizeUnit unit,
						gfloat resolution,
						guint positive_only);
void           gimp_size_entry_set_value       (GimpSizeEntry *gse, 
						gfloat value);
void           gimp_size_entry_set_value_as_pixels (GimpSizeEntry *gse, 
						gint pixels);
gfloat         gimp_size_entry_get_value       (GimpSizeEntry *gse);
gint           gimp_size_entry_get_value_as_pixels (GimpSizeEntry *gse);
void           gimp_size_entry_set_unit        (GimpSizeEntry *gse, 
						GSizeUnit unit);
GSizeUnit      gimp_size_entry_get_unit        (GimpSizeEntry *gse);
void           gimp_size_entry_set_resolution  (GimpSizeEntry *gse, 
						gfloat resolution);
#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GIMP_ENTRY_H__ */



