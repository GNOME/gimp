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


#define GIMP_TYPE_PARAM_MEMSIZE           (gimp_param_memsize_get_type ())
#define GIMP_IS_PARAM_SPEC_MEMSIZE(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_SPEC_MEMSIZE))

GType        gimp_param_memsize_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_memsize     (const gchar    *name,
                                          const gchar    *nick,
                                          const gchar    *blurb,
                                          guint           minimum,
                                          guint           maximum,
                                          guint           default_value,
                                          GParamFlags     flags);


#define GIMP_TYPE_PARAM_PATH              (gimp_param_path_get_type ())
#define GIMP_IS_PARAM_SPEC_PATH(pspec)    (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_SPEC_PATH))

GType        gimp_param_path_get_type    (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_path        (const gchar    *name,
                                          const gchar    *nick,
                                          const gchar    *blurb,
                                          gchar          *default_value,
                                          GParamFlags     flags);


/* some convenience macros to install object properties */

#define GIMP_CONFIG_INSTALL_PROP_UINT(class, id, name, min, max, default)\
  g_object_class_install_property (class, id,\
                                   g_param_spec_uint (name, NULL, NULL,\
                                   min, max, default,\
                                   G_PARAM_READWRITE | G_PARAM_CONSTRUCT))
#define GIMP_CONFIG_INSTALL_PROP_ENUM(class, id, name, enum_type, default)\
  g_object_class_install_property (class, id,\
                                   g_param_spec_enum (name, NULL, NULL,\
                                   enum_type, default,\
                                   G_PARAM_READWRITE | G_PARAM_CONSTRUCT))
#define GIMP_CONFIG_INSTALL_PROP_BOOLEAN(class, id, name, default)\
  g_object_class_install_property (class, id,\
                                   g_param_spec_boolean (name, NULL, NULL,\
                                   default,\
                                   G_PARAM_READWRITE | G_PARAM_CONSTRUCT))
#define GIMP_CONFIG_INSTALL_PROP_MEMSIZE(class, id, name, min, max, default)\
  g_object_class_install_property (class, id,\
                                   gimp_param_spec_memsize (name, NULL, NULL,\
                                   min, max, default,\
                                   G_PARAM_READWRITE | G_PARAM_CONSTRUCT))
#define GIMP_CONFIG_INSTALL_PROP_PATH(class, id, name, default)\
  g_object_class_install_property (class, id,\
                                   gimp_param_spec_path (name, NULL, NULL,\
                                   default,\
                                   G_PARAM_READWRITE | G_PARAM_CONSTRUCT))


#endif /* __GIMP_CONFIG_PARAMS_H__ */
