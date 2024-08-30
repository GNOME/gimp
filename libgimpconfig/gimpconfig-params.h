/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ParamSpecs for config objects
 * Copyright (C) 2001       Sven Neumann <sven@gimp.org>
 *               2001-2019  Michael Natterer <mitch@gimp.org>
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
 * GIMP_CONFIG_PARAM_SERIALIZE:
 *
 * A property that can and should be serialized and deserialized.
 **/
#define GIMP_CONFIG_PARAM_SERIALIZE    (1 << (0 + GIMP_PARAM_FLAG_SHIFT))

/**
 * GIMP_CONFIG_PARAM_AGGREGATE:
 *
 * The object property is to be treated as part of the parent object.
 **/
#define GIMP_CONFIG_PARAM_AGGREGATE    (1 << (1 + GIMP_PARAM_FLAG_SHIFT))

/**
 * GIMP_CONFIG_PARAM_RESTART:
 *
 * Changes to this property take effect only after a restart.
 **/
#define GIMP_CONFIG_PARAM_RESTART      (1 << (2 + GIMP_PARAM_FLAG_SHIFT))

/**
 * GIMP_CONFIG_PARAM_CONFIRM:
 *
 * Changes to this property should be confirmed by the user before
 * being applied.
 **/
#define GIMP_CONFIG_PARAM_CONFIRM      (1 << (3 + GIMP_PARAM_FLAG_SHIFT))

/**
 * GIMP_CONFIG_PARAM_DEFAULTS:
 *
 * Don't serialize this property if it has the default value.
 **/
#define GIMP_CONFIG_PARAM_DEFAULTS     (1 << (4 + GIMP_PARAM_FLAG_SHIFT))

/**
 * GIMP_CONFIG_PARAM_IGNORE:
 *
 * This property exists for obscure reasons or is needed for backward
 * compatibility. Ignore the value read and don't serialize it.
 **/
#define GIMP_CONFIG_PARAM_IGNORE       (1 << (5 + GIMP_PARAM_FLAG_SHIFT))

/**
 * GIMP_CONFIG_PARAM_DONT_COMPARE:
 *
 * Ignore this property when comparing objects.
 **/
#define GIMP_CONFIG_PARAM_DONT_COMPARE (1 << (6 + GIMP_PARAM_FLAG_SHIFT))

/**
 * GIMP_CONFIG_PARAM_FLAG_SHIFT:
 *
 * Minimum shift count to be used for core application defined
 * [flags@GObject.ParamFlags].
 */
#define GIMP_CONFIG_PARAM_FLAG_SHIFT   (7 + GIMP_PARAM_FLAG_SHIFT)

/**
 * GIMP_CONFIG_PARAM_FLAGS:
 *
 * The default flags that should be used for serializable #GimpConfig
 * properties.
 **/
#define GIMP_CONFIG_PARAM_FLAGS        (G_PARAM_READWRITE |             \
                                        G_PARAM_CONSTRUCT |             \
                                        G_PARAM_STATIC_STRINGS |        \
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

#define GIMP_CONFIG_PROP_MATRIX2(class, id, name, nick, blurb, default, flags) \
  g_object_class_install_property (class, id,\
                                   gimp_param_spec_matrix2 (name, nick, blurb,\
                                   default,\
                                   flags | GIMP_CONFIG_PARAM_FLAGS))

#define GIMP_CONFIG_PROP_FONT(class, id, name, nick, blurb, flags) \
  g_object_class_install_property (class, id,\
                                   g_param_spec_object (name, nick, blurb,\
                                   GIMP_TYPE_FONT,\
                                   flags | GIMP_CONFIG_PARAM_FLAGS))

/*  object, boxed and pointer properties are _not_ G_PARAM_CONSTRUCT  */

#define GIMP_CONFIG_PROP_OBJECT(class, id, name, nick, blurb, object_type, flags) \
  g_object_class_install_property (class, id,\
                                   g_param_spec_object (name, nick, blurb,\
                                   object_type,\
                                   flags |\
                                   G_PARAM_READWRITE |\
                                   GIMP_CONFIG_PARAM_SERIALIZE))

#define GIMP_CONFIG_PROP_COLOR(class, id, name, nick, blurb, has_alpha, default, flags) \
  g_object_class_install_property (class, id,\
                                   gimp_param_spec_color (name, nick, blurb,\
                                   has_alpha, default,\
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


/*  create a copy of a GParamSpec  */

GParamSpec * gimp_config_param_spec_duplicate (GParamSpec *pspec);


G_END_DECLS

#endif /* __GIMP_CONFIG_PARAMS_H__ */
