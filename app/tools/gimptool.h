/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __GIMP_TOOL_H__
#define __GIMP_TOOL_H__


#include "core/gimpobject.h"


/*  The possibilities for where the cursor lies  */
#define  ACTIVE_LAYER      (1 << 0)
#define  SELECTION         (1 << 1)
#define  NON_ACTIVE_LAYER  (1 << 2)


#define GIMP_TYPE_TOOL            (gimp_tool_get_type ())
#define GIMP_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TOOL, GimpTool))
#define GIMP_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TOOL, GimpToolClass))
#define GIMP_IS_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TOOL))
#define GIMP_IS_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TOOL))
#define GIMP_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TOOL, GimpToolClass))


typedef struct _GimpToolClass GimpToolClass;

struct _GimpTool
{
  GimpObject	parent_instance;

  GimpToolInfo *tool_info;

  gint          ID;           /*  unique tool ID                              */

  ToolState     state;        /*  state of tool activity                      */
  gint          paused_count; /*  paused control count                        */
  gboolean      scroll_lock;  /*  allow scrolling or not                      */
  gboolean      auto_snap_to; /*  snap to guides automatically                */

  gboolean      preserve;     /*  Preserve this tool across drawable changes  */
  GimpDisplay  *gdisp;        /*  pointer to currently active gdisp           */
  GimpDrawable *drawable;     /*  pointer to the tool's current drawable      */

  GimpToolCursorType tool_cursor;
  GimpToolCursorType toggle_cursor;  /* one of these or both will go
				      * away once all cursor_update
				      * functions are properly
				      * virtualized
				      */

  gboolean           toggled; /*  Bad hack to let the paint_core show the
			       *  right toggle cursors
			       */
};

struct _GimpToolClass
{
  GimpObjectClass  parent_class;

  /*  virtual functions  */

  void (* initialize)     (GimpTool        *tool,
			   GimpDisplay     *gdisp);
  void (* control)        (GimpTool        *tool,
			   ToolAction       action,
			   GimpDisplay     *gdisp);

  void (* button_press)   (GimpTool        *tool,
                           GimpCoords      *coords,
                           guint32          time,
			   GdkModifierType  state,
			   GimpDisplay     *gdisp);
  void (* button_release) (GimpTool        *tool,
                           GimpCoords      *coords,
			   guint32          time,
                           GdkModifierType  state,
			   GimpDisplay     *gdisp);
  void (* motion)         (GimpTool        *tool,
                           GimpCoords      *coords,
                           guint32          time,
			   GdkModifierType  state,
			   GimpDisplay     *gdisp);

  void (* arrow_key)      (GimpTool        *tool,
			   GdkEventKey     *kevent,
			   GimpDisplay     *gdisp);
  void (* modifier_key)   (GimpTool        *tool,
                           GdkModifierType  key,
                           gboolean         press,
                           GdkModifierType  state,
			   GimpDisplay     *gdisp);

  void (* oper_update)    (GimpTool        *tool,
                           GimpCoords      *coords,
			   GdkModifierType  state,
			   GimpDisplay     *gdisp);
  void (* cursor_update)  (GimpTool        *tool,
                           GimpCoords      *coords,
			   GdkModifierType  state,
			   GimpDisplay     *gdisp);
};


GType         gimp_tool_get_type        (void) G_GNUC_CONST;

void          gimp_tool_initialize      (GimpTool        *tool,
					 GimpDisplay     *gdisp);
void	      gimp_tool_control         (GimpTool        *tool,
					 ToolAction       action,
					 GimpDisplay     *gdisp);

void          gimp_tool_button_press    (GimpTool        *tool,
                                         GimpCoords      *coords,
                                         guint32          time,
					 GdkModifierType  state,
					 GimpDisplay     *gdisp);
void          gimp_tool_button_release  (GimpTool        *tool,
                                         GimpCoords      *coords,
                                         guint32          time,
					 GdkModifierType  state,
					 GimpDisplay     *gdisp);
void          gimp_tool_motion          (GimpTool        *tool,
                                         GimpCoords      *coords,
                                         guint32          time,
					 GdkModifierType  state,
					 GimpDisplay     *gdisp);

void          gimp_tool_arrow_key       (GimpTool        *tool,
					 GdkEventKey     *kevent,
					 GimpDisplay     *gdisp);
void          gimp_tool_modifier_key    (GimpTool        *tool,
                                         GdkModifierType  key,
                                         gboolean         press,
					 GdkModifierType  state,
					 GimpDisplay     *gdisp);

void          gimp_tool_oper_update     (GimpTool        *tool,
                                         GimpCoords      *coords,
                                         GdkModifierType  state,
					 GimpDisplay     *gdisp);
void          gimp_tool_cursor_update   (GimpTool        *tool,
                                         GimpCoords      *coords,
					 GdkModifierType  state,
					 GimpDisplay     *gdisp);


#endif  /*  __GIMP_TOOL_H__  */
