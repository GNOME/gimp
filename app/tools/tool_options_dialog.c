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

#include "apptypes.h"

#include "tool_options.h"
#include "tool_options_dialog.h"
#include "tools.h"

#include "dialog_handler.h"
#include "gimpcontext.h"
#include "gimpdnd.h"
#include "gimpui.h"
#include "session.h"

#include "libgimp/gimpintl.h"


/*  local function prototypes  */

static ToolType  tool_options_drag_tool      (GtkWidget *widget,
					      gpointer   data);
static void      tool_options_drop_tool      (GtkWidget *widget,
					      ToolType   tool_type,
					      gpointer   data);
static void      tool_options_reset_callback (GtkWidget *widget,
					      gpointer   data);
static void      tool_options_close_callback (GtkWidget *widget,
					      gpointer   data);


/*  private variables  */

static GtkWidget * options_shell        = NULL;
static GtkWidget * options_vbox         = NULL;
static GtkWidget * options_label        = NULL;
static GtkWidget * options_pixmap       = NULL;
static GtkWidget * options_eventbox     = NULL;
static GtkWidget * options_reset_button = NULL;

/*  dnd stuff  */
static GtkTargetEntry tool_target_table[] =
{
  GIMP_TARGET_TOOL
};
static guint n_tool_targets = (sizeof (tool_target_table) /
                               sizeof (tool_target_table[0]));



/*  public functions  */

void
tool_options_dialog_new (void)
{
  GtkWidget *frame;
  GtkWidget *hbox;
  GtkWidget *vbox;

  /*  The shell and main vbox  */
  options_shell =
    gimp_dialog_new (_("Tool Options"), "tool_options",
		     tools_help_func,
		     "dialogs/tool_options.html",
		     GTK_WIN_POS_NONE,
		     FALSE, TRUE, TRUE,

		     _("Reset"), tool_options_reset_callback,
		     NULL, NULL, &options_reset_button, FALSE, FALSE,
		     _("Close"), tool_options_close_callback,
		     NULL, NULL, NULL, TRUE, TRUE,

		     NULL);

  /*  Register dialog  */
  dialog_register (options_shell);
  session_set_window_geometry (options_shell, &tool_options_session_info,
			       FALSE );

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

  options_pixmap = gtk_pixmap_new (tool_get_pixmap (RECT_SELECT),
				   tool_get_mask (RECT_SELECT));
  gtk_box_pack_start (GTK_BOX (hbox), options_pixmap, FALSE, FALSE, 0);
  gtk_widget_show (options_pixmap);

  options_label = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (hbox), options_label, FALSE, FALSE, 1);
  gtk_widget_show (options_label);

  options_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (options_vbox), 2);
  gtk_box_pack_start (GTK_BOX (vbox), options_vbox, FALSE, FALSE, 0);

  gtk_widget_show (options_vbox);

  /*  hide the separator between the dialog's vbox and the action area  */
  gtk_widget_hide (GTK_WIDGET (g_list_nth_data (gtk_container_children (GTK_CONTAINER (GTK_BIN (options_shell)->child)), 1)));

  /*  dnd stuff  */
  gtk_drag_dest_set (options_shell,
		     GTK_DEST_DEFAULT_HIGHLIGHT |
		     GTK_DEST_DEFAULT_MOTION |
		     GTK_DEST_DEFAULT_DROP,
		     tool_target_table, n_tool_targets,
		     GDK_ACTION_COPY); 
  gimp_dnd_tool_dest_set (options_shell, tool_options_drop_tool, NULL);

  gtk_drag_source_set (options_eventbox,
		       GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
		       tool_target_table, n_tool_targets,
		       GDK_ACTION_COPY); 
  gimp_dnd_tool_source_set (options_eventbox, tool_options_drag_tool, NULL);
}

void
tool_options_dialog_show (void)
{
  if (!GTK_WIDGET_VISIBLE (options_shell)) 
    {
      gtk_widget_show (options_shell);
    } 
  else 
    {
      gdk_window_raise (options_shell->window);
    }
}

void
tool_options_dialog_free (void)
{
  session_get_window_info (options_shell, &tool_options_session_info);
  gtk_widget_destroy (options_shell);
}

void
tool_options_register (ToolType     tool_type,
		       ToolOptions *tool_options)
{
  g_return_if_fail (tool_options != NULL);

  if (! GTK_WIDGET_VISIBLE (tool_options->main_vbox))
    {
      gtk_box_pack_start (GTK_BOX (options_vbox), tool_options->main_vbox,
			  TRUE, TRUE, 0);
    }
}

void
tool_options_show (ToolType tool_type)
{
  if (tool_info[tool_type].tool_options->main_vbox)
    gtk_widget_show (tool_info[tool_type].tool_options->main_vbox);

  if (tool_info[tool_type].tool_options->title)
    gtk_label_set_text (GTK_LABEL (options_label),
			tool_info[tool_type].tool_options->title);

  gtk_pixmap_set (GTK_PIXMAP (options_pixmap),
		  tool_get_pixmap (tool_type), tool_get_mask (tool_type));

  gtk_widget_queue_draw (options_pixmap);

  gimp_help_set_help_data (options_label->parent->parent,
			   gettext (tool_info[(gint) tool_type].tool_desc),
			   tool_info[(gint) tool_type].private_tip);

  if (tool_info[tool_type].tool_options->reset_func)
    gtk_widget_set_sensitive (options_reset_button, TRUE);
  else
    gtk_widget_set_sensitive (options_reset_button, FALSE);
}

void
tool_options_hide (ToolType tool_type)
{
  if (tool_info[tool_type].tool_options)
    gtk_widget_hide (tool_info[tool_type].tool_options->main_vbox);
}


/*  private functions  */

static void
tool_options_drop_tool (GtkWidget *widget,
			ToolType   tool,
			gpointer   data)
{
  gimp_context_set_tool (gimp_context_get_user (), tool);
}

ToolType
tool_options_drag_tool (GtkWidget *widget,
			gpointer   data)
{
  return gimp_context_get_tool (gimp_context_get_user ());
}

static void
tool_options_close_callback (GtkWidget *widget,
			     gpointer   data)
{
  GtkWidget *shell;

  shell = (GtkWidget *) data;
  gimp_dialog_hide (shell);
}

static void
tool_options_reset_callback (GtkWidget *widget,
			     gpointer   data)
{
  GtkWidget *shell;

  shell = (GtkWidget *) data;

  if (!active_tool)
    return;

  if (tool_info[(gint) active_tool->type].tool_options->reset_func)
    (* tool_info[(gint) active_tool->type].tool_options->reset_func) ();
}
