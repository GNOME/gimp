/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpextension.h
 * Copyright (C) 2018 Jehan <jehan@gimp.org>
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

#ifndef __GIMP_EXTENSION_H__
#define __GIMP_EXTENSION_H__


#include "core/gimpobject.h"


#define GIMP_TYPE_EXTENSION            (gimp_extension_get_type ())
#define GIMP_EXTENSION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_EXTENSION, GimpExtension))
#define GIMP_EXTENSION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_EXTENSION, GimpExtensionClass))
#define GIMP_IS_EXTENSION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_EXTENSION))
#define GIMP_IS_EXTENSION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_EXTENSION))
#define GIMP_EXTENSION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_EXTENSION, GimpExtensionClass))

typedef struct _GimpExtensionPrivate  GimpExtensionPrivate;
typedef struct _GimpExtensionClass    GimpExtensionClass;

struct _GimpExtension
{
  GimpObject            parent_instance;

  GimpExtensionPrivate *p;
};

struct _GimpExtensionClass
{
  GimpObjectClass       parent_class;
};


GType           gimp_extension_get_type (void) G_GNUC_CONST;

GimpExtension * gimp_extension_new                     (const gchar    *dir,
                                                        gboolean        writable);
const gchar   * gimp_extension_get_name                (GimpExtension  *extension);
const gchar   * gimp_extension_get_comment             (GimpExtension  *extension);
const gchar   * gimp_extension_get_description         (GimpExtension  *extension);
GdkPixbuf     * gimp_extension_get_screenshot          (GimpExtension  *extension,
                                                        gint            width,
                                                        gint            height,
                                                        const gchar   **caption);
const gchar   * gimp_extension_get_path                (GimpExtension  *extension);

gchar         * gimp_extension_get_markup_description  (GimpExtension  *extension);

gboolean        gimp_extension_load                    (GimpExtension  *extension,
                                                        GError        **error);
gboolean        gimp_extension_run                     (GimpExtension  *extension,
                                                        GError        **error);
void            gimp_extension_stop                    (GimpExtension  *extension);

GList         * gimp_extension_get_brush_paths         (GimpExtension  *extension);
GList         * gimp_extension_get_dynamics_paths      (GimpExtension  *extension);
GList         * gimp_extension_get_mypaint_brush_paths (GimpExtension  *extension);
GList         * gimp_extension_get_pattern_paths       (GimpExtension  *extension);
GList         * gimp_extension_get_gradient_paths      (GimpExtension  *extension);
GList         * gimp_extension_get_palette_paths       (GimpExtension  *extension);
GList         * gimp_extension_get_tool_preset_paths   (GimpExtension  *extension);
GList         * gimp_extension_get_splash_paths        (GimpExtension  *extension);
GList         * gimp_extension_get_theme_paths         (GimpExtension  *extension);
GList         * gimp_extension_get_plug_in_paths       (GimpExtension  *extension);

gint            gimp_extension_cmp                     (GimpExtension  *extension1,
                                                        GimpExtension  *extension2);
gint            gimp_extension_id_cmp                  (GimpExtension  *extension1,
                                                        const gchar    *id);

#endif  /* __GIMP_EXTENSION_H__ */
