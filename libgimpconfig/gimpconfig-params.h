/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ParamSpecs for config objects
 * Copyright (C) 2001  Sven Neumann <sven@gimp.org>
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

#if !defined (__GIMP_CONFIG_H_INSIDE__) && !defined (GIMP_CONFIG_COMPILATION)
#error "Only <libgimpconfig/gimpconfig.h> can be included directly."
#endif

#ifndef __GIMP_CONFIG_PARAMS_H__
#define __GIMP_CONFIG_PARAMS_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/**
 * SECTION: gimpconfig-params
 * @title: GimpConfig-params
 * @short_description: Macros and defines to install config properties.
 *
 * Macros and defines to install config properties.
 **/


/*
 * GIMP_CONFIG_PARAM_SERIALIZE - A property that can and should be
 *                               serialized and deserialized.
 * GIMP_CONFIG_PARAM_AGGREGATE - The object property is to be treated as
 *                               part of the parent object.
 * GIMP_CONFIG_PARAM_RESTART   - Changes to this property take effect only
 *                               after a restart.
 * GIMP_CONFIG_PARAM_CONFIRM   - Changes to this property should be
 *                               confirmed by the user before being applied.
 * GIMP_CONFIG_PARAM_DEFAULTS  - Don't serialize this property if it has the
 *                               default value.
 * GIMP_CONFIG_PARAM_IGNORE    - This property exists for obscure reasons
 *                               or is needed for backward compatibility.
 *                               Ignore the value read and don't serialize it.
 */

#define GIMP_CONFIG_PARAM_SERIALIZE    (1 << (0 + G_PARAM_USER_SHIFT))
#define GIMP_CONFIG_PARAM_AGGREGATE    (1 << (1 + G_PARAM_USER_SHIFT))
#define GIMP_CONFIG_PARAM_RESTART      (1 << (2 + G_PARAM_USER_SHIFT))
#define GIMP_CONFIG_PARAM_CONFIRM      (1 << (3 + G_PARAM_USER_SHIFT))
#define GIMP_CONFIG_PARAM_DEFAULTS     (1 << (4 + G_PARAM_USER_SHIFT))
#define GIMP_CONFIG_PARAM_IGNORE       (1 << (5 + G_PARAM_USER_SHIFT))

#define GIMP_CONFIG_PARAM_FLAGS (G_PARAM_READWRITE | \
                                 G_PARAM_CONSTRUCT | \
                                 GIMP_CONFIG_PARAM_SERIALIZE)



/* some convenience macros to install object properties */

#define GIMP_CONFIG_PROP_BOOLEAN(class, id, name, nick, blurb, default, flags) \
  g_object_class_install_property (class, id,\
                                   g_param_spec_boolean (name, nick, blurb,\
                                   default,\
                                   flags | GIMP_CONFIG_PARAM_FLAGS))

#define GIMP_CONFIG_PROP_INT(class, id, name, nick, blurb, min, max, default, flags) \
  g_object_class_install_property (class, id,\
                                   g_param_spec_int (name, nick, blurb,\
                                   min, max, default,\
                                   flags | GIMP_CONFIG_PARAM_FLAGS))

#define GIMP_CONFIG_PROP_UINT(class, id, name, nick, blurb, min, max, default, flags) \
  g_object_class_install_property (class, id,\
                                   g_param_spec_uint (name, nick, blurb,\
                                   min, max, default,\
                                   flags | GIMP_CONFIG_PARAM_FLAGS))

#define GIMP_CONFIG_PROP_INT64(class, id, name, nick, blurb, min, max, default, flags) \
  g_object_class_install_property (class, id,\
                                   g_param_spec_int64 (name, nick, blurb,\
                                   min, max, default,\
                                   flags | GIMP_CONFIG_PARAM_FLAGS))

#define GIMP_CONFIG_PROP_UINT64(class, id, name, nick, blurb, min, max, default, flags) \
  g_object_class_install_property (class, id,\
                                   g_param_spec_uint64 (name, nick, blurb,\
                                   min, max, default,\
                                   flags | GIMP_CONFIG_PARAM_FLAGS))

