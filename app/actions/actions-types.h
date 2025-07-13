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

#include "dialogs/dialogs-types.h"
#include "tools/tools-types.h"


typedef enum
{
  GIMP_ACTION_SELECT_SET              =  0,
  GIMP_ACTION_SELECT_SET_TO_DEFAULT   = -1,
  GIMP_ACTION_SELECT_FIRST            = -2,
  GIMP_ACTION_SELECT_LAST             = -3,
  GIMP_ACTION_SELECT_SMALL_PREVIOUS   = -4,
  GIMP_ACTION_SELECT_SMALL_NEXT       = -5,
  GIMP_ACTION_SELECT_PREVIOUS         = -6,
  GIMP_ACTION_SELECT_NEXT             = -7,
  GIMP_ACTION_SELECT_SKIP_PREVIOUS    = -8,
  GIMP_ACTION_SELECT_SKIP_NEXT        = -9,
  GIMP_ACTION_SELECT_PERCENT_PREVIOUS = -10,
  GIMP_ACTION_SELECT_PERCENT_NEXT     = -11,
  GIMP_ACTION_SELECT_FLAT_PREVIOUS    = -12,
  GIMP_ACTION_SELECT_FLAT_NEXT        = -13,
} GimpActionSelectType;

typedef enum
{
  GIMP_SAVE_MODE_SAVE,
  GIMP_SAVE_MODE_SAVE_AS,
  GIMP_SAVE_MODE_SAVE_A_COPY,
  GIMP_SAVE_MODE_SAVE_AND_CLOSE,
  GIMP_SAVE_MODE_EXPORT,
  GIMP_SAVE_MODE_EXPORT_AS,
  GIMP_SAVE_MODE_OVERWRITE
} GimpSaveMode;
