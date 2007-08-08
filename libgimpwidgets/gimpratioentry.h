/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpratioentry.h
 * Copyright (C) 2006  Simon Budig  <simon@gimp.org>
 * Copyright (C) 2007  Sven Neumann  <sven@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_RATIO_ENTRY_H__
#define __GIMP_RATIO_ENTRY_H__

G_BEGIN_DECLS


#define GIMP_TYPE_RATIO_ENTRY            (gimp_ratio_entry_get_type ())
#define GIMP_RATIO_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_RATIO_ENTRY, GimpRatioEntry))
#define GIMP_RATIO_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_RATIO_ENTRY, GimpRatioEntryClass))
#define GIMP_IS_RATIO_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_RATIO_ENTRY))
#define GIMP_IS_RATIO_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_RATIO_ENTRY))
#define GIMP_RATIO_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_RATIO_AREA, GimpRatioEntryClass))


typedef struct _GimpRatioEntryClass  GimpRatioEntryClass;

struct _GimpRatioEntry
{
  GtkEntry        parent_instance;

  gdouble         numerator;
  gdouble         denominator;
};

struct _GimpRatioEntryClass
{
  GtkEntryClass   parent_class;

  void (* ratio_changed) (GimpRatioEntry *entry);

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
};


GType           gimp_ratio_entry_get_type     (void) G_GNUC_CONST;

GtkWidget *     gimp_ratio_entry_new          (void);

void            gimp_ratio_entry_set_fraction (GimpRatioEntry *entry,
                                               gdouble         numerator,
                                               gdouble         denominator);
void            gimp_ratio_entry_get_fraction (GimpRatioEntry *entry,
                                               gdouble        *numerator,
                                               gdouble        *denominator);

void            gimp_ratio_entry_set_ratio    (GimpRatioEntry *entry,
                                               gdouble         ratio);
gdouble         gimp_ratio_entry_get_ratio    (GimpRatioEntry *entry);

void            gimp_ratio_entry_set_aspect   (GimpRatioEntry *entry,
                                               GimpAspectType  aspect);
GimpAspectType  gimp_ratio_entry_get_aspect   (GimpRatioEntry *entry);


G_END_DECLS

#endif /* __GIMP_RATIO_ENTRY_H__ */
