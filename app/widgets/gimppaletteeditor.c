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

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "apptypes.h"

#include "widgets/gimpcontainerlistview.h"
#include "widgets/gimpdatafactoryview.h"
#include "widgets/gimppreview.h"

#include "color_area.h"
#include "color_notebook.h"
#include "context_manager.h"
#include "dialog_handler.h"
#include "gimage.h"
#include "gimpcontext.h"
#include "gimpdatafactory.h"
#include "gimpdatalist.h"
#include "gimpdnd.h"
#include "gimppalette.h"
#include "gimprc.h"
#include "palette.h"
#include "palette_import.h"
#include "palette_select.h"
#include "paletteP.h"
#include "session.h"

#include "libgimp/gimpenv.h"

#include "libgimp/gimpintl.h"

#include "pixmaps/zoom_in.xpm"
#include "pixmaps/zoom_out.xpm"


#define ENTRY_WIDTH  12
#define ENTRY_HEIGHT 10
#define SPACING       1
#define COLUMNS      16
#define ROWS         11

#define PREVIEW_WIDTH  ((ENTRY_WIDTH  * COLUMNS) + (SPACING * (COLUMNS + 1)))
#define PREVIEW_HEIGHT ((ENTRY_HEIGHT * ROWS)    + (SPACING * (ROWS    + 1)))

#define PALETTE_EVENT_MASK (GDK_EXPOSURE_MASK     | \
                            GDK_BUTTON_PRESS_MASK | \
                            GDK_ENTER_NOTIFY_MASK)


typedef struct _PaletteDialog PaletteDialog;

struct _PaletteDialog
{
  GtkWidget        *shell;

  GtkWidget        *color_area;
  GtkWidget        *scrolled_window;
  GtkWidget        *color_name;

  GtkWidget        *view;

  GimpContext      *context;

  GtkWidget        *popup_menu;
  GtkWidget        *delete_menu_item;
  GtkWidget        *edit_menu_item;

  ColorNotebook    *color_notebook;
  gboolean          color_notebook_active;

  GimpPaletteEntry *color;
  GimpPaletteEntry *dnd_color;

  guint             entry_sig_id;
  gfloat            zoom_factor;  /* range from 0.1 to 4.0 */
  gint              col_width;
  gint              last_width;
  gint              columns;
  gboolean          freeze_update;
  gboolean          columns_valid;

  GQuark            invalidate_preview_handler_id;
};


/*  local function prototypes  */
static PaletteDialog * palette_dialog_new          (gboolean        editor);

static void   palette_dialog_create_popup_menu     (PaletteDialog  *palette_dialog);
static void   palette_dialog_new_entry_callback    (GtkWidget      *widget,
						    gpointer        data);
static void   palette_dialog_edit_entry_callback   (GtkWidget      *widget,
						    gpointer        data);
static void   palette_dialog_delete_entry_callback (GtkWidget      *widget,
						    gpointer        data);
static void   palette_dialog_color_notebook_callback (ColorNotebook      *color_notebook,
						      const GimpRGB      *color,
						      ColorNotebookState  state,
						      gpointer            data);

static gint   palette_dialog_eventbox_button_press (GtkWidget      *widget,
						    GdkEventButton *bevent,
						    PaletteDialog  *palette_dialog);
static gint   palette_dialog_color_area_events     (GtkWidget      *widget,
						    GdkEvent       *event,
						    PaletteDialog  *palette_dialog);
static void   palette_dialog_draw_entries       (PaletteDialog  *palette,
						 gint            row_start,
						 gint            column_highlight);
static void   palette_dialog_redraw             (PaletteDialog  *palette);
static void   palette_dialog_scroll_top_left    (PaletteDialog  *palette);

static void   palette_dialog_color_name_changed (GtkWidget      *widget,
						 gpointer        data);
static void   palette_dialog_zoomin_callback    (GtkWidget      *widget,
						 gpointer        data);
static void   palette_dialog_zoomout_callback   (GtkWidget      *widget,
						 gpointer        data);
static void   palette_dialog_redraw_zoom        (PaletteDialog  *palette_dialog);
static void   palette_dialog_import_callback    (GtkWidget      *widget,
						 gpointer        data);
static void   palette_dialog_merge_callback     (GtkWidget      *widget,
						 gpointer        data);
static void   palette_dialog_do_merge_callback  (GtkWidget      *widget,
						 gchar          *palette_name,
						 gpointer        data);
static void   palette_dialog_close_callback     (GtkWidget      *widget,
						 gpointer        data);
static void   palette_dialog_palette_changed    (GimpContext    *context,
						 GimpPalette    *palette,
						 gpointer        data);
static void   palette_dialog_drag_color         (GtkWidget      *widget,
						 GimpRGB        *color,
						 gpointer        data);
static void   palette_dialog_drop_color         (GtkWidget      *widget,
						 const GimpRGB  *color,
						 gpointer        data);
static void   palette_dialog_invalidate_preview (GimpPalette    *palette,
						 PaletteDialog  *palette_dialog);
static void   palette_dialog_edit_palette       (GimpData       *data);


PaletteDialog *top_level_edit_palette  = NULL;
PaletteDialog *top_level_palette       = NULL;


/*  dnd stuff  */
static GtkTargetEntry color_palette_target_table[] =
{
  GIMP_TARGET_COLOR
};
static guint n_color_palette_targets = (sizeof (color_palette_target_table) /
					sizeof (color_palette_target_table[0]));


