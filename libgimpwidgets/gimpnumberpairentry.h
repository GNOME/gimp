/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaratioentry.h
 * Copyright (C) 2006  Simon Budig       <simon@ligma.org>
 * Copyright (C) 2007  Sven Neumann      <sven@ligma.org>
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
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__LIGMA_WIDGETS_H_INSIDE__) && !defined (LIGMA_WIDGETS_COMPILATION)
#error "Only <libligmawidgets/ligmawidgets.h> can be included directly."
#endif

#ifndef __LIGMA_NUMBER_PAIR_ENTRY_H__
#define __LIGMA_NUMBER_PAIR_ENTRY_H__

G_BEGIN_DECLS


#define LIGMA_TYPE_NUMBER_PAIR_ENTRY            (ligma_number_pair_entry_get_type ())
#define LIGMA_NUMBER_PAIR_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_NUMBER_PAIR_ENTRY, LigmaNumberPairEntry))
#define LIGMA_NUMBER_PAIR_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_NUMBER_PAIR_ENTRY, LigmaNumberPairEntryClass))
#define LIGMA_IS_NUMBER_PAIR_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_NUMBER_PAIR_ENTRY))
#define LIGMA_IS_NUMBER_PAIR_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_NUMBER_PAIR_ENTRY))
#define LIGMA_NUMBER_PAIR_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_NUMBER_PAIR_AREA, LigmaNumberPairEntryClass))


typedef struct _LigmaNumberPairEntryPrivate LigmaNumberPairEntryPrivate;
typedef struct _LigmaNumberPairEntryClass   LigmaNumberPairEntryClass;


struct _LigmaNumberPairEntry
{
  GtkEntry                    parent_instance;

  LigmaNumberPairEntryPrivate *priv;
};

struct _LigmaNumberPairEntryClass
{
  GtkEntryClass  parent_class;

  void (* numbers_changed) (LigmaNumberPairEntry *entry);
  void (* ratio_changed)   (LigmaNumberPairEntry *entry);

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


GType          ligma_number_pair_entry_get_type           (void) G_GNUC_CONST;
GtkWidget *    ligma_number_pair_entry_new                (const gchar         *separators,
                                                          gboolean             allow_simplification,
                                                          gdouble              min_valid_value,
                                                          gdouble              max_valid_value);
void           ligma_number_pair_entry_set_default_values (LigmaNumberPairEntry *entry,
                                                          gdouble              left,
                                                          gdouble              right);
void           ligma_number_pair_entry_get_default_values (LigmaNumberPairEntry *entry,
                                                          gdouble             *left,
                                                          gdouble             *right);
void           ligma_number_pair_entry_set_values         (LigmaNumberPairEntry *entry,
                                                          gdouble              left,
                                                          gdouble              right);
void           ligma_number_pair_entry_get_values         (LigmaNumberPairEntry *entry,
                                                          gdouble             *left,
                                                          gdouble             *right);

void           ligma_number_pair_entry_set_default_text   (LigmaNumberPairEntry *entry,
                                                          const gchar         *string);
const gchar *  ligma_number_pair_entry_get_default_text   (LigmaNumberPairEntry *entry);

void           ligma_number_pair_entry_set_ratio          (LigmaNumberPairEntry *entry,
                                                          gdouble              ratio);
gdouble        ligma_number_pair_entry_get_ratio          (LigmaNumberPairEntry *entry);

void           ligma_number_pair_entry_set_aspect         (LigmaNumberPairEntry *entry,
                                                          LigmaAspectType       aspect);
LigmaAspectType ligma_number_pair_entry_get_aspect         (LigmaNumberPairEntry *entry);

void           ligma_number_pair_entry_set_user_override  (LigmaNumberPairEntry *entry,
                                                          gboolean             user_override);
gboolean       ligma_number_pair_entry_get_user_override  (LigmaNumberPairEntry *entry);


G_END_DECLS

#endif /* __LIGMA_NUMBER_PAIR_ENTRY_H__ */
