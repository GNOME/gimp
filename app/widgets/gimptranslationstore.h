/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptranslationstore.h
 * Copyright (C) 2009  Sven Neumann <sven@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_TRANSLATION_STORE_H__
#define __GIMP_TRANSLATION_STORE_H__


#include "gimplanguagestore.h"


#define GIMP_TYPE_TRANSLATION_STORE            (gimp_translation_store_get_type ())
#define GIMP_TRANSLATION_STORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TRANSLATION_STORE, GimpTranslationStore))
#define GIMP_TRANSLATION_STORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TRANSLATION_STORE, GimpTranslationStoreClass))
#define GIMP_IS_TRANSLATION_STORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TRANSLATION_STORE))
#define GIMP_IS_TRANSLATION_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TRANSLATION_STORE))
#define GIMP_TRANSLATION_STORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TRANSLATION_STORE, GimpTranslationStoreClass))


typedef struct _GimpTranslationStoreClass  GimpTranslationStoreClass;


GType          gimp_translation_store_get_type   (void) G_GNUC_CONST;

void           gimp_translation_store_initialize (const gchar *system_lang_l10n);
GtkListStore * gimp_translation_store_new        (gboolean     manual_l18n,
                                                  const gchar *empty_label);


#endif  /* __GIMP_TRANSLATION_STORE_H__ */
