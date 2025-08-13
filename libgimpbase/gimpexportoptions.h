/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpexportoptions.h
 * Copyright (C) 2024 Alx Sa.
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

#if !defined (__GIMP_BASE_H_INSIDE__) && !defined (GIMP_BASE_COMPILATION)
#error "Only <libgimpbase/gimpbase.h> can be included directly."
#endif


#ifndef __GIMP_EXPORT_OPTIONS_H__
#define __GIMP_EXPORT_OPTIONS_H__

G_BEGIN_DECLS


/* For information look into the C source or the html documentation */


#define GIMP_TYPE_EXPORT_OPTIONS               (gimp_export_options_get_type ())
#define GIMP_VALUE_HOLDS_EXPORT_OPTIONS(value) (G_TYPE_CHECK_VALUE_TYPE ((value), GIMP_TYPE_EXPORT_OPTIONS))
G_DECLARE_FINAL_TYPE (GimpExportOptions, gimp_export_options, GIMP, EXPORT_OPTIONS, GObject)



/*
 * GIMP_TYPE_PARAM_EXPORT_OPTIONS
 */

#define GIMP_TYPE_PARAM_EXPORT_OPTIONS           (gimp_param_export_options_get_type ())
#define GIMP_IS_PARAM_SPEC_EXPORT_OPTIONS(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_EXPORT_OPTIONS))


GType        gimp_param_export_options_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_export_options     (const gchar  *name,
                                                 const gchar  *nick,
                                                 const gchar  *blurb,
                                                 GParamFlags   flags);


G_END_DECLS

#endif /* __GIMP_EXPORT_OPTIONS_H__ */
