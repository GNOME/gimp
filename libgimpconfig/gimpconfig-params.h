/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ParamSpecs for config objects
 * Copyright (C) 2001  Sven Neumann <sven@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_CONFIG_PARAMS_H__
#define __GIMP_CONFIG_PARAMS_H__


/*
 * GIMP_PARAM_SERIALIZE - A property that can and should be
 *                        serialized and deserialized.
 * GIMP_PARAM_AGGREGATE - The object property is to be treated as
 *                        part of the parent object.
 * GIMP_PARAM_RESTART   - Changes to this property take effect only
 *                        after a restart.
 * GIMP_PARAM_CONFIRM   - Changes to this property should be
 *                        confirmed by the user before being applied.
 * GIMP_PARAM_DEFAULTS  - Don't serialize this property if it has the
 *                        default value.
 * GIMP_PARAM_IGNORE    - This property exists for obscure reasons
 *                        and is needed for backward compatibility.
 *                        Ignore the value read and don't serialize it.
 */

#define GIMP_PARAM_SERIALIZE    (1 << (0 + G_PARAM_USER_SHIFT))
#define GIMP_PARAM_AGGREGATE    (1 << (1 + G_PARAM_USER_SHIFT))
#define GIMP_PARAM_RESTART      (1 << (2 + G_PARAM_USER_SHIFT))
#define GIMP_PARAM_CONFIRM      (1 << (3 + G_PARAM_USER_SHIFT))
#define GIMP_PARAM_DEFAULTS     (1 << (4 + G_PARAM_USER_SHIFT))
#define GIMP_PARAM_IGNORE       (1 << (5 + G_PARAM_USER_SHIFT))

#define GIMP_CONFIG_PARAM_FLAGS (G_PARAM_READWRITE | \
                                 G_PARAM_CONSTRUCT | \
                                 GIMP_PARAM_SERIALIZE)


/*
 * GIMP_TYPE_PARAM_RGB
 */

#define GIMP_TYPE_PARAM_RGB               (gimp_param_rgb_get_type ())
#define GIMP_IS_PARAM_SPEC_RGB(pspec)     (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_RGB))

GType        gimp_param_rgb_get_type      (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_rgb          (const gchar    *name,
                                           const gchar    *nick,
                                           const gchar    *blurb,
                                           const GimpRGB  *default_value,
                                           GParamFlags     flags);


/*
 * GIMP_TYPE_PARAM_MATRIX2
 */

#define GIMP_TYPE_PARAM_MATRIX2            (gimp_param_matrix2_get_type ())
#define GIMP_IS_PARAM_SPEC_MATRIX2(pspec)  (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_MATRIX2))

GType        gimp_param_matrix2_get_type   (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_matrix2       (const gchar        *name,
                                            const gchar        *nick,
                                            const gchar        *blurb,
                                            const GimpMatrix2  *default_value,
                                            GParamFlags         flags);


/*
 * GIMP_TYPE_PARAM_MEMSIZE
 */

#define GIMP_TYPE_PARAM_MEMSIZE           (gimp_param_memsize_get_type ())
#define GIMP_IS_PARAM_SPEC_MEMSIZE(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_MEMSIZE))

GType        gimp_param_memsize_get_type  (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_memsize      (const gchar  *name,
                                           const gchar  *nick,
                                           const gchar  *blurb,
                                           guint64       minimum,
                                           guint64       maximum,
                                           guint64       default_value,
                                           GParamFlags   flags);


/*
 * GIMP_TYPE_PARAM_PATH
 */

typedef enum
{
  GIMP_PARAM_PATH_FILE,
  GIMP_PARAM_PATH_FILE_LIST,
  GIMP_PARAM_PATH_DIR,
  GIMP_PARAM_PATH_DIR_LIST
} GimpParamPathType;

#define GIMP_TYPE_PARAM_PATH              (gimp_param_path_get_type ())
#define GIMP_IS_PARAM_SPEC_PATH(pspec)    (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_PATH))

GType        gimp_param_path_get_type     (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_path         (const gchar        *name,
                                           const gchar        *nick,
                                           const gchar        *blurb,
					   GimpParamPathType   type,
                                           gchar              *default_value,
                                           GParamFlags         flags);

GimpParamPathType  gimp_param_spec_path_type (GParamSpec      *pspec);


/*
 * GIMP_TYPE_PARAM_UNIT
 */

#define GIMP_TYPE_PARAM_UNIT              (gimp_param_unit_get_type ())
#define GIMP_IS_PARAM_SPEC_UNIT(pspec)    (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_UNIT))

GType        gimp_param_unit_get_type     (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_unit         (const gchar  *name,
                                           const gchar  *nick,
                                           const gchar  *blurb,
                                           gboolean      allow_pixels,
                                           gboolean      allow_percent,
                                           GimpUnit      default_value,
                                           GParamFlags   flags);


/* some convenience macros to install object properties */

#define GIMP_CONFIG_INSTALL_PROP_BOOLEAN(class, id,\
                                         name, blurb, default, flags)\
  g_object_class_install_property (class, id,\
                                   g_param_spec_boolean (name, NULL, blurb,\
                                   default,\
                                   flags | GIMP_CONFIG_PARAM_FLAGS))
#define GIMP_CONFIG_INSTALL_PROP_RGB(class, id,\
                                     name, blurb, default, flags)\
  g_object_class_install_property (class, id,\
                                   gimp_param_spec_rgb (name, NULL, blurb,\
                                   default,\
                                   flags | GIMP_CONFIG_PARAM_FLAGS))
#define GIMP_CONFIG_INSTALL_PROP_DOUBLE(class, id,\
                                        name, blurb, min, max, default, flags)\
  g_object_class_install_property (class, id,\
                                   g_param_spec_double (name, NULL, blurb,\
                                   min, max, default,\
                                   flags | GIMP_CONFIG_PARAM_FLAGS))
#define GIMP_CONFIG_INSTALL_PROP_RESOLUTION(class, id,\
                                            name, blurb, default, flags)\
  g_object_class_install_property (class, id,\
                                   g_param_spec_double (name, NULL, blurb,\
                                   GIMP_MIN_RESOLUTION, GIMP_MAX_RESOLUTION, \
                                   default,\
                                   flags | GIMP_CONFIG_PARAM_FLAGS))
#define GIMP_CONFIG_INSTALL_PROP_ENUM(class, id,\
                                      name, blurb, enum_type, default, flags)\
  g_object_class_install_property (class, id,\
                                   g_param_spec_enum (name, NULL, blurb,\
                                   enum_type, default,\
                                   flags | GIMP_CONFIG_PARAM_FLAGS))
