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
#include <gdk/gdkkeysyms.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "base/pixel-region.h"
#include "base/temp-buf.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimp.h"
#include "core/gimpbrush.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimptoolinfo.h"

#include "paint/gimpconvolveoptions.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimppropwidgets.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimpconvolvetool.h"
#include "gimppaintoptions-gui.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


#define FIELD_COLS     4
#define MIN_BLUR      64         /*  (8/9 original pixel)   */
#define MAX_BLUR       0.25      /*  (1/33 original pixel)  */
#define MIN_SHARPEN -512
#define MAX_SHARPEN  -64


static void   gimp_convolve_tool_class_init     (GimpConvolveToolClass *klass);
static void   gimp_convolve_tool_init           (GimpConvolveTool      *tool);

static void   gimp_convolve_tool_modifier_key   (GimpTool              *tool,
                                                 GdkModifierType        key,
                                                 gboolean               press,
                                                 GdkModifierType        state,
                                                 GimpDisplay           *gdisp);
static void   gimp_convolve_tool_cursor_update  (GimpTool              *tool,
                                                 GimpCoords            *coords,
                                                 GdkModifierType        state,
                                                 GimpDisplay           *gdisp);

static GtkWidget * gimp_convolve_options_gui    (GimpToolOptions       *options);


static GimpPaintToolClass *parent_class;


/*  public functions  */

void
gimp_convolve_tool_register (GimpToolRegisterCallback  callback,
                             gpointer                  data)
{
  (* callback) (GIMP_TYPE_CONVOLVE_TOOL,
                GIMP_TYPE_CONVOLVE_OPTIONS,
                gimp_convolve_options_gui,
                GIMP_PAINT_OPTIONS_CONTEXT_MASK,
                "gimp-convolve-tool",
                _("Convolve"),
                _("Blur or Sharpen"),
                N_("Con_volve"), "V",
                NULL, GIMP_HELP_TOOL_CONVOLVE,
                GIMP_STOCK_TOOL_BLUR,
                data);
}

GType
gimp_convolve_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpConvolveToolClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_convolve_tool_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpConvolveTool),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_convolve_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_PAINT_TOOL,
                                          "GimpConvolveTool",
                                          &tool_info, 0);
    }

  return tool_type;
}

/* static functions  */

static void
gimp_convolve_tool_class_init (GimpConvolveToolClass *klass)
{
  GimpToolClass *tool_class;

  tool_class = GIMP_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  tool_class->modifier_key  = gimp_convolve_tool_modifier_key;
  tool_class->cursor_update = gimp_convolve_tool_cursor_update;
}

static void
gimp_convolve_tool_init (GimpConvolveTool *convolve)
{
  GimpTool      *tool;
  GimpPaintTool *paint_tool;

  tool       = GIMP_TOOL (convolve);
  paint_tool = GIMP_PAINT_TOOL (convolve);

  gimp_tool_control_set_tool_cursor            (tool->control,
                                                GIMP_BLUR_TOOL_CURSOR);
  gimp_tool_control_set_toggle_tool_cursor     (tool->control,
                                                GIMP_BLUR_TOOL_CURSOR);
  gimp_tool_control_set_toggle_cursor_modifier (tool->control,
                                                GIMP_CURSOR_MODIFIER_MINUS);
}

static void
gimp_convolve_tool_modifier_key (GimpTool        *tool,
                                 GdkModifierType  key,
                                 gboolean         press,
				 GdkModifierType  state,
				 GimpDisplay     *gdisp)
{
  GimpConvolveOptions *options;

  options = GIMP_CONVOLVE_OPTIONS (tool->tool_info->tool_options);

  if ((key == GDK_CONTROL_MASK) &&
      ! (state & GDK_SHIFT_MASK)) /* leave stuff untouched in line draw mode */
    {
      switch (options->type)
        {
        case GIMP_BLUR_CONVOLVE:
          g_object_set (options, "type", GIMP_SHARPEN_CONVOLVE, NULL);
          break;

        case GIMP_SHARPEN_CONVOLVE:
          g_object_set (options, "type", GIMP_BLUR_CONVOLVE, NULL);
          break;

        default:
          break;
        }
    }
}

static void
gimp_convolve_tool_cursor_update (GimpTool        *tool,
                                  GimpCoords      *coords,
				  GdkModifierType  state,
				  GimpDisplay     *gdisp)
{
  GimpConvolveOptions *options;

  options = GIMP_CONVOLVE_OPTIONS (tool->tool_info->tool_options);

  gimp_tool_control_set_toggle (tool->control,
                                (options->type == GIMP_SHARPEN_CONVOLVE));

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, gdisp);
}


/*  tool options stuff  */

static GtkWidget *
gimp_convolve_options_gui (GimpToolOptions *tool_options)
{
  GObject   *config;
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *frame;
  gchar     *str;

  config = G_OBJECT (tool_options);

  vbox = gimp_paint_options_gui (tool_options);

  /*  the type radio box  */
  str = g_strdup_printf (_("Convolve Type  %s"),
                         gimp_get_mod_name_control ());

  frame = gimp_prop_enum_radio_frame_new (config, "type",
                                          str, 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  g_free (str);

  /*  the rate scale  */
  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  gimp_prop_scale_entry_new (config, "rate",
                             GTK_TABLE (table), 0, 0,
                             _("Rate:"),
                             1.0, 10.0, 1,
                             FALSE, 0.0, 0.0);

  return vbox;
}