/*  public functions  ********************************************************/

void 
palette_dialog_create (void)
{
  if (top_level_palette == NULL)
    {
      top_level_palette = palette_dialog_new (FALSE);
      session_set_window_geometry (top_level_palette->shell,
				   &palette_session_info, TRUE);
      dialog_register (top_level_palette->shell);

      gtk_widget_show (top_level_palette->shell);
    }
  else
    {
      if (! GTK_WIDGET_VISIBLE (top_level_palette->shell))
	{
	  gtk_widget_show (top_level_palette->shell);
	}
      else
	{
	  gdk_window_raise (top_level_palette->shell->window);
	}
    }
}

void
palette_dialog_free (void)
{
  if (top_level_edit_palette)
    {
      palette_import_dialog_destroy ();

      if (top_level_edit_palette->color_notebook) 
	color_notebook_free (top_level_edit_palette->color_notebook);

      gtk_signal_disconnect_by_func (GTK_OBJECT (top_level_edit_palette->context),
				     palette_dialog_palette_changed,
				     top_level_edit_palette);

      gimp_container_remove_handler (global_palette_factory->container,
				     top_level_edit_palette->invalidate_preview_handler_id);

      g_free (top_level_edit_palette); 
      top_level_edit_palette = NULL; 
    }

  if (top_level_palette)
    {
      session_get_window_info (top_level_palette->shell,
			       &palette_session_info);  

      if (top_level_palette->color_notebook) 
	color_notebook_free (top_level_palette->color_notebook); 

      gtk_signal_disconnect_by_func (GTK_OBJECT (top_level_palette->context),
				     palette_dialog_palette_changed,
				     top_level_palette);

      gimp_container_remove_handler (global_palette_factory->container,
				     top_level_palette->invalidate_preview_handler_id);

      g_free (top_level_palette);
      top_level_palette = NULL;
    }
}


/*  called from color_picker.h  *********************************************/

void
palette_set_active_color (gint r,
			  gint g,
			  gint b,
			  gint state)
{
  GimpPalette *palette;
  GimpRGB      color;

  gimp_rgba_set_uchar (&color,
		       (guchar) r,
		       (guchar) g,
		       (guchar) b,
		       255);

  if (top_level_edit_palette)
    {
      palette = gimp_context_get_palette (top_level_edit_palette->context);

      if (palette)
	{
	  switch (state)
	    {
	    case COLOR_NEW:
	      top_level_edit_palette->color = gimp_palette_add_entry (palette,
								      NULL,
								      &color);
	      break;

	    case COLOR_UPDATE_NEW:
	      top_level_edit_palette->color->color = color;

	      gimp_data_dirty (GIMP_DATA (palette));
	      break;

	    default:
	      break;
	    }
	}
    }

  if (active_color == FOREGROUND)
    gimp_context_set_foreground (gimp_context_get_user (), &color);
  else if (active_color == BACKGROUND)
    gimp_context_set_background (gimp_context_get_user (), &color);
}

/*  called from palette_select.c  ********************************************/

void 
palette_create_edit (GimpPalette *palette)
{
  if (top_level_edit_palette == NULL)
    {
      top_level_edit_palette = palette_dialog_new (TRUE);
      dialog_register (top_level_edit_palette->shell);

      gtk_widget_show (top_level_edit_palette->shell);
      palette_dialog_draw_entries (top_level_edit_palette, -1, -1);
    }
  else
    {
      if (! GTK_WIDGET_VISIBLE (top_level_edit_palette->shell))
	gtk_widget_show (top_level_edit_palette->shell);
      else
	gdk_window_raise (top_level_edit_palette->shell->window);
    }

  gimp_context_set_palette (top_level_edit_palette->context, palette);
}


/*  private functions  */

/*  the palette & palette edit dialog constructor  ***************************/

