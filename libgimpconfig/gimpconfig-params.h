/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ParamSpecs for config objects
 * Copyright (C) 2001       Sven Neumann <sven@ligma.org>
 *               2001-2019  Michael Natterer <mitch@ligma.org>
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

#if !defined (__LIGMA_CONFIG_H_INSIDE__) && !defined (LIGMA_CONFIG_COMPILATION)
#error "Only <libligmaconfig/ligmaconfig.h> can be included directly."
#endif

#ifndef __LIGMA_CONFIG_PARAMS_H__
#define __LIGMA_CONFIG_PARAMS_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/**
 * LIGMA_CONFIG_PARAM_SERIALIZE:
 *
 * A property that can and should be serialized and deserialized.
 **/
#define LIGMA_CONFIG_PARAM_SERIALIZE    (1 << (0 + G_PARAM_USER_SHIFT))

/**
 * LIGMA_CONFIG_PARAM_AGGREGATE:
 *
 * The object property is to be treated as part of the parent object.
 **/
#define LIGMA_CONFIG_PARAM_AGGREGATE    (1 << (1 + G_PARAM_USER_SHIFT))

/**
 * LIGMA_CONFIG_PARAM_RESTART:
 *
 * Changes to this property take effect only after a restart.
 **/
#define LIGMA_CONFIG_PARAM_RESTART      (1 << (2 + G_PARAM_USER_SHIFT))

/**
 * LIGMA_CONFIG_PARAM_CONFIRM:
 *
 * Changes to this property should be confirmed by the user before
 * being applied.
 **/
#define LIGMA_CONFIG_PARAM_CONFIRM      (1 << (3 + G_PARAM_USER_SHIFT))

/**
 * LIGMA_CONFIG_PARAM_DEFAULTS:
 *
 * Don't serialize this property if it has the default value.
 **/
#define LIGMA_CONFIG_PARAM_DEFAULTS     (1 << (4 + G_PARAM_USER_SHIFT))

/**
 * LIGMA_CONFIG_PARAM_IGNORE:
 *
 * This property exists for obscure reasons or is needed for backward
 * compatibility. Ignore the value read and don't serialize it.
 **/
#define LIGMA_CONFIG_PARAM_IGNORE       (1 << (5 + G_PARAM_USER_SHIFT))

/**
 * LIGMA_CONFIG_PARAM_DONT_COMPARE:
 *
 * Ignore this property when comparing objects.
 **/
#define LIGMA_CONFIG_PARAM_DONT_COMPARE (1 << (6 + G_PARAM_USER_SHIFT))

/**
 * LIGMA_CONFIG_PARAM_FLAGS:
 *
 * The default flags that should be used for serializable #LigmaConfig
 * properties.
 **/
#define LIGMA_CONFIG_PARAM_FLAGS        (G_PARAM_READWRITE |             \
                                        G_PARAM_CONSTRUCT |             \
                                        G_PARAM_STATIC_STRINGS |        \
                                        LIGMA_CONFIG_PARAM_SERIALIZE)


/* some convenience macros to install object properties */

#define LIGMA_CONFIG_PROP_BOOLEAN(class, id, name, nick, blurb, default, flags) \
  g_object_class_install_property (class, id,\
                                   g_param_spec_boolean (name, nick, blurb,\
                                   default,\
                                   flags | LIGMA_CONFIG_PARAM_FLAGS))

#define LIGMA_CONFIG_PROP_INT(class, id, name, nick, blurb, min, max, default, flags) \
  g_object_class_install_property (class, id,\
                                   g_param_spec_int (name, nick, blurb,\
                                   min, max, default,\
                                   flags | LIGMA_CONFIG_PARAM_FLAGS))

#define LIGMA_CONFIG_PROP_UINT(class, id, name, nick, blurb, min, max, default, flags) \
  g_object_class_install_property (class, id,\
                                   g_param_spec_uint (name, nick, blurb,\
                                   min, max, default,\
                                   flags | LIGMA_CONFIG_PARAM_FLAGS))

#define LIGMA_CONFIG_PROP_INT64(class, id, name, nick, blurb, min, max, default, flags) \
  g_object_class_install_property (class, id,\
                                   g_param_spec_int64 (name, nick, blurb,\
                                   min, max, default,\
                                   flags | LIGMA_CONFIG_PARAM_FLAGS))

#define LIGMA_CONFIG_PROP_UINT64(class, id, name, nick, blurb, min, max, default, flags) \
  g_object_class_install_property (class, id,\
                                   g_param_spec_uint64 (name, nick, blurb,\
                                   min, max, default,\
                                   flags | LIGMA_CONFIG_PARAM_FLAGS))

