/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaconfig-path.h
 * Copyright (C) 2001-2002  Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_CONFIG_PATH_H__
#define __LIGMA_CONFIG_PATH_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/*
 * LIGMA_TYPE_CONFIG_PATH
 */

#define LIGMA_TYPE_CONFIG_PATH               (ligma_config_path_get_type ())
#define LIGMA_VALUE_HOLDS_CONFIG_PATH(value) (G_TYPE_CHECK_VALUE_TYPE ((value), LIGMA_TYPE_CONFIG_PATH))

GType               ligma_config_path_get_type        (void) G_GNUC_CONST;



/*
 * LIGMA_TYPE_PARAM_CONFIG_PATH
 */

/**
 * LigmaConfigPathType:
 * @LIGMA_CONFIG_PATH_FILE:      A single file
 * @LIGMA_CONFIG_PATH_FILE_LIST: A list of files
 * @LIGMA_CONFIG_PATH_DIR:       A single folder
 * @LIGMA_CONFIG_PATH_DIR_LIST:  A list of folders
 *
 * Types of config paths.
 **/
typedef enum
{
  LIGMA_CONFIG_PATH_FILE,
  LIGMA_CONFIG_PATH_FILE_LIST,
  LIGMA_CONFIG_PATH_DIR,
  LIGMA_CONFIG_PATH_DIR_LIST
} LigmaConfigPathType;


#define LIGMA_TYPE_PARAM_CONFIG_PATH            (ligma_param_config_path_get_type ())
#define LIGMA_IS_PARAM_SPEC_CONFIG_PATH(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), LIGMA_TYPE_PARAM_CONFIG_PATH))

GType               ligma_param_config_path_get_type  (void) G_GNUC_CONST;

GParamSpec        * ligma_param_spec_config_path      (const gchar  *name,
                                                      const gchar  *nick,
                                                      const gchar  *blurb,
                                                      LigmaConfigPathType  type,
                                                      const gchar  *default_value,
                                                      GParamFlags   flags);

LigmaConfigPathType  ligma_param_spec_config_path_type (GParamSpec   *pspec);


/*
 * LigmaConfigPath utilities
 */

gchar             * ligma_config_path_expand          (const gchar  *path,
                                                      gboolean      recode,
                                                      GError      **error) G_GNUC_MALLOC;
GList             * ligma_config_path_expand_to_files (const gchar  *path,
                                                      GError      **error) G_GNUC_MALLOC;

gchar             * ligma_config_path_unexpand        (const gchar  *path,
                                                      gboolean      recode,
                                                      GError      **error) G_GNUC_MALLOC;

GFile             * ligma_file_new_for_config_path    (const gchar  *path,
                                                      GError      **error) G_GNUC_MALLOC;
gchar             * ligma_file_get_config_path        (GFile        *file,
                                                      GError      **error) G_GNUC_MALLOC;

gchar             * ligma_config_build_data_path      (const gchar  *name) G_GNUC_MALLOC;
gchar             * ligma_config_build_writable_path  (const gchar  *name) G_GNUC_MALLOC;
gchar             * ligma_config_build_plug_in_path   (const gchar  *name) G_GNUC_MALLOC;
gchar             * ligma_config_build_system_path    (const gchar  *name) G_GNUC_MALLOC;


G_END_DECLS

#endif /* __LIGMA_CONFIG_PATH_H__ */
