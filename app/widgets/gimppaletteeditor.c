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
#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdatafactory.h"
#include "core/gimppalette.h"

#include "widgets/gimpcontainerlistview.h"
#include "widgets/gimpdatafactoryview.h"
#include "widgets/gimpdnd.h"
#include "widgets/gimppreview.h"

#include "color-area.h"
#include "color-notebook.h"
#include "palette-editor.h"
#include "palette-select.h"

#include "gimprc.h"

#include "libgimp/gimpintl.h"


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


struct _PaletteEditor
{
  GtkWidget        *shell;

  GtkWidget        *name;

  GtkWidget        *color_area;
  GtkWidget        *scrolled_window;
  GtkWidget        *color_name;

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
static void   palette_editor_name_activate         (GtkWidget      *widget,
						    PaletteEditor  *palette_editor);
static void   palette_editor_name_focus_out        (GtkWidget      *widget,
						    GdkEvent       *event,
						    PaletteEditor  *palette_editor);

static void   palette_editor_create_popup_menu     (PaletteEditor  *palette_editor);
static void   palette_editor_new_entry_callback    (GtkWidget      *widget,
						    gpointer        data);
static void   palette_editor_edit_entry_callback   (GtkWidget      *widget,
						    gpointer        data);
static void   palette_editor_delete_entry_callback (GtkWidget      *widget,
						    gpointer        data);
static void   palette_editor_color_notebook_callback (ColorNotebook      *color_notebook,
						      const GimpRGB      *color,
						      ColorNotebookState  state,
						      gpointer            data);

static gint   palette_editor_eventbox_button_press (GtkWidget      *widget,
						    GdkEventButton *bevent,
						    PaletteEditor  *palette_editor);
static gint   palette_editor_color_area_events     (GtkWidget      *widget,
						    GdkEvent       *event,
						    PaletteEditor  *palette_editor);
static void   palette_editor_draw_entries       (PaletteEditor  *palette,
						 gint            row_start,
						 gint            column_highlight);
static void   palette_editor_redraw             (PaletteEditor  *palette_editor);
static void   palette_editor_scroll_top_left    (PaletteEditor  *palette_editor);

static void   palette_editor_color_name_changed (GtkWidget      *widget,
						 gpointer        data);
static void   palette_editor_zoomin_callback    (GtkWidget      *widget,
						 gpointer        data);
static void   palette_editor_zoomout_callback   (GtkWidget      *widget,
						 gpointer        data);
static void   palette_editor_redraw_zoom        (PaletteEditor  *palette_editor);
static void   palette_editor_close_callback     (GtkWidget      *widget,
						 gpointer        data);
static void   palette_editor_palette_changed    (GimpContext    *context,
						 GimpPalette    *palette,
						 gpointer        data);
static void   palette_editor_drag_color         (GtkWidget      *widget,
						 GimpRGB        *color,
						 gpointer        data);
static void   palette_editor_drop_color         (GtkWidget      *widget,
						 const GimpRGB  *color,
						 gpointer        data);
static void   palette_editor_drop_palette       (GtkWidget      *widget,
						 GimpViewable   *viewable,
						 gpointer        data);
static void   palette_editor_invalidate_preview (GimpPalette    *palette,
						 PaletteEditor  *palette_editor);


/*  dnd stuff  */
static GtkTargetEntry color_palette_target_table[] =
{
  GIMP_TARGET_COLOR,
  GIMP_TARGET_PALETTE
};
static guint n_color_palette_targets = (sizeof (color_palette_target_table) /
					sizeof (color_palette_target_table[0]));


/*  called from color_picker.h  *********************************************/

void
palette_set_active_color (gint r,
			  gint g,
			  gint b,
			  gint state)
{
#ifdef __GNUC__
#warning FIXME: palette_set_active_color()
#endif
#if 0
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
    gimp_context_set_foreground (gimp_get_user_context (the_gimp), &color);
  else if (active_color == BACKGROUND)
    gimp_context_set_background (gimp_get_user_context (the_gimp), &color);
#endif
}

/*  called from palette_select.c  ********************************************/

void
palette_editor_set_palette (PaletteEditor *palette_editor,
			    GimpPalette   *palette)
{
  g_return_if_fail (palette_editor != NULL);
  g_return_if_fail (GIMP_IS_PALETTE (palette));

  gimp_context_set_palette (palette_editor->context, palette);
}


/*  the palette & palette edit dialog constructor  ***************************/

PaletteEditor *
palette_editor_new (Gimp *gimp)
{
  PaletteEditor *palette_editor;
  GtkWidget     *main_vbox;
  GtkWidget     *hbox;
  GtkWidget     *scrolledwindow;
  GtkWidget     *palette_region;
  GtkWidget     *entry;
  GtkWidget     *eventbox;
  GtkWidget     *alignment;
  GtkWidget     *button;
  GtkWidget     *image;

  palette_editor = g_new0 (PaletteEditor, 1);

  palette_editor->context = gimp_create_context (gimp, NULL, NULL);

  palette_editor->zoom_factor   = 1.0;
  palette_editor->columns       = COLUMNS;
  palette_editor->columns_valid = TRUE;
  palette_editor->freeze_update = FALSE;

  palette_editor->shell =
    gimp_dialog_new (_("Palette Editor"), "palette_editor",
		     gimp_standard_help_func,
		     "dialogs/palette_editor/palette_editor.html",
		     GTK_WIN_POS_NONE,
		     FALSE, TRUE, FALSE,

		     "_delete_event_", palette_editor_close_callback,
		     palette_editor, NULL, NULL, TRUE, TRUE,

		     NULL);

  gtk_dialog_set_has_separator (GTK_DIALOG (palette_editor->shell), FALSE);
  gtk_widget_hide (GTK_DIALOG (palette_editor->shell)->action_area);

  main_vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 4);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (palette_editor->shell)->vbox),
		     main_vbox);
  gtk_widget_show (main_vbox);

  /* Palette's name */
  palette_editor->name = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (main_vbox), palette_editor->name,
		      FALSE, FALSE, 0);
  gtk_widget_show (palette_editor->name);

  g_signal_connect (G_OBJECT (palette_editor->name), "activate",
		    G_CALLBACK (palette_editor_name_activate),
		    palette_editor);
  g_signal_connect (G_OBJECT (palette_editor->name), "focus_out_event",
		    G_CALLBACK (palette_editor_name_focus_out),
		    palette_editor);

  palette_editor->scrolled_window = scrolledwindow =
    gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_usize (scrolledwindow, -1, PREVIEW_HEIGHT);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_ALWAYS);
  gtk_box_pack_start (GTK_BOX (main_vbox), scrolledwindow, TRUE, TRUE, 0);
  gtk_widget_show (scrolledwindow);

  eventbox = gtk_event_box_new ();
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolledwindow),
					 eventbox);
  gtk_widget_show (eventbox);

  g_signal_connect (G_OBJECT (eventbox), "button_press_event",
		    G_CALLBACK (palette_editor_eventbox_button_press),
		    palette_editor);

  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0); 
  gtk_container_add (GTK_CONTAINER (eventbox), alignment);
  gtk_widget_show (alignment);

  palette_editor->color_area = palette_region =
    gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_set_dither (GTK_PREVIEW (palette_editor->color_area),
			  GDK_RGB_DITHER_MAX);
  gtk_preview_size (GTK_PREVIEW (palette_region), PREVIEW_WIDTH, PREVIEW_HEIGHT);

  gtk_widget_set_events (palette_region, PALETTE_EVENT_MASK);
  gtk_container_add (GTK_CONTAINER (alignment), palette_region);
  gtk_widget_show (palette_region);

  g_signal_connect (G_OBJECT (palette_editor->color_area), "event",
		    G_CALLBACK (palette_editor_color_area_events),
		    palette_editor);

  /*  dnd stuff  */
  gtk_drag_source_set (palette_region,
                       GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
                       color_palette_target_table, n_color_palette_targets,
                       GDK_ACTION_COPY | GDK_ACTION_MOVE);
  gimp_dnd_color_source_set (palette_region, palette_editor_drag_color,
			     palette_editor);

  gtk_drag_dest_set (eventbox,
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_DROP,
                     color_palette_target_table, n_color_palette_targets,
                     GDK_ACTION_COPY);
  gimp_dnd_color_dest_set (eventbox, palette_editor_drop_color, palette_editor);
  gimp_dnd_viewable_dest_set (eventbox, GIMP_TYPE_PALETTE,
			      palette_editor_drop_palette, palette_editor);

  /*  The color name entry  */
  hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  entry = palette_editor->color_name = gtk_entry_new ();
  gtk_widget_show (entry);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_entry_set_text (GTK_ENTRY (entry), _("Undefined"));
  gtk_widget_set_sensitive (entry, FALSE);

  palette_editor->entry_sig_id =
    g_signal_connect (G_OBJECT (entry), "changed",
		      G_CALLBACK (palette_editor_color_name_changed),
		      palette_editor);

  /*  + and - buttons  */
  button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  image = gtk_image_new_from_stock (GTK_STOCK_ZOOM_IN, GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (palette_editor_zoomin_callback),
		    palette_editor);

  button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  image = gtk_image_new_from_stock (GTK_STOCK_ZOOM_OUT, GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (palette_editor_zoomout_callback),
		    palette_editor);

  g_signal_connect (G_OBJECT (palette_editor->context), "palette_changed",
		    G_CALLBACK (palette_editor_palette_changed),
		    palette_editor);

  palette_editor->invalidate_preview_handler_id =
    gimp_container_add_handler (gimp->palette_factory->container,
				"invalidate_preview",
				G_CALLBACK (palette_editor_invalidate_preview),
				palette_editor);

  gtk_widget_realize (palette_editor->shell);

  palette_editor_create_popup_menu (palette_editor);

  return palette_editor;
}

