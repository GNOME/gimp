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

#ifndef __ACTIONS_TYPES_H__
#define __ACTIONS_TYPES_H__


#include "dialogs/dialogs-types.h"
#include "tools/tools-types.h"


typedef enum
{
  LIGMA_ACTION_SELECT_SET              =  0,
  LIGMA_ACTION_SELECT_SET_TO_DEFAULT   = -1,
  LIGMA_ACTION_SELECT_FIRST            = -2,
  LIGMA_ACTION_SELECT_LAST             = -3,
  LIGMA_ACTION_SELECT_SMALL_PREVIOUS   = -4,
  LIGMA_ACTION_SELECT_SMALL_NEXT       = -5,
  LIGMA_ACTION_SELECT_PREVIOUS         = -6,
  LIGMA_ACTION_SELECT_NEXT             = -7,
  LIGMA_ACTION_SELECT_SKIP_PREVIOUS    = -8,
  LIGMA_ACTION_SELECT_SKIP_NEXT        = -9,
  LIGMA_ACTION_SELECT_PERCENT_PREVIOUS = -10,
  LIGMA_ACTION_SELECT_PERCENT_NEXT     = -11
} LigmaActionSelectType;

typedef enum
{
  LIGMA_SAVE_MODE_SAVE,
  LIGMA_SAVE_MODE_SAVE_AS,
  LIGMA_SAVE_MODE_SAVE_A_COPY,
  LIGMA_SAVE_MODE_SAVE_AND_CLOSE,
  LIGMA_SAVE_MODE_EXPORT,
  LIGMA_SAVE_MODE_EXPORT_AS,
  LIGMA_SAVE_MODE_OVERWRITE
} LigmaSaveMode;


#endif /* __ACTIONS_TYPES_H__ */
