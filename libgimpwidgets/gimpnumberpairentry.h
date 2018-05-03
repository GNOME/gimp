/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpratioentry.h
 * Copyright (C) 2006  Simon Budig       <simon@gimp.org>
 * Copyright (C) 2007  Sven Neumann      <sven@gimp.org>
 * Copyright (C) 2007  Martin Nordholts  <martin@svn.gnome.org>
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

#ifndef __GIMP_NUMBER_PAIR_ENTRY_H__
#define __GIMP_NUMBER_PAIR_ENTRY_H__

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

  void (* numbers_changed) (GimpNumberPairEntry *entry);
  void (* ratio_changed)   (GimpNumberPairEntry *entry);

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


GType          gimp_number_pair_entry_get_type           (void) G_GNUC_CONST;
GtkWidget *    gimp_number_pair_entry_new                (const gchar         *separators,
                                                          gboolean             allow_simplification,
                                                          gdouble              min_valid_value,
                                                          gdouble              max_valid_value);
void           gimp_number_pair_entry_set_default_values (GimpNumberPairEntry *entry,
                                                          gdouble              left,
                                                          gdouble              right);
void           gimp_number_pair_entry_get_default_values (GimpNumberPairEntry *entry,
                                                          gdouble             *left,
                                                          gdouble             *right);
void           gimp_number_pair_entry_set_values         (GimpNumberPairEntry *entry,
                                                          gdouble              left,
                                                          gdouble              right);
void           gimp_number_pair_entry_get_values         (GimpNumberPairEntry *entry,
                                                          gdouble             *left,
                                                          gdouble             *right);

void           gimp_number_pair_entry_set_default_text   (GimpNumberPairEntry *entry,
                                                          const gchar         *string);
const gchar *  gimp_number_pair_entry_get_default_text   (GimpNumberPairEntry *entry);

void           gimp_number_pair_entry_set_ratio          (GimpNumberPairEntry *entry,
                                                          gdouble              ratio);
gdouble        gimp_number_pair_entry_get_ratio          (GimpNumberPairEntry *entry);

void           gimp_number_pair_entry_set_aspect         (GimpNumberPairEntry *entry,
                                                          GimpAspectType       aspect);
GimpAspectType gimp_number_pair_entry_get_aspect         (GimpNumberPairEntry *entry);

void           gimp_number_pair_entry_set_user_override  (GimpNumberPairEntry *entry,
                                                          gboolean             user_override);
gboolean       gimp_number_pair_entry_get_user_override  (GimpNumberPairEntry *entry);


G_END_DECLS

#endif /* __GIMP_NUMBER_PAIR_ENTRY_H__ */