#define GIMP_CONFIG_PROP_UNIT(class, id, name, nick, blurb, pixels, percent, default, flags) \
  g_object_class_install_property (class, id,\
                                   gimp_param_spec_unit (name, nick, blurb,\
                                   pixels, percent, default,\
                                   flags | GIMP_CONFIG_PARAM_FLAGS))

#define GIMP_CONFIG_PROP_MEMSIZE(class, id, name, nick, blurb, min, max, default, flags) \
  g_object_class_install_property (class, id,\
                                   gimp_param_spec_memsize (name, nick, blurb,\
                                   min, max, default,\
                                   flags | GIMP_CONFIG_PARAM_FLAGS))

#define GIMP_CONFIG_PROP_DOUBLE(class, id, name, nick, blurb, min, max, default, flags) \
  g_object_class_install_property (class, id,\
                                   g_param_spec_double (name, nick, blurb,\
                                   min, max, default,\
                                   flags | GIMP_CONFIG_PARAM_FLAGS))

#define GIMP_CONFIG_PROP_RESOLUTION(class, id, name, nick, blurb, default, flags) \
  g_object_class_install_property (class, id,\
                                   g_param_spec_double (name, nick, blurb,\
                                   GIMP_MIN_RESOLUTION, GIMP_MAX_RESOLUTION, \
                                   default,\
                                   flags | GIMP_CONFIG_PARAM_FLAGS))

#define GIMP_CONFIG_PROP_ENUM(class, id, name, nick, blurb, enum_type, default, flags) \
  g_object_class_install_property (class, id,\
                                   g_param_spec_enum (name, nick, blurb,\
                                   enum_type, default,\
                                   flags | GIMP_CONFIG_PARAM_FLAGS))

#define GIMP_CONFIG_PROP_STRING(class, id, name, nick, blurb, default, flags) \
  g_object_class_install_property (class, id,\
                                   g_param_spec_string (name, nick, blurb,\
                                   default,\
                                   flags | GIMP_CONFIG_PARAM_FLAGS))

#define GIMP_CONFIG_PROP_PATH(class, id, name, nick, blurb, path_type, default, flags) \
  g_object_class_install_property (class, id,\
                                   gimp_param_spec_config_path (name, nick, blurb,\
                                   path_type, default,\
                                   flags | GIMP_CONFIG_PARAM_FLAGS))

#define GIMP_CONFIG_PROP_RGB(class, id, name, nick, blurb, has_alpha, default, flags) \
  g_object_class_install_property (class, id,\
                                   gimp_param_spec_rgb (name, nick, blurb,\
                                   has_alpha, default, \
                                   flags | GIMP_CONFIG_PARAM_FLAGS))

#define GIMP_CONFIG_PROP_MATRIX2(class, id, name, nick, blurb, default, flags) \
  g_object_class_install_property (class, id,\
                                   gimp_param_spec_matrix2 (name, nick, blurb,\
                                   default,\
                                   flags | GIMP_CONFIG_PARAM_FLAGS))


/*  object, boxed and pointer properties are _not_ G_PARAM_CONSTRUCT  */

#define GIMP_CONFIG_PROP_OBJECT(class, id, name, nick, blurb, object_type, flags) \
  g_object_class_install_property (class, id,\
                                   g_param_spec_object (name, nick, blurb,\
                                   object_type,\
                                   flags |\
                                   G_PARAM_READWRITE |\
                                   GIMP_CONFIG_PARAM_SERIALIZE))

#define GIMP_CONFIG_PROP_BOXED(class, id, name, nick, blurb, boxed_type, flags) \
  g_object_class_install_property (class, id,\
                                   g_param_spec_boxed (name, nick, blurb,\
                                   boxed_type,\
                                   flags |\
                                   G_PARAM_READWRITE |\
                                   GIMP_CONFIG_PARAM_SERIALIZE))

