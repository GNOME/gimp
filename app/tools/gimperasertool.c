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

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "core/core-types.h"
#include "libgimptool/gimptooltypes.h"

#include "core/gimptoolinfo.h"

#include "paint/gimperaser.h"

#include "gimperasertool.h"
#include "paint_options.h"

#include "libgimp/gimpintl.h"


static void   gimp_eraser_tool_class_init    (GimpEraserToolClass  *klass);
static void   gimp_eraser_tool_init          (GimpEraserTool       *eraser);

static void   gimp_eraser_tool_modifier_key  (GimpTool             *tool,
                                              GdkModifierType       key,
                                              gboolean              press,
                                              GdkModifierType       state,
                                              GimpDisplay          *gdisp);
static void   gimp_eraser_tool_cursor_update (GimpTool             *tool,
                                              GimpCoords           *coords,
                                              GdkModifierType       state,
                                              GimpDisplay          *gdisp);

static GimpToolOptions * gimp_eraser_tool_options_new   (GimpToolInfo    *tool_info);
static void              gimp_eraser_tool_options_reset (GimpToolOptions *tool_options);


static GimpPaintToolClass *parent_class = NULL;


void
gimp_eraser_tool_register (GimpToolRegisterCallback  callback,
                           Gimp                     *gimp)
{
  (* callback) (GIMP_TYPE_ERASER_TOOL,
                gimp_eraser_tool_options_new,
                TRUE,
                "gimp-eraser-tool",
                _("Eraser"),
                _("Erase to background or transparency"),
                N_("/Tools/Paint Tools/Eraser"), "<shift>E",
                NULL, "tools/eraser.html",
                GIMP_STOCK_TOOL_ERASER,
                gimp);
}

GType
gimp_eraser_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpEraserToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_eraser_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpEraserTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_eraser_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_PAINT_TOOL,
					  "GimpEraserTool", 
                                          &tool_info, 0);
    }

  return tool_type;
}

static void
gimp_eraser_tool_class_init (GimpEraserToolClass *klass)
{
  GimpToolClass *tool_class;

  tool_class = GIMP_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  tool_class->modifier_key  = gimp_eraser_tool_modifier_key;
  tool_class->cursor_update = gimp_eraser_tool_cursor_update;
}

static void
gimp_eraser_tool_init (GimpEraserTool *eraser)
{
  GimpTool      *tool;
  GimpPaintTool *paint_tool;

  tool       = GIMP_TOOL (eraser);
  paint_tool = GIMP_PAINT_TOOL (eraser);

  tool->control = gimp_tool_control_new  (FALSE,                      /* scroll_lock */
                                          TRUE,                       /* auto_snap_to */
                                          TRUE,                       /* preserve */
                                          FALSE,                      /* handle_empty_image */
                                          GIMP_MOTION_MODE_EXACT,     /* motion_mode */
                                          GIMP_MOUSE_CURSOR,          /* cursor */
                                          GIMP_ERASER_TOOL_CURSOR,    /* tool_cursor */
                                          GIMP_CURSOR_MODIFIER_NONE,  /* cursor_modifier */
                                          GIMP_MOUSE_CURSOR,          /* toggle_cursor */
                                          GIMP_ERASER_TOOL_CURSOR,    /* toggle_tool_cursor */
                                          GIMP_CURSOR_MODIFIER_MINUS  /* toggle_cursor_modifier */);

  paint_tool->core = g_object_new (GIMP_TYPE_ERASER, NULL);
}

static void
gimp_eraser_tool_modifier_key (GimpTool        *tool,
                               GdkModifierType  key,
                               gboolean         press,
                               GdkModifierType  state,
                               GimpDisplay     *gdisp)
{
  GimpEraserOptions *options;

  options = (GimpEraserOptions *) tool->tool_info->tool_options;

  if ((key == GDK_CONTROL_MASK) &&
      ! (state & GDK_SHIFT_MASK)) /* leave stuff untouched in line draw mode */
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->anti_erase_w),
                                    ! options->anti_erase);
    }
}
  
static void
gimp_eraser_tool_cursor_update (GimpTool        *tool,
                                GimpCoords      *coords,
                                GdkModifierType  state,
                                GimpDisplay     *gdisp)
{
  GimpEraserOptions *options;

  options = (GimpEraserOptions *) tool->tool_info->tool_options;

  gimp_tool_control_set_toggle(tool->control, options->anti_erase);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, gdisp);
}


/*  tool options stuff  */

static GimpToolOptions *
gimp_eraser_tool_options_new (GimpToolInfo *tool_info)
{
  GimpEraserOptions *options;
  GtkWidget         *vbox;

  options = gimp_eraser_options_new ();

  paint_options_init ((GimpPaintOptions *) options, tool_info);

  ((GimpToolOptions *) options)->reset_func = gimp_eraser_tool_options_reset;

  /*  the main vbox  */
  vbox = ((GimpToolOptions *) options)->main_vbox;

  /* the anti_erase toggle */
  options->anti_erase_w =
    gtk_check_button_new_with_label (_("Anti Erase (<Ctrl>)"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->anti_erase_w),
				options->anti_erase_d);
  gtk_box_pack_start (GTK_BOX (vbox), options->anti_erase_w, FALSE, FALSE, 0);
  gtk_widget_show (options->anti_erase_w);

  g_signal_connect (G_OBJECT (options->anti_erase_w), "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &options->anti_erase);

  /* the hard toggle */
  options->hard_w = gtk_check_button_new_with_label (_("Hard Edge"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->hard_w),
				options->hard_d);
  gtk_box_pack_start (GTK_BOX (vbox), options->hard_w, FALSE, FALSE, 0);
  gtk_widget_show (options->hard_w);

  g_signal_connect (G_OBJECT (options->hard_w), "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &options->hard);

  return (GimpToolOptions *) options;
}

static void
gimp_eraser_tool_options_reset (GimpToolOptions *tool_options)
{
  GimpEraserOptions *options;

  options = (GimpEraserOptions *) tool_options;

  paint_options_reset (tool_options);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->anti_erase_w),
				options->anti_erase_d);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->hard_w),
				options->hard_d);
}
