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

#include "tools-types.h"

#include "core/gimp.h"
#include "core/gimpgradient.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdnd.h"
#include "widgets/gimpdock.h"
#include "widgets/gimpenummenu.h"
#include "widgets/gimppreview.h"

#include "gimpblendoptions.h"
#include "paint_options.h"

#include "libgimp/gimpintl.h"


static void   gimp_blend_options_init       (GimpBlendOptions      *options);
static void   gimp_blend_options_class_init (GimpBlendOptionsClass *options_class);

static void   gimp_blend_options_reset       (GimpToolOptions  *tool_options);
static void   blend_options_gradient_clicked (GtkWidget        *widget,
                                              gpointer          data);
static void   gradient_type_callback         (GtkWidget        *widget,
                                              GimpBlendOptions *options);
static void   blend_options_drop_tool        (GtkWidget        *widget,
                                              GimpViewable     *viewable,
                                              gpointer          data);
static void   blend_options_drop_gradient    (GtkWidget        *widget,
                                              GimpViewable     *viewable,
                                              gpointer          data);


GType
gimp_blend_options_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (GimpBlendOptionsClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_blend_options_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpBlendOptions),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_blend_options_init,
      };

      type = g_type_register_static (GIMP_TYPE_PAINT_OPTIONS,
                                     "GimpBlendOptions",
                                     &info, 0);
    }

  return type;
}

static void 
gimp_blend_options_class_init (GimpBlendOptionsClass *klass)
{
  GimpToolOptionsClass *options_class;

  options_class = GIMP_TOOL_OPTIONS_CLASS (klass);
}

static void
gimp_blend_options_init (GimpBlendOptions *options)
{
  options->offset  	 = options->offset_d  	    = 0.0;
  options->gradient_type = options->gradient_type_d = GIMP_LINEAR;
  options->repeat        = options->repeat_d        = GIMP_REPEAT_NONE;
  options->supersample   = options->supersample_d   = FALSE;
  options->max_depth     = options->max_depth_d     = 3;
  options->threshold     = options->threshold_d     = 0.2;
}

void
gimp_blend_options_gui (GimpToolOptions *tool_options)
{
  GimpBlendOptions *options;
  GtkWidget        *vbox;
  GtkWidget        *table;
  GtkWidget        *frame;

  options = GIMP_BLEND_OPTIONS (tool_options);

  gimp_paint_options_gui (tool_options);

  tool_options->reset_func = gimp_blend_options_reset;

  vbox = tool_options->main_vbox;

  gimp_dnd_viewable_dest_add (vbox,
                              GIMP_TYPE_GRADIENT,
                              blend_options_drop_gradient,
			      options);
  gimp_dnd_viewable_dest_add (vbox,
                              GIMP_TYPE_TOOL_INFO,
                              blend_options_drop_tool,
			      options);

  /*  the offset scale  */
  table = gtk_table_new (4, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 1);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  options->offset_w = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
					    _("Offset:"), -1, 50,
					    options->offset,
					    0.0, 100.0, 1.0, 10.0, 1,
					    TRUE, 0.0, 0.0,
					    NULL, NULL);

  g_signal_connect (options->offset_w, "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &options->offset);

  /*  the gradient preview  */
  {
    GimpGradient *gradient;
    GtkWidget    *button;
    GtkWidget    *preview;

    gradient = gimp_context_get_gradient (GIMP_CONTEXT (options));

    button = gtk_button_new ();
    preview = gimp_preview_new_full (GIMP_VIEWABLE (gradient),
                                     96, 16, 0,
                                     FALSE, FALSE, TRUE);
    gtk_container_add (GTK_CONTAINER (button), preview);
    gtk_widget_show (preview);

    gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                               _("Gradient:"), 1.0, 0.5,
                               button, 2, TRUE);

    g_signal_connect_object (options, "gradient_changed",
                             G_CALLBACK (gimp_preview_set_viewable),
                             preview,
                             G_CONNECT_SWAPPED);
    g_signal_connect (button, "clicked",
                      G_CALLBACK (blend_options_gradient_clicked),
                      NULL);
  }

  /*  the gradient type menu  */
  options->gradient_type_w =
    gimp_enum_option_menu_new (GIMP_TYPE_GRADIENT_TYPE,
                               G_CALLBACK (gradient_type_callback),
                               options);
  gimp_option_menu_set_history (GTK_OPTION_MENU (options->gradient_type_w),
                                GINT_TO_POINTER (options->gradient_type));
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
			     _("Shape:"), 1.0, 0.5,
			     options->gradient_type_w, 2, TRUE);

  /*  the repeat option  */
  options->repeat_w = 
    gimp_enum_option_menu_new (GIMP_TYPE_REPEAT_MODE,
                               G_CALLBACK (gimp_menu_item_update),
                               &options->repeat);
  gimp_option_menu_set_history (GTK_OPTION_MENU (options->repeat_w),
                                GINT_TO_POINTER (options->repeat));
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 3,
			     _("Repeat:"), 1.0, 0.5,
			     options->repeat_w, 2, TRUE);

  /*  show the table  */
  gtk_widget_show (table);

  /*  frame for supersampling options  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  /* vbox for the supersampling stuff */
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  /*  supersampling toggle  */
  options->supersample_w =
    gtk_check_button_new_with_label (_("Adaptive Supersampling"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->supersample_w),
				options->supersample);
  gtk_frame_set_label_widget (GTK_FRAME (frame), options->supersample_w);
  gtk_widget_show (options->supersample_w);

  g_signal_connect (options->supersample_w, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &options->supersample);

  /*  table for supersampling options  */
  table = gtk_table_new (2, 3, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 1);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  /*  automatically set the sensitive state of the table  */
  gtk_widget_set_sensitive (table, options->supersample);
  g_object_set_data (G_OBJECT (options->supersample_w), "set_sensitive",
                     table);

  /*  max depth scale  */
  options->max_depth_w = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
					       _("Max Depth:"), -1, 4,
					       options->max_depth,
					       1.0, 10.0, 1.0, 1.0, 0,
					       TRUE, 0.0, 0.0,
					       NULL, NULL);

  g_signal_connect (options->max_depth_w, "value_changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &options->max_depth);

  /*  threshold scale  */
  options->threshold_w = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
					       _("Threshold:"), -1, 4,
					       options->threshold,
					       0.0, 4.0, 0.01, 0.1, 2,
					       TRUE, 0.0, 0.0,
					       NULL, NULL);

  g_signal_connect (options->threshold_w, "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &options->threshold);

  /*  show the table  */
  gtk_widget_show (table);
}