PaletteDialog *
palette_dialog_new (gboolean editor)
{
  PaletteDialog *palette_dialog;
  GtkWidget     *hbox;
  GtkWidget     *hbox2;
  GtkWidget     *vbox;
  GtkWidget     *scrolledwindow;
  GtkWidget     *palette_region;
  GtkWidget     *entry;
  GtkWidget     *eventbox;
  GtkWidget     *alignment;
  GtkWidget     *frame;
  GtkWidget     *button;

  palette_dialog = g_new0 (PaletteDialog, 1);

  if (! editor)
    palette_dialog->context = gimp_context_get_user ();
  else
    palette_dialog->context = gimp_context_new (NULL, NULL);

  palette_dialog->zoom_factor   = 1.0;
  palette_dialog->columns       = COLUMNS;
  palette_dialog->columns_valid = TRUE;
  palette_dialog->freeze_update = FALSE;

  if (editor)
    {
      palette_dialog->shell =
	gimp_dialog_new (_("Color Palette Edit"), "color_palette_edit",
			 gimp_standard_help_func,
			 "dialogs/palette_editor/palette_editor.html",
			 GTK_WIN_POS_NONE,
			 FALSE, TRUE, FALSE,

			 "_delete_event_", palette_dialog_close_callback,
			 palette_dialog, NULL, NULL, TRUE, TRUE,

			 NULL);
    }
  else
    {
      palette_dialog->shell =
	gimp_dialog_new (_("Color Palette"), "color_palette",
			 gimp_standard_help_func,
			 "dialogs/palette_selection.html",
			 GTK_WIN_POS_NONE,
			 FALSE, TRUE, FALSE,

			 "_delete_event_", palette_dialog_close_callback,
			 palette_dialog, NULL, NULL, TRUE, TRUE,

			 NULL);
     }

  gtk_widget_hide (GTK_WIDGET (g_list_nth_data (gtk_container_children (GTK_CONTAINER (GTK_DIALOG (palette_dialog->shell)->vbox)), 0)));

  gtk_widget_hide (GTK_DIALOG (palette_dialog->shell)->action_area);

  /*  The main container widget  */
  if (! editor)
    {
      hbox = gtk_notebook_new ();
      gtk_container_set_border_width (GTK_CONTAINER (hbox), 1);
    }
  else
    {
      hbox = gtk_hbox_new (FALSE, 4);
      gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
    }

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (palette_dialog->shell)->vbox),
		     hbox);
  gtk_widget_show (hbox);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_widget_show (vbox);

  palette_dialog->scrolled_window =
    scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_ALWAYS);
  gtk_box_pack_start (GTK_BOX (vbox), scrolledwindow, TRUE, TRUE, 0);
  gtk_widget_show (scrolledwindow);

  eventbox = gtk_event_box_new ();
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolledwindow),
					 eventbox);
  gtk_signal_connect (GTK_OBJECT (eventbox), "button_press_event",
		      GTK_SIGNAL_FUNC (palette_dialog_eventbox_button_press),
		      palette_dialog);
  gtk_widget_show (eventbox);

  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0); 
  gtk_container_add (GTK_CONTAINER (eventbox), alignment);
  gtk_widget_show (alignment);

  palette_dialog->color_area = palette_region =
    gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_set_dither (GTK_PREVIEW (palette_dialog->color_area),
			  GDK_RGB_DITHER_MAX);
  gtk_preview_size (GTK_PREVIEW (palette_region), PREVIEW_WIDTH, PREVIEW_HEIGHT);
  
  gtk_widget_set_events (palette_region, PALETTE_EVENT_MASK);
  gtk_signal_connect (GTK_OBJECT (palette_dialog->color_area), "event",
		      GTK_SIGNAL_FUNC (palette_dialog_color_area_events),
		      palette_dialog);

  gtk_container_add (GTK_CONTAINER (alignment), palette_region);
  gtk_widget_show (palette_region);

  /*  dnd stuff  */
  gtk_drag_source_set (palette_region,
                       GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
                       color_palette_target_table, n_color_palette_targets,
                       GDK_ACTION_COPY | GDK_ACTION_MOVE);
  gimp_dnd_color_source_set (palette_region, palette_dialog_drag_color,
			     palette_dialog);

  gtk_drag_dest_set (alignment,
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_DROP,
                     color_palette_target_table, n_color_palette_targets,
                     GDK_ACTION_COPY);
  gimp_dnd_color_dest_set (alignment, palette_dialog_drop_color, palette_dialog);

  /*  The color name entry  */
  hbox2 = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox2, FALSE, FALSE, 0);
  gtk_widget_show (hbox2);

  entry = palette_dialog->color_name = gtk_entry_new ();
  gtk_widget_show (entry);
  gtk_box_pack_start (GTK_BOX (hbox2), entry, TRUE, TRUE, 0);
  gtk_entry_set_text (GTK_ENTRY (entry), _("Undefined"));
  gtk_widget_set_sensitive (entry, FALSE);
  palette_dialog->entry_sig_id =
    gtk_signal_connect (GTK_OBJECT (entry), "changed",
			GTK_SIGNAL_FUNC (palette_dialog_color_name_changed),
			palette_dialog);

  /*  + and - buttons  */
  button = gimp_pixmap_button_new (zoom_in_xpm, NULL);
  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (palette_dialog_zoomin_callback),
		      palette_dialog);
  gtk_widget_show (button);

  button = gimp_pixmap_button_new (zoom_out_xpm, NULL);
  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (palette_dialog_zoomout_callback),
		      palette_dialog);
  gtk_widget_show (button);
  
  /*  The Palette List  */
  palette_dialog->view =
    gimp_data_factory_view_new (GIMP_VIEW_TYPE_LIST,
				global_palette_factory,
				editor ? NULL : palette_dialog_edit_palette,
				palette_dialog->context,
				SM_PREVIEW_HEIGHT,
				2, 4);

  if (! editor)
    {
      gtk_notebook_append_page (GTK_NOTEBOOK (hbox), vbox,
				gtk_label_new (_("Palette")));
      gtk_notebook_append_page (GTK_NOTEBOOK (hbox), palette_dialog->view,
				gtk_label_new (_("Select")));
    }
  else
    {
      gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
      gtk_box_pack_start (GTK_BOX (hbox), palette_dialog->view, TRUE, TRUE, 0);
    }

  if (editor)
    gtk_list_set_selection_mode (GTK_LIST (GIMP_CONTAINER_LIST_VIEW (GIMP_DATA_FACTORY_VIEW (palette_dialog->view)->view)->gtk_list),
				 GTK_SELECTION_EXTENDED);

  gtk_signal_connect (GTK_OBJECT (palette_dialog->context), "palette_changed",
		      GTK_SIGNAL_FUNC (palette_dialog_palette_changed),
		      palette_dialog);

  palette_dialog->invalidate_preview_handler_id =
    gimp_container_add_handler (global_palette_factory->container,
				"invalidate_preview",
				GTK_SIGNAL_FUNC (palette_dialog_invalidate_preview),
				palette_dialog);

  gtk_widget_show (palette_dialog->view);
  
  if (editor)
    {
      frame = gtk_frame_new (_("Palette Ops"));
      gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame); 

      vbox = gtk_vbox_new (FALSE, 2);
      gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
      gtk_container_add (GTK_CONTAINER (frame), vbox);
      gtk_widget_show (vbox);

      button = gtk_button_new_with_label (_("Import"));
      GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);
      gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  GTK_SIGNAL_FUNC (palette_dialog_import_callback),
			  palette_dialog);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gimp_help_set_help_data (button, NULL,
			       "dialogs/palette_editor/import_palette.html");
      gtk_widget_show (button);

      button = gtk_button_new_with_label (_("Merge"));
      GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);
      gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  GTK_SIGNAL_FUNC (palette_dialog_merge_callback),
			  palette_dialog);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gimp_help_set_help_data (button, NULL,
			       "dialogs/palette_editor/merge_palette.html");
      gtk_widget_show (button);
    }

  gtk_widget_realize (palette_dialog->shell);

  palette_dialog_create_popup_menu (palette_dialog);

  return palette_dialog;
}

