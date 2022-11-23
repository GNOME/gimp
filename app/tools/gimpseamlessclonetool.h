/* LIGMA - The GNU Image Manipulation Program
 *
 * ligmaseamlessclonetool.h
 * Copyright (C) 2011 Barak Itkin <lightningismyname@gmail.com>
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

#ifndef __LIGMA_SEAMLESS_CLONE_TOOL_H__
#define __LIGMA_SEAMLESS_CLONE_TOOL_H__


#include "ligmadrawtool.h"


#define LIGMA_TYPE_SEAMLESS_CLONE_TOOL            (ligma_seamless_clone_tool_get_type ())
#define LIGMA_SEAMLESS_CLONE_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_SEAMLESS_CLONE_TOOL, LigmaSeamlessCloneTool))
#define LIGMA_SEAMLESS_CLONE_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_SEAMLESS_CLONE_TOOL, LigmaSeamlessCloneToolClass))
#define LIGMA_IS_SEAMLESS_CLONE_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_SEAMLESS_CLONE_TOOL))
#define LIGMA_IS_SEAMLESS_CLONE_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_SEAMLESS_CLONE_TOOL))
#define LIGMA_SEAMLESS_CLONE_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_SEAMLESS_CLONE_TOOL, LigmaSeamlessCloneToolClass))

#define LIGMA_SEAMLESS_CLONE_TOOL_GET_OPTIONS(t)  (LIGMA_SEAMLESS_CLONE_OPTIONS (ligma_tool_get_options (LIGMA_TOOL (t))))


typedef struct _LigmaSeamlessCloneTool      LigmaSeamlessCloneTool;
typedef struct _LigmaSeamlessCloneToolClass LigmaSeamlessCloneToolClass;

struct _LigmaSeamlessCloneTool
{
  LigmaDrawTool    parent_instance;

  GeglBuffer     *paste;         /* A buffer containing the original
                                  * paste that will be used in the
                                  * rendering process */

  GeglNode       *render_node;    /* The parent of the Gegl graph that
                                   * renders the seamless cloning */

  GeglNode       *sc_node;        /* A Gegl node to do the seamless
                                   * cloning live with translation of
                                   * the paste */

  gint            tool_state;     /* The current state in the tool's
                                   * state machine */

  LigmaDrawableFilter *filter;     /* The filter object which renders
                                   * the live preview, and commits it
                                   * when at the end */

  gint width, height;             /* The width and height of the paste.
                                   * Needed for mouse hit detection */

  gint xoff, yoff;                /* The current offset of the paste */
  gint xoff_p, yoff_p;            /* The previous offset of the paste */

  gdouble xclick, yclick;         /* The image location of the last
                                   * mouse click. To be used when the
                                   * mouse is in motion, to recalculate
                                   * the xoff and yoff values */
};

struct _LigmaSeamlessCloneToolClass
{
  LigmaDrawToolClass parent_class;
};


void    ligma_seamless_clone_tool_register (LigmaToolRegisterCallback  callback,
                                           gpointer                  data);

GType   ligma_seamless_clone_tool_get_type (void) G_GNUC_CONST;


#endif  /*  __LIGMA_SEAMLESS_CLONE_TOOL_H__  */
