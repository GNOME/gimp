/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * Copyright (C) 2003  Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_CONFIG_ERROR_H__
#define __LIGMA_CONFIG_ERROR_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/**
 * LigmaConfigError:
 * @LIGMA_CONFIG_ERROR_OPEN:        open failed
 * @LIGMA_CONFIG_ERROR_OPEN_ENOENT: file does not exist
 * @LIGMA_CONFIG_ERROR_WRITE:       write failed
 * @LIGMA_CONFIG_ERROR_PARSE:       parser error
 * @LIGMA_CONFIG_ERROR_VERSION:     parser failed due to version mismatch
 *
 * The possible values of a #GError thrown by libligmaconfig.
 **/
typedef enum
{
  LIGMA_CONFIG_ERROR_OPEN,
  LIGMA_CONFIG_ERROR_OPEN_ENOENT,
  LIGMA_CONFIG_ERROR_WRITE,
  LIGMA_CONFIG_ERROR_PARSE,
  LIGMA_CONFIG_ERROR_VERSION
} LigmaConfigError;

#define LIGMA_CONFIG_ERROR (ligma_config_error_quark ())

GQuark        ligma_config_error_quark   (void) G_GNUC_CONST;


G_END_DECLS

#endif  /* __LIGMA_CONFIG_ERROR_H__ */