/*  the palette dialog popup menu & callbacks  *******************************/

static void
palette_dialog_create_popup_menu (PaletteDialog *palette_dialog)
{
  GtkWidget *menu;
  GtkWidget *menu_item;

  palette_dialog->popup_menu = menu = gtk_menu_new ();

  menu_item = gtk_menu_item_new_with_label (_("New"));
  gtk_menu_append (GTK_MENU (menu), menu_item);
  gtk_signal_connect (GTK_OBJECT (menu_item), "activate", 
		      GTK_SIGNAL_FUNC (palette_dialog_new_entry_callback),
		      palette_dialog);
  gtk_widget_show (menu_item);

  menu_item = gtk_menu_item_new_with_label (_("Edit"));
  gtk_menu_append (GTK_MENU (menu), menu_item);

  gtk_signal_connect (GTK_OBJECT (menu_item), "activate", 
		      GTK_SIGNAL_FUNC (palette_dialog_edit_entry_callback),
		      palette_dialog);
  gtk_widget_show (menu_item);

  palette_dialog->edit_menu_item = menu_item;

  menu_item = gtk_menu_item_new_with_label (_("Delete"));
  gtk_signal_connect (GTK_OBJECT (menu_item), "activate", 
		      GTK_SIGNAL_FUNC (palette_dialog_delete_entry_callback),
		      palette_dialog);
  gtk_menu_append (GTK_MENU (menu), menu_item);
  gtk_widget_show (menu_item);

  palette_dialog->delete_menu_item = menu_item;
}

static void
palette_dialog_new_entry_callback (GtkWidget *widget,
				   gpointer   data)
{
  PaletteDialog *palette_dialog;
  GimpRGB        color;

  palette_dialog = (PaletteDialog *) data;

  if (! (palette_dialog && gimp_context_get_palette (palette_dialog->context)))
    return;

  if (active_color == FOREGROUND)
    gimp_context_get_foreground (gimp_context_get_user (), &color);
  else if (active_color == BACKGROUND)
    gimp_context_get_background (gimp_context_get_user (), &color);

  palette_dialog->color =
    gimp_palette_add_entry (gimp_context_get_palette (palette_dialog->context),
			    NULL,
			    &color);
}

static void
palette_dialog_edit_entry_callback (GtkWidget *widget,
				    gpointer   data)
{
  PaletteDialog *palette_dialog;

  palette_dialog = (PaletteDialog *) data;

  if (! (palette_dialog && gimp_context_get_palette (palette_dialog->context)
	 && palette_dialog->color))
    return;

  if (! palette_dialog->color_notebook)
    {
      palette_dialog->color_notebook =
	color_notebook_new (_("Edit Palette Color"),
			    (const GimpRGB *) &palette_dialog->color->color,
			    palette_dialog_color_notebook_callback,
			    palette_dialog,
			    FALSE,
			    FALSE);
      palette_dialog->color_notebook_active = TRUE;
    }
  else
    {
      if (! palette_dialog->color_notebook_active)
	{
	  color_notebook_show (palette_dialog->color_notebook);
	  palette_dialog->color_notebook_active = TRUE;
	}

      color_notebook_set_color (palette_dialog->color_notebook,
				&palette_dialog->color->color);
    }
}

static void
palette_dialog_delete_entry_callback (GtkWidget *widget,
				      gpointer   data)
{
  PaletteDialog *palette_dialog;

  palette_dialog = (PaletteDialog *) data;

  if (! (palette_dialog && gimp_context_get_palette (palette_dialog->context) &&
	 palette_dialog->color))
    return;

  gimp_palette_delete_entry (gimp_context_get_palette (palette_dialog->context),
			     palette_dialog->color);
}

