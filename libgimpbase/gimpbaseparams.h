/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_BASE_PARAMS_H__
#define __GIMP_BASE_PARAMS_H__


/* For information look into the C source or the html documentation */

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


#endif  /* __GIMP_BASE_PARAMS_H__ */
