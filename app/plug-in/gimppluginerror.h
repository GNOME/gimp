/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __LIGMA_PLUG_IN_ERROR_H__
#define __LIGMA_PLUG_IN_ERROR_H__


typedef enum
{
  LIGMA_PLUG_IN_FAILED,  /* generic error condition */
  LIGMA_PLUG_IN_EXECUTION_FAILED,
  LIGMA_PLUG_IN_NOT_FOUND
} LigmaPlugInErrorCode;


#define LIGMA_PLUG_IN_ERROR (ligma_plug_in_error_quark ())

GQuark  ligma_plug_in_error_quark (void) G_GNUC_CONST;


#endif /* __LIGMA_PLUG_IN_ERROR_H__ */
