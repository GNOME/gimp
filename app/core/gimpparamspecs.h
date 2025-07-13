/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#pragma once


/*
 * GIMP_TYPE_PARAM_STRING
 */

#define GIMP_TYPE_PARAM_STRING           (gimp_param_string_get_type ())
#define GIMP_PARAM_SPEC_STRING(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_STRING, GimpParamSpecString))
#define GIMP_IS_PARAM_SPEC_STRING(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_STRING))

typedef struct _GimpParamSpecString GimpParamSpecString;

struct _GimpParamSpecString
{
  GParamSpecString parent_instance;

  guint            allow_non_utf8 : 1;
  guint            non_empty      : 1;
};

GType        gimp_param_string_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_string     (const gchar *name,
                                         const gchar *nick,
                                         const gchar *blurb,
                                         gboolean     allow_non_utf8,
                                         gboolean     null_ok,
                                         gboolean     non_empty,
                                         const gchar *default_value,
                                         GParamFlags  flags);


/*
 * GIMP_TYPE_PARAM_ENUM
 */

#define GIMP_TYPE_PARAM_ENUM           (gimp_param_enum_get_type ())
#define GIMP_PARAM_SPEC_ENUM(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_ENUM, GimpParamSpecEnum))

#define GIMP_IS_PARAM_SPEC_ENUM(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_ENUM))

typedef struct _GimpParamSpecEnum GimpParamSpecEnum;

struct _GimpParamSpecEnum
{
  GParamSpecEnum  parent_instance;

  GSList         *excluded_values;
};

GType        gimp_param_enum_get_type     (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_enum         (const gchar       *name,
                                           const gchar       *nick,
                                           const gchar       *blurb,
                                           GType              enum_type,
                                           gint               default_value,
                                           GParamFlags        flags);

void   gimp_param_spec_enum_exclude_value (GimpParamSpecEnum *espec,
                                           gint               value);


/*  include the declaration of the remaining paramspecs, they are
 *  identical app/ and libgimp/.
 */
#define GIMP_COMPILATION
#include "../../libgimp/gimpparamspecs.h"
#undef GIMP_COMPILATION