static void
palette_dialog_color_notebook_callback (ColorNotebook      *color_notebook,
					const GimpRGB      *color,
					ColorNotebookState  state,
					gpointer            data)
{
  PaletteDialog *palette_dialog;
  GimpPalette   *palette;

  palette_dialog = data;

  palette = gimp_context_get_palette (palette_dialog->context);

  switch (state)
    {
    case COLOR_NOTEBOOK_UPDATE:
      break;

    case COLOR_NOTEBOOK_OK:
      if (palette_dialog->color)
	{
	  palette_dialog->color->color = *color;

	  /*  Update either foreground or background colors  */
	  if (active_color == FOREGROUND)
	    gimp_context_set_foreground (gimp_context_get_user (), color);
	  else if (active_color == BACKGROUND)
	    gimp_context_set_background (gimp_context_get_user (), color);

	  gimp_data_dirty (GIMP_DATA (palette));
	}

      /* Fallthrough */
    case COLOR_NOTEBOOK_CANCEL:
      if (palette_dialog->color_notebook_active)
	{
	  color_notebook_hide (palette_dialog->color_notebook);
	  palette_dialog->color_notebook_active = FALSE;
	}
    }
}

/*  the color area event callbacks  ******************************************/

static gint
palette_dialog_eventbox_button_press (GtkWidget      *widget,
				      GdkEventButton *bevent,
				      PaletteDialog  *palette_dialog)
{
  if (gtk_get_event_widget ((GdkEvent *) bevent) == palette_dialog->color_area)
    return FALSE;

  if (bevent->button == 3)
    {
      if (GTK_WIDGET_SENSITIVE (palette_dialog->edit_menu_item))
	{
	  gtk_widget_set_sensitive (palette_dialog->edit_menu_item, FALSE);
	  gtk_widget_set_sensitive (palette_dialog->delete_menu_item, FALSE);
	}

      gtk_menu_popup (GTK_MENU (palette_dialog->popup_menu), NULL, NULL, 
		      NULL, NULL, 3,
		      bevent->time);
    }

  return TRUE;
}

static gint
palette_dialog_color_area_events (GtkWidget     *widget,
				  GdkEvent      *event,
				  PaletteDialog *palette_dialog)
{
  GdkEventButton *bevent;
  GList          *list;
  gint            entry_width;
  gint            entry_height;
  gint            row, col;
  gint            pos;

  switch (event->type)
    {
    case GDK_EXPOSE:
      palette_dialog_redraw (palette_dialog);
      break;

    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;
      entry_width  = palette_dialog->col_width + SPACING;
      entry_height = (ENTRY_HEIGHT * palette_dialog->zoom_factor) +  SPACING;
      col = (bevent->x - 1) / entry_width;
      row = (bevent->y - 1) / entry_height;
      pos = row * palette_dialog->columns + col;

      if (gimp_context_get_palette (palette_dialog->context))
	list = g_list_nth (gimp_context_get_palette (palette_dialog->context)->colors, pos);
      else
	list = NULL;

      if (list)
	palette_dialog->dnd_color = list->data;
      else
	palette_dialog->dnd_color = NULL;

      if ((bevent->button == 1 || bevent->button == 3) &&
	  gimp_context_get_palette (palette_dialog->context))
	{
	  if (list)
	    {
	      if (palette_dialog->color)
		{
		  palette_dialog->freeze_update = TRUE;
 		  palette_dialog_draw_entries (palette_dialog, -1, -1);
		  palette_dialog->freeze_update = FALSE;
		}
	      palette_dialog->color = (GimpPaletteEntry *) list->data;

	      if (active_color == FOREGROUND)
		{
		  if (bevent->state & GDK_CONTROL_MASK)
		    gimp_context_set_background (gimp_context_get_user (),
						 &palette_dialog->color->color);
		  else
		    gimp_context_set_foreground (gimp_context_get_user (),
						 &palette_dialog->color->color);
		}
	      else if (active_color == BACKGROUND)
		{
		  if (bevent->state & GDK_CONTROL_MASK)
		    gimp_context_set_foreground (gimp_context_get_user (),
						 &palette_dialog->color->color);
		  else
		    gimp_context_set_background (gimp_context_get_user (),
						 &palette_dialog->color->color);
		}

	      palette_dialog_draw_entries (palette_dialog, row, col);
	      /*  Update the active color name  */
	      gtk_entry_set_text (GTK_ENTRY (palette_dialog->color_name),
				  palette_dialog->color->name);
	      gtk_widget_set_sensitive (palette_dialog->color_name, TRUE);
	      /* palette_update_current_entry (palette_dialog); */

	      if (bevent->button == 3)
		{
		  if (! GTK_WIDGET_SENSITIVE (palette_dialog->edit_menu_item))
		    {
		      gtk_widget_set_sensitive (palette_dialog->edit_menu_item, TRUE);
		      gtk_widget_set_sensitive (palette_dialog->delete_menu_item, TRUE);
		    }

		  gtk_menu_popup (GTK_MENU (palette_dialog->popup_menu),
				  NULL, NULL, 
				  NULL, NULL, 3,
				  bevent->time);
		}
	    }
	  else
	    {
	      if (bevent->button == 3)
		{
		  if (GTK_WIDGET_SENSITIVE (palette_dialog->edit_menu_item))
		    {
		      gtk_widget_set_sensitive (palette_dialog->edit_menu_item, FALSE);
		      gtk_widget_set_sensitive (palette_dialog->delete_menu_item, FALSE);
		    }

		  gtk_menu_popup (GTK_MENU (palette_dialog->popup_menu),
				  NULL, NULL, 
				  NULL, NULL, 3,
				  bevent->time);
		}
	    }
	}
      break;

    default:
      break;
    }

  return FALSE;
}

/*  functions for drawing & updating the palette dialog color area  **********/

