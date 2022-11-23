/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmaextension.h
 * Copyright (C) 2018 Jehan <jehan@ligma.org>
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

#ifndef __LIGMA_EXTENSION_H__
#define __LIGMA_EXTENSION_H__


#include "core/ligmaobject.h"


#define LIGMA_TYPE_EXTENSION            (ligma_extension_get_type ())
#define LIGMA_EXTENSION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_EXTENSION, LigmaExtension))
#define LIGMA_EXTENSION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_EXTENSION, LigmaExtensionClass))
#define LIGMA_IS_EXTENSION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_EXTENSION))
#define LIGMA_IS_EXTENSION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_EXTENSION))
#define LIGMA_EXTENSION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_EXTENSION, LigmaExtensionClass))

typedef struct _LigmaExtensionPrivate  LigmaExtensionPrivate;
typedef struct _LigmaExtensionClass    LigmaExtensionClass;

struct _LigmaExtension
{
  LigmaObject            parent_instance;

  LigmaExtensionPrivate *p;
};

struct _LigmaExtensionClass
{
  LigmaObjectClass       parent_class;
};


GType           ligma_extension_get_type (void) G_GNUC_CONST;

LigmaExtension * ligma_extension_new                     (const gchar    *dir,
                                                        gboolean        writable);
const gchar   * ligma_extension_get_name                (LigmaExtension  *extension);
const gchar   * ligma_extension_get_comment             (LigmaExtension  *extension);
const gchar   * ligma_extension_get_description         (LigmaExtension  *extension);
GdkPixbuf     * ligma_extension_get_screenshot          (LigmaExtension  *extension,
                                                        gint            width,
                                                        gint            height,
                                                        const gchar   **caption);
const gchar   * ligma_extension_get_path                (LigmaExtension  *extension);

gchar         * ligma_extension_get_markup_description  (LigmaExtension  *extension);

gboolean        ligma_extension_load                    (LigmaExtension  *extension,
                                                        GError        **error);
gboolean        ligma_extension_run                     (LigmaExtension  *extension,
                                                        GError        **error);
void            ligma_extension_stop                    (LigmaExtension  *extension);

GList         * ligma_extension_get_brush_paths         (LigmaExtension  *extension);
GList         * ligma_extension_get_dynamics_paths      (LigmaExtension  *extension);
GList         * ligma_extension_get_mypaint_brush_paths (LigmaExtension  *extension);
GList         * ligma_extension_get_pattern_paths       (LigmaExtension  *extension);
GList         * ligma_extension_get_gradient_paths      (LigmaExtension  *extension);
GList         * ligma_extension_get_palette_paths       (LigmaExtension  *extension);
GList         * ligma_extension_get_tool_preset_paths   (LigmaExtension  *extension);
GList         * ligma_extension_get_splash_paths        (LigmaExtension  *extension);
GList         * ligma_extension_get_theme_paths         (LigmaExtension  *extension);
GList         * ligma_extension_get_plug_in_paths       (LigmaExtension  *extension);

gint            ligma_extension_cmp                     (LigmaExtension  *extension1,
                                                        LigmaExtension  *extension2);
gint            ligma_extension_id_cmp                  (LigmaExtension  *extension1,
                                                        const gchar    *id);

#endif  /* __LIGMA_EXTENSION_H__ */