/*  private functions  */

static void
palette_editor_name_activate (GtkWidget     *widget,
			      PaletteEditor *palette_editor)
{
  GimpPalette *palette;
  const gchar *entry_text;

  palette = gimp_context_get_palette (palette_editor->context);

  entry_text = gtk_entry_get_text (GTK_ENTRY (widget));

  gimp_object_set_name (GIMP_OBJECT (palette), entry_text);
}

static void
palette_editor_name_focus_out (GtkWidget     *widget,
			       GdkEvent      *event,
			       PaletteEditor *palette_editor)
{
  palette_editor_name_activate (widget, palette_editor);
}

/*  the palette dialog popup menu & callbacks  *******************************/

static void
palette_editor_create_popup_menu (PaletteEditor *palette_editor)
{
  GtkWidget *menu;
  GtkWidget *menu_item;

  palette_editor->popup_menu = menu = gtk_menu_new ();

  menu_item = gtk_menu_item_new_with_label (_("New"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  gtk_widget_show (menu_item);

  g_signal_connect (G_OBJECT (menu_item), "activate", 
		    G_CALLBACK (palette_editor_new_entry_callback),
		    palette_editor);

  menu_item = gtk_menu_item_new_with_label (_("Edit"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  gtk_widget_show (menu_item);

  g_signal_connect (G_OBJECT (menu_item), "activate", 
		    G_CALLBACK (palette_editor_edit_entry_callback),
		    palette_editor);

  palette_editor->edit_menu_item = menu_item;

  menu_item = gtk_menu_item_new_with_label (_("Delete"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  gtk_widget_show (menu_item);

  g_signal_connect (G_OBJECT (menu_item), "activate", 
		    G_CALLBACK (palette_editor_delete_entry_callback),
		    palette_editor);

  palette_editor->delete_menu_item = menu_item;
}

static void
palette_editor_new_entry_callback (GtkWidget *widget,
				   gpointer   data)
{
  PaletteEditor *palette_editor;
  GimpContext   *user_context;
  GimpRGB        color;

  palette_editor = (PaletteEditor *) data;

  if (! (palette_editor && gimp_context_get_palette (palette_editor->context)))
    return;

  user_context = gimp_get_user_context (palette_editor->context->gimp);

  if (active_color == FOREGROUND)
    gimp_context_get_foreground (user_context, &color);
  else if (active_color == BACKGROUND)
    gimp_context_get_background (user_context, &color);

  palette_editor->color =
    gimp_palette_add_entry (gimp_context_get_palette (palette_editor->context),
			    NULL,
			    &color);
}

static void
palette_editor_edit_entry_callback (GtkWidget *widget,
				    gpointer   data)
{
  PaletteEditor *palette_editor;

  palette_editor = (PaletteEditor *) data;

  if (! (palette_editor && gimp_context_get_palette (palette_editor->context)
	 && palette_editor->color))
    return;

  if (! palette_editor->color_notebook)
    {
      palette_editor->color_notebook =
	color_notebook_new (_("Edit Palette Color"),
			    (const GimpRGB *) &palette_editor->color->color,
			    palette_editor_color_notebook_callback,
			    palette_editor,
			    FALSE,
			    FALSE);
      palette_editor->color_notebook_active = TRUE;
    }
  else
    {
      if (! palette_editor->color_notebook_active)
	{
	  color_notebook_show (palette_editor->color_notebook);
	  palette_editor->color_notebook_active = TRUE;
	}

      color_notebook_set_color (palette_editor->color_notebook,
				&palette_editor->color->color);
    }
}

static void
palette_editor_delete_entry_callback (GtkWidget *widget,
				      gpointer   data)
{
  PaletteEditor *palette_editor;

  palette_editor = (PaletteEditor *) data;

  if (! (palette_editor && gimp_context_get_palette (palette_editor->context) &&
	 palette_editor->color))
    return;

  gimp_palette_delete_entry (gimp_context_get_palette (palette_editor->context),
			     palette_editor->color);
}

static void
palette_editor_color_notebook_callback (ColorNotebook      *color_notebook,
					const GimpRGB      *color,
					ColorNotebookState  state,
					gpointer            data)
{
  PaletteEditor *palette_editor;
  GimpContext   *user_context;
  GimpPalette   *palette;

  palette_editor = (PaletteEditor *) data;

  palette = gimp_context_get_palette (palette_editor->context);

  user_context = gimp_get_user_context (palette_editor->context->gimp);

  switch (state)
    {
    case COLOR_NOTEBOOK_UPDATE:
      break;

    case COLOR_NOTEBOOK_OK:
      if (palette_editor->color)
	{
	  palette_editor->color->color = *color;

	  /*  Update either foreground or background colors  */
	  if (active_color == FOREGROUND)
	    gimp_context_set_foreground (user_context, color);
	  else if (active_color == BACKGROUND)
	    gimp_context_set_background (user_context, color);

	  gimp_data_dirty (GIMP_DATA (palette));
	}

      /* Fallthrough */
    case COLOR_NOTEBOOK_CANCEL:
      if (palette_editor->color_notebook_active)
	{
	  color_notebook_hide (palette_editor->color_notebook);
	  palette_editor->color_notebook_active = FALSE;
	}
    }
}

/*  the color area event callbacks  ******************************************/

static gint
palette_editor_eventbox_button_press (GtkWidget      *widget,
				      GdkEventButton *bevent,
				      PaletteEditor  *palette_editor)
{
  if (gtk_get_event_widget ((GdkEvent *) bevent) == palette_editor->color_area)
    return FALSE;

  if (bevent->button == 3)
    {
      if (GTK_WIDGET_SENSITIVE (palette_editor->edit_menu_item))
	{
	  gtk_widget_set_sensitive (palette_editor->edit_menu_item, FALSE);
	  gtk_widget_set_sensitive (palette_editor->delete_menu_item, FALSE);
	}

      gtk_menu_popup (GTK_MENU (palette_editor->popup_menu), NULL, NULL, 
		      NULL, NULL, 3,
		      bevent->time);
    }

  return TRUE;
}

static gint
palette_editor_color_area_events (GtkWidget     *widget,
				  GdkEvent      *event,
				  PaletteEditor *palette_editor)
{
  GimpContext    *user_context;
  GdkEventButton *bevent;
  GList          *list;
  gint            entry_width;
  gint            entry_height;
  gint            row, col;
  gint            pos;

  user_context = gimp_get_user_context (palette_editor->context->gimp);

  switch (event->type)
    {
    case GDK_EXPOSE:
      palette_editor_redraw (palette_editor);
      break;

    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;
      entry_width  = palette_editor->col_width + SPACING;
      entry_height = (ENTRY_HEIGHT * palette_editor->zoom_factor) +  SPACING;
      col = (bevent->x - 1) / entry_width;
      row = (bevent->y - 1) / entry_height;
      pos = row * palette_editor->columns + col;

      if (gimp_context_get_palette (palette_editor->context))
	list = g_list_nth (gimp_context_get_palette (palette_editor->context)->colors, pos);
      else
	list = NULL;

      if (list)
	palette_editor->dnd_color = list->data;
      else
	palette_editor->dnd_color = NULL;

      if ((bevent->button == 1 || bevent->button == 3) &&
	  gimp_context_get_palette (palette_editor->context))
	{
	  if (list)
	    {
	      if (palette_editor->color)
		{
		  palette_editor->freeze_update = TRUE;
 		  palette_editor_draw_entries (palette_editor, -1, -1);
		  palette_editor->freeze_update = FALSE;
		}
	      palette_editor->color = (GimpPaletteEntry *) list->data;

	      if (active_color == FOREGROUND)
		{
		  if (bevent->state & GDK_CONTROL_MASK)
		    gimp_context_set_background (user_context,
						 &palette_editor->color->color);
		  else
		    gimp_context_set_foreground (user_context,
						 &palette_editor->color->color);
		}
	      else if (active_color == BACKGROUND)
		{
		  if (bevent->state & GDK_CONTROL_MASK)
		    gimp_context_set_foreground (user_context,
						 &palette_editor->color->color);
		  else
		    gimp_context_set_background (user_context,
						 &palette_editor->color->color);
		}

	      palette_editor_draw_entries (palette_editor, row, col);

	      /*  Update the active color name  */
	      g_print ("color name before: >>%s<<\n",
		       palette_editor->color->name);

	      g_signal_handler_block (G_OBJECT (palette_editor->color_name),
				      palette_editor->entry_sig_id);

	      gtk_entry_set_text (GTK_ENTRY (palette_editor->color_name),
				  palette_editor->color->name);

	      g_signal_handler_unblock (G_OBJECT (palette_editor->color_name),
					palette_editor->entry_sig_id);

	      g_print ("color name after: >>%s<<\n",
		       palette_editor->color->name);

	      gtk_widget_set_sensitive (palette_editor->color_name, TRUE);
	      /* palette_update_current_entry (palette_editor); */

	      if (bevent->button == 3)
		{
		  if (! GTK_WIDGET_SENSITIVE (palette_editor->edit_menu_item))
		    {
		      gtk_widget_set_sensitive (palette_editor->edit_menu_item, TRUE);
		      gtk_widget_set_sensitive (palette_editor->delete_menu_item, TRUE);
		    }

		  gtk_menu_popup (GTK_MENU (palette_editor->popup_menu),
				  NULL, NULL, 
				  NULL, NULL, 3,
				  bevent->time);
		}
	    }
	  else
	    {
	      if (bevent->button == 3)
		{
		  if (GTK_WIDGET_SENSITIVE (palette_editor->edit_menu_item))
		    {
		      gtk_widget_set_sensitive (palette_editor->edit_menu_item, FALSE);
		      gtk_widget_set_sensitive (palette_editor->delete_menu_item, FALSE);
		    }

		  gtk_menu_popup (GTK_MENU (palette_editor->popup_menu),
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
palette_editor_draw_color_row (guchar         *colors,
			       gint            n_colors,
			       gint            y,
			       gint            column_highlight,
			       guchar         *buffer,
			       PaletteEditor  *palette_editor)
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

  if (! palette_editor)
    return -1;

  preview = palette_editor->color_area;

  bcolor = 0;

  width        = preview->requisition.width;
  height       = preview->requisition.height;
  entry_width  = palette_editor->col_width;
  entry_height = (ENTRY_HEIGHT * palette_editor->zoom_factor);

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

      for (i = 0; i < (palette_editor->columns - n_colors); i++)
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
palette_editor_draw_entries (PaletteEditor *palette_editor,
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

  if (! (palette_editor && gimp_context_get_palette (palette_editor->context)))
    return;

  width  = palette_editor->color_area->requisition.width;
  height = palette_editor->color_area->requisition.height;

  entry_width  = palette_editor->col_width;
  entry_height = (ENTRY_HEIGHT * palette_editor->zoom_factor);

  if (entry_width <= 0)
    return;

  colors = g_new (guchar, palette_editor->columns * 3);
  buffer = g_new (guchar, width * 3);

  if (row_start < 0)
    {
      y = 0;
      list = gimp_context_get_palette (palette_editor->context)->colors;
      column_highlight = -1;
    }
  else
    {
      y = (entry_height + SPACING) * row_start;
      list = g_list_nth (gimp_context_get_palette (palette_editor->context)->colors,
			 row_start * palette_editor->columns);
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

      if (index == palette_editor->columns)
	{
	  index = 0;
	  y = palette_editor_draw_color_row (colors, palette_editor->columns, y,
					     column_highlight, buffer,
					     palette_editor);

	  if (y >= height || row_start >= 0)
	    {
	      /* This row only */
	      gtk_widget_draw (palette_editor->color_area, NULL);
	      g_free (buffer);
	      g_free (colors);
	      return;
	    }
	}
    }

  while (y < height)
    {
      y = palette_editor_draw_color_row (colors, index, y, column_highlight,
					 buffer, palette_editor);
      index = 0;
      if (row_start >= 0)
	break;
    }

  g_free (buffer);
  g_free (colors);

  if (! palette_editor->freeze_update)
    gtk_widget_draw (palette_editor->color_area, NULL);
}

static void
palette_editor_scroll_top_left (PaletteEditor *palette_editor)
{
  GtkAdjustment *hadj;
  GtkAdjustment *vadj;

  if (! (palette_editor && palette_editor->scrolled_window))
    return;

  hadj = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (palette_editor->scrolled_window));
  vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (palette_editor->scrolled_window));

  if (hadj)
    gtk_adjustment_set_value (hadj, 0.0);
  if (vadj)
    gtk_adjustment_set_value (vadj, 0.0);
}

static void
palette_editor_redraw (PaletteEditor *palette_editor)
{
  gint  vsize;
  gint  nrows;
  gint  n_entries;
  gint  preview_width;
  guint width;

  if (! gimp_context_get_palette (palette_editor->context))
    return;

  width = palette_editor->color_area->parent->parent->parent->allocation.width;

  if ((palette_editor->columns_valid) && palette_editor->last_width == width)
    return;

  palette_editor->last_width = width;
  palette_editor->col_width = width / (palette_editor->columns + 1) - SPACING;
  if (palette_editor->col_width < 0) palette_editor->col_width = 0;
  palette_editor->columns_valid = TRUE;

  n_entries = gimp_context_get_palette (palette_editor->context)->n_colors;
  nrows = n_entries / palette_editor->columns;
  if (n_entries % palette_editor->columns)
    nrows += 1;

  vsize = nrows * (SPACING + (gint) (ENTRY_HEIGHT * palette_editor->zoom_factor)) + SPACING;

  preview_width =
    (palette_editor->col_width + SPACING) * palette_editor->columns + SPACING;

  gtk_preview_size (GTK_PREVIEW (palette_editor->color_area),
		    preview_width, vsize);
  gtk_widget_queue_resize (palette_editor->color_area);

  palette_editor_draw_entries (palette_editor, -1, -1);

  if (palette_editor->color)
    {
      palette_editor_draw_entries (palette_editor,
				   palette_editor->color->position / palette_editor->columns,
				   palette_editor->color->position % palette_editor->columns);
    }
}

static void
palette_editor_palette_changed (GimpContext *context,
				GimpPalette *palette,
				gpointer     data)
{
  PaletteEditor *palette_editor;

  palette_editor = (PaletteEditor *) data;

  /* FIXME */
  if (! palette)
    return;

  gtk_entry_set_text (GTK_ENTRY (palette_editor->name),
		      gimp_object_get_name (GIMP_OBJECT (palette)));

  if (palette_editor->color_notebook_active)
    {
      color_notebook_hide (palette_editor->color_notebook);
      palette_editor->color_notebook_active = FALSE;
    }

  if (palette_editor->color_notebook)
    color_notebook_free (palette_editor->color_notebook);
  palette_editor->color_notebook = NULL;

  palette_editor->columns_valid = FALSE;
  palette_editor_redraw (palette_editor);

  palette_editor_scroll_top_left (palette_editor);

  palette_editor->color = NULL;

  /*  Stop errors in case no colors are selected  */
  g_signal_handler_block (G_OBJECT (palette_editor->color_name),
			  palette_editor->entry_sig_id);

  gtk_entry_set_text (GTK_ENTRY (palette_editor->color_name), _("Undefined")); 

  g_signal_handler_unblock (G_OBJECT (palette_editor->color_name),
			    palette_editor->entry_sig_id);

  gtk_widget_set_sensitive (palette_editor->color_name, FALSE);
}

/*  the color name entry callback  *******************************************/

static void
palette_editor_color_name_changed (GtkWidget *widget,
				   gpointer   data)
{
  PaletteEditor *palette_editor;

  palette_editor = (PaletteEditor *) data;

  g_return_if_fail (gimp_context_get_palette (palette_editor->context) != NULL);

  if (palette_editor->color->name)
    g_free (palette_editor->color->name);
  palette_editor->color->name = 
    g_strdup (gtk_entry_get_text (GTK_ENTRY (palette_editor->color_name)));

  gimp_data_dirty (GIMP_DATA (gimp_context_get_palette (palette_editor->context)));
}

/*  palette zoom functions & callbacks  **************************************/

static void
palette_editor_zoomin_callback (GtkWidget *widget,
				gpointer   data)
{
  PaletteEditor *palette_editor;

  palette_editor = (PaletteEditor *) data;

  palette_editor->zoom_factor += 0.1;

  palette_editor_redraw_zoom (palette_editor);
}

static void
palette_editor_zoomout_callback (GtkWidget *widget,
				 gpointer   data)
{
  PaletteEditor *palette_editor;

  palette_editor = (PaletteEditor *) data;

  palette_editor->zoom_factor -= 0.1;

  palette_editor_redraw_zoom (palette_editor);
}

static void
palette_editor_redraw_zoom (PaletteEditor *palette_editor)
{
  if (palette_editor->zoom_factor > 4.0)
    {
      palette_editor->zoom_factor = 4.0;
    }
  else if (palette_editor->zoom_factor < 0.1)
    {
      palette_editor->zoom_factor = 0.1;
    }

  palette_editor->columns = COLUMNS;

  palette_editor->columns_valid = FALSE;
  palette_editor_redraw (palette_editor); 

  palette_editor_scroll_top_left (palette_editor);
}

/*  the palette dialog action callbacks  **************************************/

static void
palette_editor_close_callback (GtkWidget *widget,
			       gpointer   data)
{
  PaletteEditor *palette_editor;

  palette_editor = (PaletteEditor *) data;

  if (palette_editor)
    {
      if (palette_editor->color_notebook_active)
	{
	  color_notebook_hide (palette_editor->color_notebook);
	  palette_editor->color_notebook_active = FALSE;
	}

      if (GTK_WIDGET_VISIBLE (palette_editor->shell))
	gtk_widget_hide (palette_editor->shell);
    }
}

/*  the palette dialog color dnd callbacks  **********************************/

static void
palette_editor_drag_color (GtkWidget *widget,
			   GimpRGB   *color,
			   gpointer   data)
{
  PaletteEditor *palette_editor;

  palette_editor = (PaletteEditor *) data;

  if (palette_editor &&
      gimp_context_get_palette (palette_editor->context) &&
      palette_editor->dnd_color)
    {
      *color = palette_editor->dnd_color->color;
    }
  else
    {
      gimp_rgba_set (color, 0.0, 0.0, 0.0, 1.0);
    }
}

static void
palette_editor_drop_color (GtkWidget     *widget,
			   const GimpRGB *color,
			   gpointer       data)
{
  PaletteEditor *palette_editor;

  palette_editor = (PaletteEditor *) data;

  if (palette_editor && gimp_context_get_palette (palette_editor->context))
    {
      palette_editor->color =
	gimp_palette_add_entry (gimp_context_get_palette (palette_editor->context),
				NULL,
				(GimpRGB *) color);
    }
}

static void
palette_editor_drop_palette (GtkWidget    *widget,
			     GimpViewable *viewable,
			     gpointer      data)
{
  PaletteEditor *palette_editor;
  GimpPalette   *palette;

  palette_editor = (PaletteEditor *) data;
  palette        = GIMP_PALETTE (viewable);

  if (palette_editor)
    {
      gimp_context_set_palette (palette_editor->context, palette);
    }
}

static void
palette_editor_invalidate_preview (GimpPalette   *palette,
				   PaletteEditor *palette_editor)
{
  if (palette == gimp_context_get_palette (palette_editor->context))
    {
      palette_editor->columns_valid = FALSE;
      palette_editor_redraw (palette_editor);
    }
}