static gint
palette_dialog_draw_color_row (guchar         *colors,
			       gint            n_colors,
			       gint            y,
			       gint            column_highlight,
			       guchar         *buffer,
			       PaletteDialog  *palette_dialog)
{
  guchar    *p;
  guchar     bcolor;
  gint       width, height;
  gint       entry_width;
  gint       entry_height;
  gint       vsize;
  gint       vspacing;
  gint       i, j;
  GtkWidget *preview;

  if (! palette_dialog)
    return -1;

  preview = palette_dialog->color_area;

  bcolor = 0;

  width        = preview->requisition.width;
  height       = preview->requisition.height;
  entry_width  = palette_dialog->col_width;
  entry_height = (ENTRY_HEIGHT * palette_dialog->zoom_factor);

  if ((y >= 0) && ((y + SPACING) < height))
    vspacing = SPACING;
  else if (y < 0)
    vspacing = SPACING + y;
  else
    vspacing = height - y;

  if (vspacing > 0)
    {
      if (y < 0)
	y += SPACING - vspacing;

      for (i = SPACING - vspacing; i < SPACING; i++, y++)
	{
	  p = buffer;
	  for (j = 0; j < width; j++)
	    {
	      *p++ = bcolor;
	      *p++ = bcolor;
	      *p++ = bcolor;
	    }
	  
	  if (column_highlight >= 0)
	    {
	      guchar *ph;

	      ph = &buffer[3 * column_highlight * (entry_width + SPACING)];

	      for (j = 0 ; j <= entry_width + SPACING; j++)
		{
		  *ph++ = ~bcolor;
		  *ph++ = ~bcolor;
		  *ph++ = ~bcolor;
		}

 	      gtk_preview_draw_row (GTK_PREVIEW (preview), buffer, 0,
				    y + entry_height + 1, width); 
	    }

	  gtk_preview_draw_row (GTK_PREVIEW (preview), buffer, 0, y, width);
	}

      if (y > SPACING)
	y += SPACING - vspacing;
    }
  else
    y += SPACING;

  vsize = (y >= 0) ? (entry_height) : (entry_height + y);

  if ((y >= 0) && ((y + entry_height) < height))
    vsize = entry_height;
  else if (y < 0)
    vsize = entry_height + y;
  else
    vsize = height - y;

  if (vsize > 0)
    {
      p = buffer;
      for (i = 0; i < n_colors; i++)
	{
	  for (j = 0; j < SPACING; j++)
	    {
	      *p++ = bcolor;
	      *p++ = bcolor;
	      *p++ = bcolor;
	    }

	  for (j = 0; j < entry_width; j++)
	    {
	      *p++ = colors[i * 3];
	      *p++ = colors[i * 3 + 1];
	      *p++ = colors[i * 3 + 2];
	    }
	}

      for (i = 0; i < (palette_dialog->columns - n_colors); i++)
	{
	  for (j = 0; j < (SPACING + entry_width); j++)
	    {
	      *p++ = 0;
	      *p++ = 0;
	      *p++ = 0;
	    }
	}

      for (j = 0; j < SPACING; j++)
	{
	  if (n_colors == column_highlight)
	    {
	      *p++ = ~bcolor;
	      *p++ = ~bcolor;
	      *p++ = ~bcolor;
	    }
	  else
	    {
	      *p++ = bcolor;
	      *p++ = bcolor;
	      *p++ = bcolor;
	    }
	}

      if (y < 0)
	y += entry_height - vsize;
      for (i = 0; i < vsize; i++, y++)
	{
	  if (column_highlight >= 0)
	    {
	      guchar *ph;

	      ph = &buffer[3 * column_highlight * (entry_width + SPACING)];

	      *ph++ = ~bcolor;
	      *ph++ = ~bcolor;
	      *ph++ = ~bcolor;
	      ph += 3 * (entry_width);
	      *ph++ = ~bcolor;
	      *ph++ = ~bcolor;
	      *ph++ = ~bcolor;
	    }

	  gtk_preview_draw_row (GTK_PREVIEW (preview), buffer, 0, y, width);
	}
      if (y > entry_height)
	y += entry_height - vsize;
    }
  else
    y += entry_height;

  return y;
}

static void
palette_dialog_draw_entries (PaletteDialog *palette_dialog,
			     gint           row_start,
			     gint           column_highlight)
{
  GimpPaletteEntry *entry;
  guchar           *buffer;
  guchar           *colors;
  GList            *list;
  gint              width, height;
  gint              entry_width;
  gint              entry_height;
  gint              index, y;

  if (! (palette_dialog && gimp_context_get_palette (palette_dialog->context)))
    return;

  width  = palette_dialog->color_area->requisition.width;
  height = palette_dialog->color_area->requisition.height;

  entry_width  = palette_dialog->col_width;
  entry_height = (ENTRY_HEIGHT * palette_dialog->zoom_factor);

  if (entry_width <= 0)
    return;

  colors = g_new (guchar, palette_dialog->columns * 3);
  buffer = g_new (guchar, width * 3);

  if (row_start < 0)
    {
      y = 0;
      list = gimp_context_get_palette (palette_dialog->context)->colors;
      column_highlight = -1;
    }
  else
    {
      y = (entry_height + SPACING) * row_start;
      list = g_list_nth (gimp_context_get_palette (palette_dialog->context)->colors,
			 row_start * palette_dialog->columns);
    }

  index = 0;

  for (; list; list = g_list_next (list))
    {
      entry = (GimpPaletteEntry *) list->data;

      gimp_rgb_get_uchar (&entry->color,
			  &colors[index * 3],
			  &colors[index * 3 + 1],
			  &colors[index * 3 + 2]);
      index++;

      if (index == palette_dialog->columns)
	{
	  index = 0;
	  y = palette_dialog_draw_color_row (colors, palette_dialog->columns, y,
					     column_highlight, buffer,
					     palette_dialog);

	  if (y >= height || row_start >= 0)
	    {
	      /* This row only */
	      gtk_widget_draw (palette_dialog->color_area, NULL);
	      g_free (buffer);
	      g_free (colors);
	      return;
	    }
	}
    }

  while (y < height)
    {
      y = palette_dialog_draw_color_row (colors, index, y, column_highlight,
					 buffer, palette_dialog);
      index = 0;
      if (row_start >= 0)
	break;
    }

  g_free (buffer);
  g_free (colors);

  if (! palette_dialog->freeze_update)
    gtk_widget_draw (palette_dialog->color_area, NULL);
}