#define GIMP_CONFIG_INSTALL_PROP_INT(class, id,\
                                     name, blurb, min, max, default, flags)\
  g_object_class_install_property (class, id,\
                                   g_param_spec_int (name, NULL, blurb,\
                                   min, max, default,\
                                   flags | GIMP_CONFIG_PARAM_FLAGS))
#define GIMP_CONFIG_INSTALL_PROP_MATRIX2(class, id,\
                                        name, blurb, default, flags)\
  g_object_class_install_property (class, id,\
                                   gimp_param_spec_matrix2 (name, NULL, blurb,\
                                   default,\
                                   flags | GIMP_CONFIG_PARAM_FLAGS))
#define GIMP_CONFIG_INSTALL_PROP_MEMSIZE(class, id,\
                                         name, blurb, min, max, default, flags)\
  g_object_class_install_property (class, id,\
                                   gimp_param_spec_memsize (name, NULL, blurb,\
                                   min, max, default,\
                                   flags | GIMP_CONFIG_PARAM_FLAGS))
#define GIMP_CONFIG_INSTALL_PROP_PATH(class, id,\
                                      name, blurb, type, default, flags)\
  g_object_class_install_property (class, id,\
                                   gimp_param_spec_path (name, NULL, blurb,\
                                   type, default,\
                                   flags | GIMP_CONFIG_PARAM_FLAGS))
#define GIMP_CONFIG_INSTALL_PROP_STRING(class, id,\
                                        name, blurb, default, flags)\
  g_object_class_install_property (class, id,\
                                   g_param_spec_string (name, NULL, blurb,\
                                   default,\
                                   flags | GIMP_CONFIG_PARAM_FLAGS))
#define GIMP_CONFIG_INSTALL_PROP_UINT(class, id,\
                                      name, blurb, min, max, default, flags)\
  g_object_class_install_property (class, id,\
                                   g_param_spec_uint (name, NULL, blurb,\
                                   min, max, default,\
                                   flags | GIMP_CONFIG_PARAM_FLAGS))
#define GIMP_CONFIG_INSTALL_PROP_UNIT(class, id,\
                                      name, blurb, pixels, percent, default, flags)\
  g_object_class_install_property (class, id,\
                                   gimp_param_spec_unit (name, NULL, blurb,\
                                   pixels, percent, default,\
                                   flags | GIMP_CONFIG_PARAM_FLAGS))


/*  object and pointer properties are _not_ G_PARAM_CONSTRUCT  */

#define GIMP_CONFIG_INSTALL_PROP_OBJECT(class, id,\
                                        name, blurb, object_type, flags)\
  g_object_class_install_property (class, id,\
                                   g_param_spec_object (name, NULL, blurb,\
                                   object_type,\
                                   flags |\
                                   G_PARAM_READWRITE | GIMP_PARAM_SERIALIZE))

#define GIMP_CONFIG_INSTALL_PROP_POINTER(class, id,\
                                         name, blurb, flags)\
  g_object_class_install_property (class, id,\
                                   g_param_spec_pointer (name, NULL, blurb,\
                                   flags |\
                                   G_PARAM_READWRITE | GIMP_PARAM_SERIALIZE))


#endif /* __GIMP_CONFIG_PARAMS_H__ */
