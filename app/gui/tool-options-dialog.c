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

#include "tools/tools-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimplist.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpdnd.h"
#include "widgets/gimppreview.h"
#include "widgets/gimpwidgets-utils.h"

#include "tools/gimptool.h"
#include "tools/tool_options.h"
#include "tools/tool_manager.h"

#include "tool-options-dialog.h"

#include "app_procs.h"

#include "libgimp/gimpintl.h"


/*  local function prototypes  */

static void           tool_options_dialog_tool_changed   (GimpContext  *context,
							  GimpToolInfo *tool_info,
							  gpointer      data);
static GimpViewable * tool_options_dialog_drag_tool      (GtkWidget    *widget,
							  gpointer      data);
static void           tool_options_dialog_drop_tool      (GtkWidget    *widget,
							  GimpViewable *viewable,
							  gpointer      data);
static void           tool_options_dialog_reset_callback (GtkWidget    *widget,
							  gpointer      data);
static void           tool_options_dialog_close_callback (GtkWidget    *widget,
							  gpointer      data);


/*  private variables  */

static GtkWidget   *options_shell        = NULL;
static GtkWidget   *options_vbox         = NULL;
static GtkWidget   *options_label        = NULL;
static GtkWidget   *options_preview      = NULL;
static GtkWidget   *options_eventbox     = NULL;
static GtkWidget   *options_reset_button = NULL;

static GimpToolOptions *visible_tool_options = NULL;


/*  public functions  */

GtkWidget *
tool_options_dialog_create (void)
{
  GimpToolInfo *tool_info;
  GtkWidget    *frame;
  GtkWidget    *hbox;
  GtkWidget    *vbox;

  if (options_shell)
    return options_shell;

  tool_info = gimp_context_get_tool (gimp_get_user_context (the_gimp));

  if (! tool_info)
    {
      g_warning ("%s(): no tool info registered for active tool",
                 G_GNUC_FUNCTION);
    }

  /*  The shell and main vbox  */
  options_shell =
    gimp_dialog_new (_("Tool Options"), "tool_options",
		     tool_manager_help_func,
		     "dialogs/tool_options.html",
		     GTK_WIN_POS_NONE,
		     FALSE, TRUE, TRUE,

		     _("Reset"), tool_options_dialog_reset_callback,
		     NULL, NULL, &options_reset_button, FALSE, FALSE,

		     GTK_STOCK_CLOSE, tool_options_dialog_close_callback,
		     NULL, NULL, NULL, TRUE, TRUE,

		     NULL);

  /*  hide the separator between the dialog's vbox and the action area  */
  gtk_widget_hide (GTK_WIDGET (g_list_nth_data (gtk_container_children (GTK_CONTAINER (GTK_BIN (options_shell)->child)), 0)));

  /*  The outer frame  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (options_shell)->vbox), frame);
  gtk_widget_show (frame);

  /*  The vbox containing the title frame and the options vbox  */
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  /*  The title frame  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  options_eventbox = gtk_event_box_new ();
  gtk_container_add (GTK_CONTAINER (frame), options_eventbox);
  gtk_widget_show (options_eventbox);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (options_eventbox), hbox);
  gtk_widget_show (hbox);

  options_preview = gimp_preview_new (GIMP_VIEWABLE (tool_info), 22, 0, FALSE);
  gtk_box_pack_start (GTK_BOX (hbox), options_preview, FALSE, FALSE, 0);
  gtk_widget_show (options_preview);

  options_label = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (hbox), options_label, FALSE, FALSE, 1);
  gtk_widget_show (options_label);

  options_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (options_vbox), 2);
  gtk_box_pack_start (GTK_BOX (vbox), options_vbox, FALSE, FALSE, 0);

  gtk_widget_show (options_vbox);

  /*  dnd stuff  */
  gimp_gtk_drag_dest_set_by_type (options_shell,
				  GTK_DEST_DEFAULT_HIGHLIGHT |
				  GTK_DEST_DEFAULT_MOTION    |
				  GTK_DEST_DEFAULT_DROP,
				  GIMP_TYPE_TOOL_INFO,
				  GDK_ACTION_COPY);
  gimp_dnd_viewable_dest_set (options_shell,
			      GIMP_TYPE_TOOL_INFO,
			      tool_options_dialog_drop_tool, NULL);

  gimp_gtk_drag_source_set_by_type (options_eventbox,
				    GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
				    GIMP_TYPE_TOOL_INFO,
				    GDK_ACTION_COPY); 
  gimp_dnd_viewable_source_set (options_eventbox,
				GIMP_TYPE_TOOL_INFO,
				tool_options_dialog_drag_tool, NULL);

  gtk_signal_connect_while_alive
    (GTK_OBJECT (gimp_get_user_context (the_gimp)), "tool_changed",
     GTK_SIGNAL_FUNC (tool_options_dialog_tool_changed),
     NULL,
     GTK_OBJECT (options_shell));

  tool_info = gimp_context_get_tool (gimp_get_user_context (the_gimp));

  tool_options_dialog_tool_changed (gimp_get_user_context (the_gimp),
				    tool_info,
				    NULL);

  return options_shell;
}

