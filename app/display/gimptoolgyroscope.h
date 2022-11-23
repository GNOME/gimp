/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatoolgyroscope.h
 * Copyright (C) 2018 Ell
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

#ifndef __LIGMA_TOOL_GYROSCOPE_H__
#define __LIGMA_TOOL_GYROSCOPE_H__


#include "ligmatoolwidget.h"


#define LIGMA_TYPE_TOOL_GYROSCOPE            (ligma_tool_gyroscope_get_type ())
#define LIGMA_TOOL_GYROSCOPE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TOOL_GYROSCOPE, LigmaToolGyroscope))
#define LIGMA_TOOL_GYROSCOPE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TOOL_GYROSCOPE, LigmaToolGyroscopeClass))
#define LIGMA_IS_TOOL_GYROSCOPE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TOOL_GYROSCOPE))
#define LIGMA_IS_TOOL_GYROSCOPE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TOOL_GYROSCOPE))
#define LIGMA_TOOL_GYROSCOPE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TOOL_GYROSCOPE, LigmaToolGyroscopeClass))


typedef struct _LigmaToolGyroscope        LigmaToolGyroscope;
typedef struct _LigmaToolGyroscopePrivate LigmaToolGyroscopePrivate;
typedef struct _LigmaToolGyroscopeClass   LigmaToolGyroscopeClass;

struct _LigmaToolGyroscope
{
  LigmaToolWidget            parent_instance;

  LigmaToolGyroscopePrivate *private;
};

struct _LigmaToolGyroscopeClass
{
  LigmaToolWidgetClass  parent_class;
};


GType            ligma_tool_gyroscope_get_type (void) G_GNUC_CONST;

LigmaToolWidget * ligma_tool_gyroscope_new      (LigmaDisplayShell *shell);


#endif /* __LIGMA_TOOL_GYROSCOPE_H__ */