static void
palette_dialog_scroll_top_left (PaletteDialog *palette_dialog)
{
  GtkAdjustment *hadj;
  GtkAdjustment *vadj;

  if (! (palette_dialog && palette_dialog->scrolled_window))
    return;

  hadj = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (palette_dialog->scrolled_window));
  vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (palette_dialog->scrolled_window));

  if (hadj)
    gtk_adjustment_set_value (hadj, 0.0);
  if (vadj)
    gtk_adjustment_set_value (vadj, 0.0);
}

static void
palette_dialog_redraw (PaletteDialog *palette_dialog)
{
  GtkWidget *parent;
  gint       vsize;
  gint       nrows;
  gint       n_entries;
  gint       preview_width;
  guint      width;

  if (! gimp_context_get_palette (palette_dialog->context))
    return;

  width = palette_dialog->color_area->parent->parent->parent->allocation.width;

  if ((palette_dialog->columns_valid) && palette_dialog->last_width == width)
    return;

  palette_dialog->last_width = width;
  palette_dialog->col_width = width / (palette_dialog->columns + 1) - SPACING;
  if (palette_dialog->col_width < 0) palette_dialog->col_width = 0;
  palette_dialog->columns_valid = TRUE;

  n_entries = gimp_context_get_palette (palette_dialog->context)->n_colors;
  nrows = n_entries / palette_dialog->columns;
  if (n_entries % palette_dialog->columns)
    nrows += 1;

  vsize = nrows * (SPACING + (gint) (ENTRY_HEIGHT * palette_dialog->zoom_factor)) + SPACING;

  parent = palette_dialog->color_area->parent->parent;
  gtk_widget_ref (palette_dialog->color_area->parent);
  gtk_container_remove (GTK_CONTAINER (parent),
			palette_dialog->color_area->parent);

  preview_width =
    (palette_dialog->col_width + SPACING) * palette_dialog->columns + SPACING;

  gtk_preview_size (GTK_PREVIEW (palette_dialog->color_area),
		    preview_width, vsize);

  gtk_container_add (GTK_CONTAINER (parent), palette_dialog->color_area->parent);
  gtk_widget_unref (palette_dialog->color_area->parent);

  if (palette_dialog->color)
    palette_dialog_draw_entries (palette_dialog,
				 palette_dialog->color->position / palette_dialog->columns,
				 palette_dialog->color->position % palette_dialog->columns);
  else
    palette_dialog_draw_entries (palette_dialog, -1, -1);
}

static void
palette_dialog_palette_changed (GimpContext *context,
				GimpPalette *palette,
				gpointer     data)
{
  PaletteDialog *palette_dialog;

  palette_dialog = (PaletteDialog *) data;

  /* FIXME */
  if (! palette)
    return;

  if (palette_dialog->color_notebook_active)
    {
      color_notebook_hide (palette_dialog->color_notebook);
      palette_dialog->color_notebook_active = FALSE;
    }

  if (palette_dialog->color_notebook)
    color_notebook_free (palette_dialog->color_notebook);
  palette_dialog->color_notebook = NULL;

  palette_dialog->columns_valid = FALSE;
  palette_dialog_redraw (palette_dialog);

  palette_dialog_scroll_top_left (palette_dialog);

  /*  Stop errors in case no colors are selected  */
  gtk_signal_handler_block (GTK_OBJECT (palette_dialog->color_name),
			    palette_dialog->entry_sig_id);
  gtk_entry_set_text (GTK_ENTRY (palette_dialog->color_name), _("Undefined")); 
  gtk_widget_set_sensitive (palette_dialog->color_name, FALSE);
  gtk_signal_handler_unblock (GTK_OBJECT (palette_dialog->color_name),
			      palette_dialog->entry_sig_id);
}

/*  the color name entry callback  *******************************************/

static void
palette_dialog_color_name_changed (GtkWidget *widget,
				   gpointer   data)
{
  PaletteDialog *palette_dialog;

  palette_dialog = (PaletteDialog *) data;

  g_return_if_fail (gimp_context_get_palette (palette_dialog->context) != NULL);

  if (palette_dialog->color->name)
    g_free (palette_dialog->color->name);
  palette_dialog->color->name = 
    g_strdup (gtk_entry_get_text (GTK_ENTRY (palette_dialog->color_name)));

  gimp_data_dirty (GIMP_DATA (gimp_context_get_palette (palette_dialog->context)));
}

/*  palette zoom functions & callbacks  **************************************/