static void
gimp_blend_options_reset (GimpToolOptions *tool_options)
{
  GimpBlendOptions *options;

  options = GIMP_BLEND_OPTIONS (tool_options);

  gimp_paint_options_reset (tool_options);

  options->gradient_type = options->gradient_type_d;
  options->repeat        = options->repeat_d;

  gtk_option_menu_set_history (GTK_OPTION_MENU (options->gradient_type_w),
			       options->gradient_type_d);
  gtk_option_menu_set_history (GTK_OPTION_MENU (options->repeat_w),
			       options->repeat_d);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->offset_w),
			    options->offset_d);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->supersample_w),
				options->supersample_d);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->max_depth_w),
			    options->max_depth_d);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->threshold_w),
			    options->threshold_d);
}

static void
blend_options_gradient_clicked (GtkWidget *widget, 
                                gpointer   data)
{
  GtkWidget *toplevel;

  toplevel = gtk_widget_get_toplevel (widget);

  if (GIMP_IS_DOCK (toplevel))
    gimp_dialog_factory_dialog_raise (GIMP_DOCK (toplevel)->dialog_factory,
                                      "gimp-gradient-list", -1); 
}

static void
gradient_type_callback (GtkWidget        *widget,
			GimpBlendOptions *options)
{
  gimp_menu_item_update (widget, &options->gradient_type);

  gtk_widget_set_sensitive (options->repeat_w, 
			    (options->gradient_type < 6));
}

static void
blend_options_drop_gradient (GtkWidget    *widget,
			     GimpViewable *viewable,
			     gpointer      data)
{
  GimpToolOptions *tool_options;

  tool_options = GIMP_TOOL_OPTIONS (data);

  gimp_context_set_gradient (GIMP_CONTEXT (tool_options),
                             GIMP_GRADIENT (viewable));
}

static void
blend_options_drop_tool (GtkWidget    *widget,
			 GimpViewable *viewable,
			 gpointer      data)
{
  GimpToolOptions *tool_options;
  GimpContext     *context;

  tool_options = (GimpToolOptions *) data;

  context = gimp_get_user_context (tool_options->tool_info->gimp);

  gimp_context_set_tool (context, GIMP_TOOL_INFO (viewable));
}