void
tool_options_dialog_free (void)
{
  if (options_shell)
    {
      gtk_widget_destroy (options_shell);
      options_shell = NULL;
    }
}


/*  private functions  */

static void
tool_options_dialog_tool_changed (GimpContext  *context,
				  GimpToolInfo *tool_info,
				  gpointer      data)
{
  if (visible_tool_options &&
      (! tool_info || tool_info->tool_options != visible_tool_options))
    {
      gtk_widget_hide (visible_tool_options->main_vbox);

      visible_tool_options = NULL;
    }

  if (tool_info)
    {
      if (tool_info->tool_options)
	{
	  if (! tool_info->tool_options->main_vbox->parent)
	    gtk_box_pack_start (GTK_BOX (options_vbox),
				tool_info->tool_options->main_vbox,
				FALSE, FALSE, 0);

	  gtk_widget_show (tool_info->tool_options->main_vbox);

	  visible_tool_options = tool_info->tool_options;

	  gtk_label_set_text (GTK_LABEL (options_label),
			      tool_info->blurb);

	  if (tool_info->tool_options->reset_func)
	    gtk_widget_set_sensitive (options_reset_button, TRUE);
	  else
	    gtk_widget_set_sensitive (options_reset_button, FALSE);
	}
      else
	{
	  gtk_widget_set_sensitive (options_reset_button, FALSE);
	}

      gimp_preview_set_viewable (GIMP_PREVIEW (options_preview),
				 GIMP_VIEWABLE (tool_info));

      gimp_help_set_help_data (options_label->parent->parent,
			       tool_info->help,
			       tool_info->help_data);

    }
}

static void
tool_options_dialog_drop_tool (GtkWidget    *widget,
			       GimpViewable *viewable,
			       gpointer      data)
{
  gimp_context_set_tool (gimp_get_user_context (the_gimp),
			 GIMP_TOOL_INFO (viewable));
}

GimpViewable *
tool_options_dialog_drag_tool (GtkWidget *widget,
			       gpointer   data)
{
  return (GimpViewable *) gimp_context_get_tool (gimp_get_user_context (the_gimp));
}

static void
tool_options_dialog_close_callback (GtkWidget *widget,
				    gpointer   data)
{
  GtkWidget *shell;

  shell = (GtkWidget *) data;

  gimp_dialog_hide (shell);
}

static void
tool_options_dialog_reset_callback (GtkWidget *widget,
				    gpointer   data)
{
  GimpToolInfo *tool_info;
  GimpTool     *active_tool;
  GtkWidget    *shell;

  shell = (GtkWidget *) data;

  active_tool = tool_manager_get_active (the_gimp);

  if (! active_tool)
    return;

  tool_info = tool_manager_get_info_by_tool (the_gimp, active_tool);

  if (! tool_info)
    {
      g_warning ("%s(): no tool info registered for %s",
                 G_GNUC_FUNCTION,
		 g_type_name (G_TYPE_FROM_INSTANCE (active_tool)));
    }

  if (tool_info->tool_options->reset_func)
    tool_info->tool_options->reset_func (tool_info->tool_options);
}