#define GIMP_CONFIG_PROP_POINTER(class, id, name, nick, blurb, flags)    \
  g_object_class_install_property (class, id,\
                                   g_param_spec_pointer (name, nick, blurb,\
                                   flags |\
                                   G_PARAM_READWRITE |\
                                   GIMP_CONFIG_PARAM_SERIALIZE))


/*  deprecated macros, they all lack the "nick" parameter  */

#ifndef GIMP_DISABLE_DEPRECATED

#define GIMP_CONFIG_INSTALL_PROP_BOOLEAN(class, id, name, blurb, default, flags) \
  GIMP_CONFIG_PROP_BOOLEAN(class, id, name, NULL, blurb, default, flags);

#define GIMP_CONFIG_INSTALL_PROP_INT(class, id, name, blurb, min, max, default, flags) \
  GIMP_CONFIG_PROP_INT(class, id, name, NULL, blurb, min, max, default, flags)

#define GIMP_CONFIG_INSTALL_PROP_UINT(class, id, name, blurb, min, max, default, flags) \
  GIMP_CONFIG_PROP_UINT(class, id, name, NULL, blurb, min, max, default, flags)

#define GIMP_CONFIG_INSTALL_PROP_UNIT(class, id, name, blurb, pixels, percent, default, flags) \
  GIMP_CONFIG_PROP_UNIT(class, id, name, NULL, blurb, pixels, percent, default, flags)

#define GIMP_CONFIG_INSTALL_PROP_MEMSIZE(class, id, name, blurb, min, max, default, flags) \
  GIMP_CONFIG_PROP_MEMSIZE(class, id, name, NULL, blurb, min, max, default, flags)

#define GIMP_CONFIG_INSTALL_PROP_DOUBLE(class, id, name, blurb, min, max, default, flags) \
  GIMP_CONFIG_PROP_DOUBLE(class, id, name, NULL, blurb, min, max, default, flags)

#define GIMP_CONFIG_INSTALL_PROP_RESOLUTION(class, id, name, blurb, default, flags) \
  GIMP_CONFIG_PROP_RESOLUTION(class, id, name, NULL, blurb, default, flags)

#define GIMP_CONFIG_INSTALL_PROP_ENUM(class, id, name, blurb, enum_type, default, flags) \
  GIMP_CONFIG_PROP_ENUM(class, id, name, NULL, blurb, enum_type, default, flags)

#define GIMP_CONFIG_INSTALL_PROP_STRING(class, id, name, blurb, default, flags) \
  GIMP_CONFIG_PROP_STRING(class, id, name, NULL, blurb, default, flags)

#define GIMP_CONFIG_INSTALL_PROP_PATH(class, id, name, blurb, path_type, default, flags) \
  GIMP_CONFIG_PROP_PATH(class, id, name, NULL, blurb, path_type, default, flags)

#define GIMP_CONFIG_INSTALL_PROP_RGB(class, id, name, blurb, has_alpha, default, flags) \
  GIMP_CONFIG_PROP_RGB(class, id, name, NULL, blurb, has_alpha, default, flags)

#define GIMP_CONFIG_INSTALL_PROP_MATRIX2(class, id, name, blurb, default, flags) \
  GIMP_CONFIG_PROP_MATRIX2(class, id, name, NULL, blurb, default, flags)

#define GIMP_CONFIG_INSTALL_PROP_OBJECT(class, id, name, blurb, object_type, flags) \
  GIMP_CONFIG_PROP_OBJECT(class, id, name, NULL, blurb, object_type, flags)

#define GIMP_CONFIG_INSTALL_PROP_BOXED(class, id, name, blurb, boxed_type, flags) \
  GIMP_CONFIG_PROP_BOXED(class, id, name, NULL, blurb, boxed_type, flags)

#define GIMP_CONFIG_INSTALL_PROP_POINTER(class, id, name, blurb, flags) \
  GIMP_CONFIG_PROP_POINTER(class, id, name, NULL, blurb, flags)

#endif /* GIMP_DISABLE_DEPRECATED */


G_END_DECLS

#endif /* __GIMP_CONFIG_PARAMS_H__ */
