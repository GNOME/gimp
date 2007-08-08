/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpratioentry.h
 * Copyright (C) 2006  Simon Budig       <simon@gimp.org>
 * Copyright (C) 2007  Sven Neumann      <sven@gimp.org>
 * Copyright (C) 2007  Martin Nordholts  <martin@svn.gnome.org>
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

#ifndef GIMP_NUMBER_PAIR_ENTRY_H
#define GIMP_NUMBER_PAIR_ENTRY_H

G_BEGIN_DECLS


#define GIMP_TYPE_NUMBER_PAIR_ENTRY            (gimp_number_pair_entry_get_type ())
#define GIMP_NUMBER_PAIR_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_NUMBER_PAIR_ENTRY, GimpNumberPairEntry))
#define GIMP_NUMBER_PAIR_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_NUMBER_PAIR_ENTRY, GimpNumberPairEntryClass))
#define GIMP_IS_NUMBER_PAIR_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_NUMBER_PAIR_ENTRY))
#define GIMP_IS_NUMBER_PAIR_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_NUMBER_PAIR_ENTRY))
#define GIMP_NUMBER_PAIR_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_NUMBER_PAIR_AREA, GimpNumberPairEntryClass))


typedef struct _GimpNumberPairEntryPrivate GimpNumberPairEntryPrivate;
typedef struct _GimpNumberPairEntryClass   GimpNumberPairEntryClass;


struct _GimpNumberPairEntry
{
  GtkEntry                    parent_instance;

  GimpNumberPairEntryPrivate *priv;
};

struct _GimpNumberPairEntryClass
{
  GtkEntryClass  parent_class;

  void (*numbers_changed) (GimpNumberPairEntry *entry);
  void (*ratio_changed)   (GimpNumberPairEntry *entry);

  void (*padding_1) (void);
  void (*padding_2) (void);
  void (*padding_3) (void);
  void (*padding_4) (void);
};


GType           gimp_number_pair_entry_get_type             (void) G_GNUC_CONST;
GtkWidget *     gimp_number_pair_entry_new                  (const gchar         *separators,
                                                             gboolean             allow_simplification,
                                                             gdouble              min_valid_value,
                                                             gdouble              max_valid_value);
void            gimp_number_pair_entry_set_default_values   (GimpNumberPairEntry *entry,
                                                             gdouble              left,
                                                             gdouble              right);
void            gimp_number_pair_entry_set_values           (GimpNumberPairEntry *entry,
                                                             gdouble              left,
                                                             gdouble              right);
void            gimp_number_pair_entry_get_values           (GimpNumberPairEntry *entry,
                                                             gdouble             *left,
                                                             gdouble             *right);


G_END_DECLS

#endif /* GIMP_NUMBER_PAIR_ENTRY_H */