static void
palette_dialog_zoomin_callback (GtkWidget *widget,
				gpointer   data)
{
  PaletteDialog *palette_dialog;

  palette_dialog = (PaletteDialog *) data;

  palette_dialog->zoom_factor += 0.1;

  palette_dialog_redraw_zoom (palette_dialog);
}

static void
palette_dialog_zoomout_callback (GtkWidget *widget,
				 gpointer   data)
{
  PaletteDialog *palette_dialog;

  palette_dialog = (PaletteDialog *) data;

  palette_dialog->zoom_factor -= 0.1;

  palette_dialog_redraw_zoom (palette_dialog);
}

static void
palette_dialog_redraw_zoom (PaletteDialog *palette_dialog)
{
  if (palette_dialog->zoom_factor > 4.0)
    {
      palette_dialog->zoom_factor = 4.0;
    }
  else if (palette_dialog->zoom_factor < 0.1)
    {
      palette_dialog->zoom_factor = 0.1;
    }

  palette_dialog->columns = COLUMNS;

  palette_dialog->columns_valid = FALSE;
  palette_dialog_redraw (palette_dialog); 

  palette_dialog_scroll_top_left (palette_dialog);
}

/*  the palette dialog action callbacks  **************************************/

static void
palette_dialog_import_callback (GtkWidget *widget,
				gpointer   data)
{
  palette_import_dialog_show ();
}

static void
palette_dialog_merge_callback (GtkWidget *widget,
			       gpointer   data)
{
  GtkWidget *qbox;

  qbox = gimp_query_string_box (_("Merge Palette"),
				gimp_standard_help_func,
				"dialogs/palette_editor/merge_palette.html",
				_("Enter a name for merged palette"),
				NULL,
				NULL, NULL,
				palette_dialog_do_merge_callback,
				data);
  gtk_widget_show (qbox);
}

static void
palette_dialog_do_merge_callback (GtkWidget *widget,
				  gchar     *palette_name,
				  gpointer   data)
{
  PaletteDialog    *palette_dialog;
  GimpPalette      *palette;
  GimpPalette      *new_palette;
  GimpPaletteEntry *entry;
  GList            *sel_list;

  new_palette = GIMP_PALETTE (gimp_palette_new (palette_name));

  palette_dialog = (PaletteDialog *) data;

  sel_list = GTK_LIST (GIMP_CONTAINER_LIST_VIEW (GIMP_DATA_FACTORY_VIEW (palette_dialog->view)->view)->gtk_list)->selection;

  while (sel_list)
    {
      GtkWidget *list_item;
      GList     *cols;

      list_item = GTK_WIDGET (sel_list->data);

      palette = (GimpPalette *)
	GIMP_PREVIEW (gtk_object_get_data (GTK_OBJECT (list_item),
					   "preview"))->viewable;

      if (palette)
	{
	  for (cols = palette->colors; cols; cols = g_list_next (cols))
	    {
	      entry = (GimpPaletteEntry *) cols->data;

	      gimp_palette_add_entry (new_palette,
				      entry->name,
				      &entry->color);
	    }
	}

      sel_list = sel_list->next;
    }

  gimp_container_add (global_palette_factory->container,
		      GIMP_OBJECT (new_palette));
}

static void
palette_dialog_close_callback (GtkWidget *widget,
			       gpointer   data)
{
  PaletteDialog *palette_dialog;

  palette_dialog = (PaletteDialog *) data;

  if (palette_dialog)
    {
      if (palette_dialog->color_notebook_active)
	{
	  color_notebook_hide (palette_dialog->color_notebook);
	  palette_dialog->color_notebook_active = FALSE;
	}

      if (palette_dialog == top_level_edit_palette)
	{
	  palette_import_dialog_destroy ();
	}

      if (GTK_WIDGET_VISIBLE (palette_dialog->shell))
	gtk_widget_hide (palette_dialog->shell);
    }
}

/*  the palette dialog color dnd callbacks  **********************************/

static void
palette_dialog_drag_color (GtkWidget *widget,
			   GimpRGB   *color,
			   gpointer   data)
{
  PaletteDialog *palette_dialog;

  palette_dialog = (PaletteDialog *) data;

  if (palette_dialog &&
      gimp_context_get_palette (palette_dialog->context) &&
      palette_dialog->dnd_color)
    {
      *color = palette_dialog->dnd_color->color;
    }
  else
    {
      gimp_rgba_set (color, 0.0, 0.0, 0.0, 1.0);
    }
}

static void
palette_dialog_drop_color (GtkWidget     *widget,
			   const GimpRGB *color,
			   gpointer       data)
{
  PaletteDialog *palette_dialog;

  palette_dialog = (PaletteDialog *) data;

  if (palette_dialog && gimp_context_get_palette (palette_dialog->context))
    {
      palette_dialog->color =
	gimp_palette_add_entry (gimp_context_get_palette (palette_dialog->context),
				NULL,
				(GimpRGB *) color);
    }
}

static void
palette_dialog_invalidate_preview (GimpPalette   *palette,
				   PaletteDialog *palette_dialog)
{
  if (palette == gimp_context_get_palette (palette_dialog->context))
    {
      palette_dialog->columns_valid = FALSE;
      palette_dialog_redraw (palette_dialog);
    }
}

static void
palette_dialog_edit_palette (GimpData *data)
{
  GimpPalette *palette;

  palette = GIMP_PALETTE (data);

  palette_create_edit (palette);
}
