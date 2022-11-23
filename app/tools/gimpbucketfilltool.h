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

#ifndef  __LIGMA_BUCKET_FILL_TOOL_H__
#define  __LIGMA_BUCKET_FILL_TOOL_H__


#include "ligmacolortool.h"


#define LIGMA_TYPE_BUCKET_FILL_TOOL            (ligma_bucket_fill_tool_get_type ())
#define LIGMA_BUCKET_FILL_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_BUCKET_FILL_TOOL, LigmaBucketFillTool))
#define LIGMA_BUCKET_FILL_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_BUCKET_FILL_TOOL, LigmaBucketFillToolClass))
#define LIGMA_IS_BUCKET_FILL_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_BUCKET_FILL_TOOL))
#define LIGMA_IS_BUCKET_FILL_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_BUCKET_FILL_TOOL))
#define LIGMA_BUCKET_FILL_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_BUCKET_FILL_TOOL, LigmaBucketFillToolClass))

#define LIGMA_BUCKET_FILL_TOOL_GET_OPTIONS(t)  (LIGMA_BUCKET_FILL_OPTIONS (ligma_tool_get_options (LIGMA_TOOL (t))))


typedef struct _LigmaBucketFillTool      LigmaBucketFillTool;
typedef struct _LigmaBucketFillToolClass LigmaBucketFillToolClass;
typedef struct _LigmaBucketFillToolPrivate LigmaBucketFillToolPrivate;

struct _LigmaBucketFillTool
{
  LigmaColorTool              parent_instance;

  LigmaBucketFillToolPrivate *priv;
};

struct _LigmaBucketFillToolClass
{
  LigmaColorToolClass  parent_class;
};


void    ligma_bucket_fill_tool_register (LigmaToolRegisterCallback  callback,
                                        gpointer                  data);

GType   ligma_bucket_fill_tool_get_type (void) G_GNUC_CONST;


#endif  /*  __LIGMA_BUCKET_FILL_TOOL_H__  */