#define LIGMA_CONFIG_PROP_UNIT(class, id, name, nick, blurb, pixels, percent, default, flags) \
  g_object_class_install_property (class, id,\
                                   ligma_param_spec_unit (name, nick, blurb,\
                                   pixels, percent, default,\
                                   flags | LIGMA_CONFIG_PARAM_FLAGS))

#define LIGMA_CONFIG_PROP_MEMSIZE(class, id, name, nick, blurb, min, max, default, flags) \
  g_object_class_install_property (class, id,\
                                   ligma_param_spec_memsize (name, nick, blurb,\
                                   min, max, default,\
                                   flags | LIGMA_CONFIG_PARAM_FLAGS))

#define LIGMA_CONFIG_PROP_DOUBLE(class, id, name, nick, blurb, min, max, default, flags) \
  g_object_class_install_property (class, id,\
                                   g_param_spec_double (name, nick, blurb,\
                                   min, max, default,\
                                   flags | LIGMA_CONFIG_PARAM_FLAGS))

#define LIGMA_CONFIG_PROP_RESOLUTION(class, id, name, nick, blurb, default, flags) \
  g_object_class_install_property (class, id,\
                                   g_param_spec_double (name, nick, blurb,\
                                   LIGMA_MIN_RESOLUTION, LIGMA_MAX_RESOLUTION, \
                                   default,\
                                   flags | LIGMA_CONFIG_PARAM_FLAGS))

#define LIGMA_CONFIG_PROP_ENUM(class, id, name, nick, blurb, enum_type, default, flags) \
  g_object_class_install_property (class, id,\
                                   g_param_spec_enum (name, nick, blurb,\
                                   enum_type, default,\
                                   flags | LIGMA_CONFIG_PARAM_FLAGS))

#define LIGMA_CONFIG_PROP_STRING(class, id, name, nick, blurb, default, flags) \
  g_object_class_install_property (class, id,\
                                   g_param_spec_string (name, nick, blurb,\
                                   default,\
                                   flags | LIGMA_CONFIG_PARAM_FLAGS))

#define LIGMA_CONFIG_PROP_PATH(class, id, name, nick, blurb, path_type, default, flags) \
  g_object_class_install_property (class, id,\
                                   ligma_param_spec_config_path (name, nick, blurb,\
                                   path_type, default,\
                                   flags | LIGMA_CONFIG_PARAM_FLAGS))

#define LIGMA_CONFIG_PROP_RGB(class, id, name, nick, blurb, has_alpha, default, flags) \
  g_object_class_install_property (class, id,\
                                   ligma_param_spec_rgb (name, nick, blurb,\
                                   has_alpha, default, \
                                   flags | LIGMA_CONFIG_PARAM_FLAGS))

#define LIGMA_CONFIG_PROP_MATRIX2(class, id, name, nick, blurb, default, flags) \
  g_object_class_install_property (class, id,\
                                   ligma_param_spec_matrix2 (name, nick, blurb,\
                                   default,\
                                   flags | LIGMA_CONFIG_PARAM_FLAGS))


/*  object, boxed and pointer properties are _not_ G_PARAM_CONSTRUCT  */

#define LIGMA_CONFIG_PROP_OBJECT(class, id, name, nick, blurb, object_type, flags) \
  g_object_class_install_property (class, id,\
                                   g_param_spec_object (name, nick, blurb,\
                                   object_type,\
                                   flags |\
                                   G_PARAM_READWRITE |\
                                   LIGMA_CONFIG_PARAM_SERIALIZE))

#define LIGMA_CONFIG_PROP_BOXED(class, id, name, nick, blurb, boxed_type, flags) \
  g_object_class_install_property (class, id,\
                                   g_param_spec_boxed (name, nick, blurb,\
                                   boxed_type,\
                                   flags |\
                                   G_PARAM_READWRITE |\
                                   LIGMA_CONFIG_PARAM_SERIALIZE))

#define LIGMA_CONFIG_PROP_POINTER(class, id, name, nick, blurb, flags)    \
  g_object_class_install_property (class, id,\
                                   g_param_spec_pointer (name, nick, blurb,\
                                   flags |\
                                   G_PARAM_READWRITE |\
                                   LIGMA_CONFIG_PARAM_SERIALIZE))


/*  create a copy of a GParamSpec  */

GParamSpec * ligma_config_param_spec_duplicate (GParamSpec *pspec);


G_END_DECLS

#endif /* __LIGMA_CONFIG_PARAMS_H__ */
