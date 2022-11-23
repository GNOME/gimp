/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * vectors-enums.h
 * Copyright (C) 2006 Simon Budig  <simon@ligma.org>
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

#ifndef __VECTORS_ENUMS_H__
#define __VECTORS_ENUMS_H__


typedef enum
{
  LIGMA_ANCHOR_ANCHOR,
  LIGMA_ANCHOR_CONTROL
} LigmaAnchorType;

typedef enum
{
  LIGMA_ANCHOR_FEATURE_NONE,
  LIGMA_ANCHOR_FEATURE_EDGE,
  LIGMA_ANCHOR_FEATURE_ALIGNED,
  LIGMA_ANCHOR_FEATURE_SYMMETRIC
} LigmaAnchorFeatureType;

typedef enum
{
  EXTEND_SIMPLE,
  EXTEND_EDITABLE
} LigmaVectorExtendMode;


#endif /* __VECTORS_ENUMS_H__ */
