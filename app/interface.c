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

#include <stdlib.h>
#include <string.h>
#include "appenv.h"
#include "actionarea.h"
#include "app_procs.h"
#include "buildmenu.h"
#include "colormaps.h"
#include "color_area.h"
#include "commands.h"
#include "devices.h"
#include "dialog_handler.h"
#include "disp_callbacks.h"
#include "errors.h"
#include "fileops.h"
#include "gdisplay.h"
#include "gdisplay_ops.h"
#include "gimage.h"
#include "gimprc.h"
#include "general.h"
#include "indicator_area.h"
#include "interface.h"
#include "menus.h"
#include "qmask.h"
#include "session.h"
#include "tools.h"
#include "libgimp/gimpsizeentry.h"

#include "pixmaps.h"
#include "pixmaps/qmasksel.xpm"
#include "pixmaps/qmasknosel.xpm"

#include "libgimp/gimpintl.h"

/*  local functions  */
static void  tools_select_update   (GtkWidget *widget,
				    gpointer   data);
static gint  tools_button_press    (GtkWidget *widget,
				    GdkEventButton *bevent,
				    gpointer   data);
static void  gdisplay_destroy      (GtkWidget *widget,
				    GDisplay  *display);

static gint  gdisplay_delete       (GtkWidget *widget,
				    GdkEvent *,
				    GDisplay  *display);

static void  toolbox_destroy       (void);
static gint  toolbox_delete        (GtkWidget *,
				    GdkEvent *,
				    gpointer);
static gint  toolbox_check_device  (GtkWidget *,
				    GdkEvent *,
				    gpointer);

static GdkPixmap *create_pixmap    (GdkWindow  *parent,
				    GdkBitmap **mask,
				    char      **data,
				    int         width,
				    int         height);
static void gimp_set_drop_open     (GtkWidget *);
static void gimp_dnd_data_received (GtkWidget *,
				    GdkDragContext *,
				    gint,
				    gint,
				    GtkSelectionData *,
				    guint,
				    guint);
static gint gimp_dnd_open_files    (gchar *);

static int pixmap_colors[8][3] =
{
  { 0x00, 0x00, 0x00 }, /* a - 0   */
  { 0x24, 0x24, 0x24 }, /* b - 36  */
  { 0x49, 0x49, 0x49 }, /* c - 73  */
  { 0x6D, 0x6D, 0x6D }, /* d - 109 */
  { 0x92, 0x92, 0x92 }, /* e - 146 */
  { 0xB6, 0xB6, 0xB6 }, /* f - 182 */
  { 0xDB, 0xDB, 0xDB }, /* g - 219 */
  { 0xFF, 0xFF, 0xFF }, /* h - 255 */
};

#define COLUMNS   3
#define ROWS      8
#define MARGIN    2

/*  Widgets for each tool button--these are used from command.c to activate on
 *  tool selection via both menus and keyboard accelerators.
 */
GtkWidget   * tool_label;
GtkTooltips * tool_tips;

static GdkColor    colors[12];
static GtkWidget * toolbox_shell = NULL;

enum {
  TARGET_URI_LIST,
  TARGET_TEXT_PLAIN,
  TARGET_URL
} TargetType;

static
GtkTargetEntry dnd_target_table[] =
{
  { "text/uri-list", 0, TARGET_URI_LIST },
  { "text/plain", 0, TARGET_TEXT_PLAIN },
  { "_NETSCAPE_URL",0, TARGET_URL }
};

static guint
dnd_n_targets = sizeof(dnd_target_table) / sizeof(dnd_target_table[0]);

static void
tools_select_update (GtkWidget *w,
		     gpointer   data)
{
  ToolType tool_type;

  tool_type = (ToolType) data;

  if ((tool_type != -1) && GTK_TOGGLE_BUTTON (w)->active)
    tools_select (tool_type);
}

static gint
tools_button_press (GtkWidget      *w,
		    GdkEventButton *event,
		    gpointer        data)
{
  if ((event->type == GDK_2BUTTON_PRESS) &&
      (event->button == 1))
    tools_options_dialog_show ();

  return FALSE;
}

static gint
toolbox_delete (GtkWidget *w,
		GdkEvent  *e,
		gpointer   data)
{
  app_exit (0);

  return TRUE;
}

static void
toolbox_destroy ()
{
  app_exit_finish ();
}

static gint
toolbox_check_device (GtkWidget *w,
		      GdkEvent  *e,
		      gpointer   data)
{
  devices_check_change (e);

  return FALSE;
}

static void
gdisplay_destroy (GtkWidget *w,
		  GDisplay  *gdisp)
{
  gdisplay_remove_and_delete (gdisp);
}

static gint
gdisplay_delete (GtkWidget *w,
		 GdkEvent  *e,
		 GDisplay  *gdisp)
{
  gdisplay_close_window (gdisp, FALSE);

  return TRUE;
}

static void
allocate_colors (GtkWidget *parent)
{
  GdkColormap *colormap;
  int i;

  gtk_widget_realize (parent);
  colormap = gdk_window_get_colormap (parent->window);

  for (i = 0; i < 8; i++)
    {
      colors[i].red = pixmap_colors[i][0] << 8;
      colors[i].green = pixmap_colors[i][1] << 8;
      colors[i].blue = pixmap_colors[i][2] << 8;

      gdk_color_alloc (colormap, &colors[i]);
    }

  colors[8] = parent->style->bg[GTK_STATE_NORMAL];
  gdk_color_alloc (colormap, &colors[8]);

  colors[9] = parent->style->bg[GTK_STATE_ACTIVE];
  gdk_color_alloc (colormap, &colors[9]);

  colors[10] = parent->style->bg[GTK_STATE_PRELIGHT];
  gdk_color_alloc (colormap, &colors[10]);

  /* postit yellow (khaki) as background for tooltips */
  colors[11].red = 61669;
  colors[11].green = 59113;
  colors[11].blue = 35979;
  gdk_color_alloc (colormap, &colors[11]);
}

#ifdef icky
static void
create_qmask_area (GtkWidget *parent)
{
  GtkWidget *frame;
  GtkWidget *alignment;
  GdkPixmap *default_pixmap;
  GdkPixmap *swap_pixmap;

  gtk_widget_realize (parent);

  default_pixmap = create_pixmap (parent->window, NULL, default_bits,
				  default_width, default_height);
  swap_pixmap    = create_pixmap (parent->window, NULL, swap_bits,
				  swap_width, swap_height);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_box_pack_start (GTK_BOX (parent), frame, FALSE, FALSE, 0);
  gtk_widget_realize (frame);

  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_set_border_width (GTK_CONTAINER (alignment), 3);
  gtk_container_add (GTK_CONTAINER (frame), alignment);

  gtk_container_add (GTK_CONTAINER (alignment), qmask_area);
  gtk_widget_show (qmask_off);
  gtk_widget_show (qmask_on);
  gtk_widget_show (alignment);
  gtk_widget_show (frame);
}
#endif /* icky */

static void
create_indicator_area (GtkWidget *parent)
{
  GtkWidget *frame;
  GtkWidget *alignment;
  GtkWidget *ind_area;
  GdkPixmap *default_pixmap;
  GdkPixmap *swap_pixmap;

  gtk_widget_realize (parent);

  default_pixmap = create_pixmap (parent->window, NULL, default_bits,
				  default_width, default_height);
  swap_pixmap    = create_pixmap (parent->window, NULL, swap_bits,
				  swap_width, swap_height);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_box_pack_start (GTK_BOX (parent), frame, FALSE, FALSE, 0);
  gtk_widget_realize (frame);

  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_set_border_width (GTK_CONTAINER (alignment), 3);
  gtk_container_add (GTK_CONTAINER (frame), alignment);

  ind_area = indicator_area_create (54, 42);
  gtk_container_add (GTK_CONTAINER (alignment), ind_area);
  gtk_widget_show (ind_area);
  gtk_widget_show (alignment);
  gtk_widget_show (frame);
}

static void
create_color_area (GtkWidget *parent)
{
  GtkWidget *frame;
  GtkWidget *alignment;
  GtkWidget *col_area;
  GdkPixmap *default_pixmap;
  GdkPixmap *swap_pixmap;

  gtk_widget_realize (parent);

  default_pixmap = create_pixmap (parent->window, NULL, default_bits,
				  default_width, default_height);
  swap_pixmap    = create_pixmap (parent->window, NULL, swap_bits,
				  swap_width, swap_height);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_box_pack_start (GTK_BOX (parent), frame, FALSE, FALSE, 0);
  gtk_widget_realize (frame);

  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_set_border_width (GTK_CONTAINER (alignment), 3);
  gtk_container_add (GTK_CONTAINER (frame), alignment);

  col_area = color_area_create (54, 42, default_pixmap, swap_pixmap);
  gtk_container_add (GTK_CONTAINER (alignment), col_area);
  gtk_tooltips_set_tip (tool_tips, col_area, _("Foreground & background colors.  The black "
			                       "and white squares reset colors.  The arrows swap colors. Double "
			                       "click to select a color from a colorrequester."),
			NULL);
  gtk_widget_show (col_area);
  gtk_widget_show (alignment);
  gtk_widget_show (frame);
}

GdkPixmap *
create_tool_pixmap (GtkWidget *parent,
		    ToolType   type)
{
  /*
   * FIXME this really should be dones without using the #defined tool names
   * but it should work this way for now
   */
  if (type == SCALE || type == SHEAR || type == PERSPECTIVE)
    type = ROTATE;

  if (tool_info[(int) type].icon_data)
    return create_pixmap (parent->window, NULL,
			  tool_info[(int) type].icon_data,
			  22, 22);
  else
    return create_pixmap (parent->window, NULL,
			  dialog_bits,
			  22, 22);

  g_return_val_if_fail (FALSE, NULL);

  return NULL;	/* not reached */
}

static void
create_tools (GtkWidget *parent)
{
  GtkWidget *table;
  GtkWidget *button;
  GtkWidget *alignment;
  GtkWidget *pixmap;
  GSList *group;
  gint i, j;

  /*create_logo (parent);*/
  table = gtk_table_new (ROWS, COLUMNS, TRUE);
  gtk_box_pack_start (GTK_BOX (parent), table, TRUE, TRUE, 0);
  gtk_widget_realize (table);

  group = NULL;

  i = 0;
  for (j = 0; j < num_tools; j++)
    {
      if (tool_info[j].icon_data)
	{
	  tool_info[j].tool_widget = button = gtk_radio_button_new (group);
	  gtk_container_set_border_width (GTK_CONTAINER (button), 0);
	  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));

	  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);

	  gtk_table_attach (GTK_TABLE (table), button,
			    (i % 3), (i % 3) + 1,
			    (i / 3), (i / 3) + 1,
			    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			    0, 0);

	  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
	  gtk_container_set_border_width (GTK_CONTAINER (alignment), 0);
	  gtk_container_add (GTK_CONTAINER (button), alignment);

	  pixmap = create_pixmap_widget (table->window, tool_info[j].icon_data, 22, 22);
	  gtk_container_add (GTK_CONTAINER (alignment), pixmap);

	  gtk_signal_connect (GTK_OBJECT (button), "toggled",
			      (GtkSignalFunc) tools_select_update,
			      (gpointer) tool_info[j].tool_id);

	  gtk_signal_connect (GTK_OBJECT (button), "button_press_event",
			      (GtkSignalFunc) tools_button_press,
			      (gpointer) tool_info[j].tool_id);

	  gtk_tooltips_set_tip (tool_tips, button, gettext(tool_info[j].tool_desc), tool_info[i].private_tip);

	  gtk_widget_show (pixmap);
	  gtk_widget_show (alignment);
	  gtk_widget_show (button);
	  i++;
	}
      else
	{
	  tool_info[j].tool_widget = button = gtk_radio_button_new (group);
	  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));

	  gtk_signal_connect (GTK_OBJECT (button), "clicked",
			      (GtkSignalFunc) tools_select_update,
			      (gpointer) tool_info[j].tool_id);
	}
    }
  gtk_widget_show (table);
}

static GdkPixmap *
create_pixmap (GdkWindow *parent, GdkBitmap **mask,
	       char **data, int width, int height)
{
  GdkPixmap *pixmap;
  GdkImage *image;
  GdkGC *gc;
  GdkVisual *visual;
  GdkColormap *cmap;
  gint r, s, t, cnt;
  guchar *mem;
  guchar value;
  guint32 pixel;

  visual = gdk_window_get_visual (parent);
  cmap = gdk_window_get_colormap (parent);
  image = gdk_image_new (GDK_IMAGE_NORMAL, visual, width, height);
  pixmap = gdk_pixmap_new (parent, width, height, -1);
  gc = NULL;

  if (mask)
    {
      GdkColor tmp_color;

      *mask = gdk_pixmap_new (parent, width, height, 1);
      gc = gdk_gc_new (*mask);
      gdk_draw_rectangle (*mask, gc, TRUE, 0, 0, -1, -1);

      tmp_color.pixel = 1;
      gdk_gc_set_foreground (gc, &tmp_color);
    }

  for (r = 0; r < height; r++)
    {
      mem = image->mem;
      mem += image->bpl * r;

      for (s = 0, cnt = 0; s < width; s++)
	{
	  value = data[r][s];

	  if (value == '.')
	    {
	      pixel = colors[8].pixel;

	      if (mask)
		{
		  if (cnt < s)
		    gdk_draw_line (*mask, gc, cnt, r, s - 1, r);
		  cnt = s + 1;
		}
	    }
	  else
	    {
	      pixel = colors[value - 'a'].pixel;
	    }

	  if (image->byte_order == GDK_LSB_FIRST)
	    {
	      for (t = 0; t < image->bpp; t++)
		*mem++ = (unsigned char) ((pixel >> (t * 8)) & 0xFF);
	    }
	  else
	    {
	      for (t = 0; t < image->bpp; t++)
		*mem++ = (unsigned char) ((pixel >> ((image->bpp - t - 1) * 8)) & 0xFF);
	    }
	}

      if (mask && (cnt < s))
	gdk_draw_line (*mask, gc, cnt, r, s - 1, r);
    }

  if (mask)
    gdk_gc_destroy (gc);

  gc = gdk_gc_new (parent);
  gdk_draw_image (pixmap, gc, image, 0, 0, 0, 0, width, height);
  gdk_gc_destroy (gc);
  gdk_image_destroy (image);

  return pixmap;
}

GtkWidget*
create_pixmap_widget (GdkWindow *parent,
		      char **data, int width, int height)
{
  GdkPixmap *pixmap;
  GdkBitmap *mask;

  pixmap = create_pixmap (parent, &mask, data, width, height);

  return gtk_pixmap_new (pixmap, mask);
}

void
create_toolbox ()
{
  GtkWidget *window;
  GtkWidget *main_vbox;
  GtkWidget *vbox;
  GtkWidget *menubar;
  GList *device_list;
  GtkAccelGroup *table;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  /* Register dialog */
  dialog_register_toolbox(window);

  gtk_window_set_wmclass (GTK_WINDOW (window), "toolbox", "Gimp");
  gtk_window_set_title (GTK_WINDOW (window), _("The GIMP"));
  session_set_window_geometry (window, &toolbox_session_info, TRUE);
  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
		      GTK_SIGNAL_FUNC (toolbox_delete),
		      NULL);

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      (GtkSignalFunc) toolbox_destroy,
		      NULL);

  /* We need to know when the current device changes, so we can update
   * the correct tool - to do this we connect to motion events.
   * We can't just use EXTENSION_EVENTS_CURSOR though, since that
   * would get us extension events for the mouse pointer, and our
   * device would change to that and not change back. So we check
   * manually that all devices have a cursor, before establishing the check.
   */
  device_list = gdk_input_list_devices ();
  while (device_list)
    {
      if (!((GdkDeviceInfo *)(device_list->data))->has_cursor)
	break;

      device_list = device_list->next;
    }

  if (!device_list)		/* all devices have cursor */
    {
      gtk_signal_connect (GTK_OBJECT (window), "motion_notify_event",
			  GTK_SIGNAL_FUNC (toolbox_check_device), NULL);

      gtk_widget_set_events (window, GDK_POINTER_MOTION_MASK);
      gtk_widget_set_extension_events (window, GDK_EXTENSION_EVENTS_CURSOR);
    }

  main_vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 1);
  gtk_container_add (GTK_CONTAINER (window), main_vbox);
  gtk_widget_show (main_vbox);

  /*  allocate the colors for creating pixmaps  */
  allocate_colors (main_vbox);

  /*  tooltips  */
  tool_tips = gtk_tooltips_new ();
  gtk_tooltips_set_colors (tool_tips,
			   &colors[11],
			   &main_vbox->style->fg[GTK_STATE_NORMAL]);
  if (!show_tool_tips)
    gtk_tooltips_disable (tool_tips);

  /*  Build the menu bar with menus  */
  menus_get_toolbox_menubar (&menubar, &table);
  gtk_box_pack_start (GTK_BOX (main_vbox), menubar, FALSE, TRUE, 0);
  gtk_widget_show (menubar);

  /*  Install the accelerator table in the main window  */
  gtk_window_add_accel_group (GTK_WINDOW (window), table);

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (main_vbox), vbox, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 0);
  gtk_widget_show (vbox);

  create_tools (vbox);
  /*create_tool_label (vbox);*/
  /*create_progress_area (vbox);*/
  create_color_area (vbox);
  if (show_indicators && (!no_data) )
      create_indicator_area (vbox);
#ifdef icky
  create_qmask_area (vbox);
#endif /*icky */
  gtk_widget_show (window);
  gimp_set_drop_open (window);


  toolbox_shell = window;
}

void
toolbox_free ()
{
  int i;

  session_get_window_info (toolbox_shell, &toolbox_session_info);

  gtk_widget_destroy (toolbox_shell);
  for (i = 0; i < num_tools; i++)
    {
      if (!tool_info[i].icon_data)
	gtk_object_sink    (GTK_OBJECT (tool_info[i].tool_widget));
    }
  gtk_object_destroy (GTK_OBJECT (tool_tips));
  gtk_object_unref   (GTK_OBJECT (tool_tips));
}

void
toolbox_raise_callback (GtkWidget *widget,
			gpointer  client_data)
{
  gdk_window_raise(toolbox_shell->window);
}

void
create_display_shell (GDisplay* gdisp,
		      int   width,
		      int   height,
		      char *title,
		      int   type)
{
  static GtkWidget *image_popup_menu = NULL;
  static GtkAccelGroup *image_accel_group = NULL;
  
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *table_inner;
  GtkWidget *table_lower;
  GtkWidget *frame;
  GtkWidget *arrow;
  GtkWidget *pixmap;

  GSList *group = NULL;

  int n_width, n_height;
  int s_width, s_height;
  int scalesrc, scaledest;
  int contextid;

  {
    /*  adjust the initial scale -- so that window fits on screen */
    s_width = gdk_screen_width ();
    s_height = gdk_screen_height ();

    scalesrc = gdisp->scale & 0x00ff;
    scaledest = gdisp->scale >> 8;

    n_width = (width * scaledest) / scalesrc;
    n_height = (height * scaledest) / scalesrc;

    /*  Limit to the size of the screen...  */
    while (n_width > s_width || n_height > s_height)
      {
	if (scaledest > 1)
	  scaledest--;
	else
	  if (scalesrc < 0xff)
	    scalesrc++;

	n_width = (width * scaledest) / scalesrc;
	n_height = (height * scaledest) / scalesrc;
      }

    gdisp->scale = (scaledest << 8) + scalesrc;
  }

  /*  The adjustment datums  */
  gdisp->hsbdata = GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, width, 1, 1, width));
  gdisp->vsbdata = GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, height, 1, 1, height));

  /*  The toplevel shell */
  gdisp->shell = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_ref  (gdisp->shell);
  gtk_window_set_title (GTK_WINDOW (gdisp->shell), title);
  gtk_window_set_wmclass (GTK_WINDOW (gdisp->shell), "image_window", "Gimp");
  gtk_window_set_policy (GTK_WINDOW (gdisp->shell), TRUE, TRUE, TRUE);
  gtk_object_set_user_data (GTK_OBJECT (gdisp->shell), (gpointer) gdisp);
  gtk_widget_set_events (gdisp->shell, GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON_PRESS_MASK | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
  gtk_signal_connect (GTK_OBJECT (gdisp->shell), "delete_event",
		      GTK_SIGNAL_FUNC (gdisplay_delete),
		      gdisp);

  gtk_signal_connect (GTK_OBJECT (gdisp->shell), "destroy",
		      (GtkSignalFunc) gdisplay_destroy,
		      gdisp);

  gtk_signal_connect (GTK_OBJECT (gdisp->shell), "button_press_event",
		      (GtkSignalFunc) gdisplay_shell_events,
		      gdisp);

  gtk_signal_connect (GTK_OBJECT (gdisp->shell), "key_press_event",
		      (GtkSignalFunc) gdisplay_shell_events,
		      gdisp);

  /*  the vbox, table containing all widgets  */
  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (gdisp->shell), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);

  /*  the table widget is pretty stupid so we need 2 tables
      or it treats rulers and canvas with equal weight when
      allocating space, ugh. */
  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 1);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 1);
  gtk_box_pack_start(GTK_BOX (vbox), table, TRUE, TRUE, 0);

  table_inner = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table_inner), 0, 1);
  gtk_table_set_row_spacing (GTK_TABLE (table_inner), 0, 1);

  table_lower = gtk_table_new (1,3,FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table_lower), 0, 1);
 /*  gtk_table_set_row_spacing (GTK_TABLE (table_lower), 0, 1); */

  /* hbox for statusbar area */
  gdisp->statusarea = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), gdisp->statusarea, FALSE, TRUE, 0);

  /*  scrollbars, rulers, canvas, menu popup button  */
  gdisp->origin = gtk_button_new ();
  GTK_WIDGET_UNSET_FLAGS (gdisp->origin, GTK_CAN_FOCUS);
  gtk_widget_set_events (GTK_WIDGET (gdisp->origin),
			 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  gtk_signal_connect (GTK_OBJECT (gdisp->origin), "button_press_event",
		      (GtkSignalFunc) gdisplay_origin_button_press,
		      gdisp);

  arrow = gtk_arrow_new (GTK_ARROW_RIGHT, GTK_SHADOW_OUT);
  gtk_container_set_border_width (GTK_CONTAINER (gdisp->origin), 0);
  gtk_container_add (GTK_CONTAINER (gdisp->origin), arrow);

  gdisp->hrule = gtk_hruler_new ();
  gtk_widget_set_events (GTK_WIDGET (gdisp->hrule),
			 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  gtk_signal_connect_object (GTK_OBJECT (gdisp->shell), "motion_notify_event",
			     (GtkSignalFunc) GTK_WIDGET_CLASS (GTK_OBJECT (gdisp->hrule)->klass)->motion_notify_event,
			     GTK_OBJECT (gdisp->hrule));
  gtk_signal_connect (GTK_OBJECT (gdisp->hrule), "button_press_event",
		      (GtkSignalFunc) gdisplay_hruler_button_press,
		      gdisp);

  gdisp->vrule = gtk_vruler_new ();
  gtk_widget_set_events (GTK_WIDGET (gdisp->vrule),
			 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  gtk_signal_connect_object (GTK_OBJECT (gdisp->shell), "motion_notify_event",
			     (GtkSignalFunc) GTK_WIDGET_CLASS (GTK_OBJECT (gdisp->vrule)->klass)->motion_notify_event,
			     GTK_OBJECT (gdisp->vrule));
  gtk_signal_connect (GTK_OBJECT (gdisp->vrule), "button_press_event",
		      (GtkSignalFunc) gdisplay_vruler_button_press,
		      gdisp);

  gdisp->hsb = gtk_hscrollbar_new (gdisp->hsbdata);
  GTK_WIDGET_UNSET_FLAGS (gdisp->hsb, GTK_CAN_FOCUS);
  gdisp->vsb = gtk_vscrollbar_new (gdisp->vsbdata);
  GTK_WIDGET_UNSET_FLAGS (gdisp->vsb, GTK_CAN_FOCUS);


  gdisp->qmaskoff = gtk_radio_button_new(group);
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (gdisp->qmaskoff));
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (gdisp->qmaskoff), FALSE);
  gtk_signal_connect (GTK_OBJECT (gdisp->qmaskoff), "toggled",
                     (GtkSignalFunc) qmask_deactivate,
                     gdisp);

  gdisp->qmaskon = gtk_radio_button_new(group);
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (gdisp->qmaskon));
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (gdisp->qmaskon), FALSE);
  gtk_signal_connect (GTK_OBJECT (gdisp->qmaskon), "toggled",
                     (GtkSignalFunc) qmask_activate,
                     gdisp);


  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gdisp->qmaskoff), TRUE);
  gtk_widget_set_usize(GTK_WIDGET(gdisp->qmaskon), 15, 15);
  gtk_widget_set_usize(GTK_WIDGET(gdisp->qmaskoff), 15, 15);
  /* Draw pixmaps - note: you must realize the parent prior to doing the
     rest! */  
    {
    GdkPixmap *pxmp;
    GdkBitmap *mask;
    GtkStyle *style;

    style = gtk_widget_get_style(gdisp->shell);
    gtk_widget_realize(gdisp->shell);
   
    pxmp = gdk_pixmap_create_from_xpm_d(gdisp->shell->window, &mask,
                                        &style->bg[GTK_STATE_NORMAL],
                                        qmasksel_xpm);   

    pixmap = gtk_pixmap_new (pxmp, mask);
    gtk_container_add (GTK_CONTAINER (gdisp->qmaskon), pixmap);
    gtk_widget_show(pixmap);
  
    pxmp = gdk_pixmap_create_from_xpm_d(gdisp->shell->window, &mask,
                                        &style->bg[GTK_STATE_NORMAL],
                                        qmasknosel_xpm);   
    pixmap = gtk_pixmap_new (pxmp, mask);
    gtk_container_add (GTK_CONTAINER (gdisp->qmaskoff), pixmap);
    gtk_widget_show(pixmap);
    }
 
  gdisp->canvas = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (gdisp->canvas), n_width, n_height);
  gtk_widget_set_events (gdisp->canvas, CANVAS_EVENT_MASK);
  gtk_widget_set_extension_events (gdisp->canvas, GDK_EXTENSION_EVENTS_ALL);
  GTK_WIDGET_SET_FLAGS (gdisp->canvas, GTK_CAN_FOCUS);
  gtk_object_set_user_data (GTK_OBJECT (gdisp->canvas), (gpointer) gdisp);

  /* set the active display before doing any other canvas event processing  */
  gtk_signal_connect (GTK_OBJECT (gdisp->canvas), "event",
		      (GtkSignalFunc) gdisplay_shell_events,
		      gdisp);

  gtk_signal_connect (GTK_OBJECT (gdisp->canvas), "event",
		      (GtkSignalFunc) gdisplay_canvas_events,
		      gdisp);

  /*  pack all the widgets  */

  gtk_table_attach (GTK_TABLE (table), table_inner, 0, 1, 0, 1,
		    GTK_FILL | GTK_EXPAND | GTK_SHRINK,
		    GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 0);
 /* sneak in an extra table here */
  gtk_table_attach (GTK_TABLE (table_lower), gdisp->hsb, 2, 3, 0, 1,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table_lower), gdisp->qmaskoff, 0, 1, 0, 1,
		    GTK_SHRINK, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

  gtk_table_attach (GTK_TABLE (table_lower), gdisp->qmaskon, 1, 2, 0, 1,
		    GTK_SHRINK, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

  gtk_table_attach (GTK_TABLE (table), table_lower, 0, 1, 1, 2,
		    GTK_FILL | GTK_SHRINK | GTK_FILL, GTK_FILL, 0, 0);

  gtk_table_attach (GTK_TABLE (table), gdisp->vsb, 1, 2, 0, 1,
		    GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

  gtk_table_attach (GTK_TABLE (table_inner), gdisp->origin, 0, 1, 0, 1,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table_inner), gdisp->hrule, 1, 2, 0, 1,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table_inner), gdisp->vrule, 0, 1, 1, 2,
		    GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table_inner), gdisp->canvas, 1, 2, 1, 2,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

  if (! image_popup_menu)
    menus_get_image_menu (&image_popup_menu, &image_accel_group);

  gtk_container_set_resize_mode (GTK_CONTAINER (gdisp->statusarea), 
				 GTK_RESIZE_QUEUE);

  /* cursor, statusbar, progressbar  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (gdisp->statusarea), frame, FALSE, TRUE, 0);

  gdisp->cursor_label = gtk_label_new (" ");
  gtk_container_add (GTK_CONTAINER (frame), gdisp->cursor_label);

  /* we need to realize the cursor_label widget here, so the size gets
     computed correctly */
  gtk_widget_realize (gdisp->cursor_label);
  gdisplay_resize_cursor_label (gdisp);

  gdisp->statusbar = gtk_statusbar_new ();
  gtk_widget_set_usize (gdisp->statusbar, 1, -1);
  gtk_container_set_resize_mode (GTK_CONTAINER (gdisp->statusbar),
				 GTK_RESIZE_QUEUE);
  gtk_box_pack_start (GTK_BOX (gdisp->statusarea), gdisp->statusbar, TRUE, TRUE, 0);
  contextid = gtk_statusbar_get_context_id (GTK_STATUSBAR (gdisp->statusbar),
					    "title");
  gtk_statusbar_push (GTK_STATUSBAR (gdisp->statusbar),
		      contextid,
		      title);

  gdisp->progressbar = gtk_progress_bar_new();
  gtk_widget_set_usize (gdisp->progressbar, 80, -1);
  gtk_box_pack_start (GTK_BOX (gdisp->statusarea), gdisp->progressbar, FALSE, TRUE, 0);

  gdisp->cancelbutton = gtk_button_new_with_label(_("Cancel"));
  gtk_box_pack_start (GTK_BOX (gdisp->statusarea), gdisp->cancelbutton, FALSE, TRUE, 0);
  gtk_widget_set_sensitive (gdisp->cancelbutton, FALSE);

 /*  the popup menu  */
  gdisp->popup = image_popup_menu;

  /*  the accelerator table for images  */
  gtk_window_add_accel_group (GTK_WINDOW (gdisp->shell), image_accel_group);

  gtk_widget_show (arrow);
  gtk_widget_show (gdisp->qmaskon);
  gtk_widget_show (gdisp->qmaskoff);


  gtk_widget_show (gdisp->hsb);
  gtk_widget_show (gdisp->vsb);

  if (show_rulers)
    {
      gtk_widget_show (gdisp->origin);
      gtk_widget_show (gdisp->hrule);
      gtk_widget_show (gdisp->vrule);
    }

  gtk_widget_show (gdisp->canvas);
  gtk_widget_show (frame);
  gtk_widget_show (gdisp->cursor_label);
  gtk_widget_show (gdisp->statusbar);
  gtk_widget_show (gdisp->progressbar);
  gtk_widget_show (gdisp->cancelbutton);
  
  gtk_widget_show (table_lower);
  gtk_widget_show (table_inner);
  gtk_widget_show (table);
  if (show_statusbar)
    {
      gtk_widget_show (gdisp->statusarea);
    }
  gtk_widget_show (vbox);

  gtk_widget_show (gdisp->shell);
#ifdef __GNUC__
#warning DODGY?
#endif /*__GNUC__ */
  gtk_widget_realize (gdisp->canvas);
  gdk_window_set_back_pixmap(gdisp->canvas->window, NULL, 0);

  /*  set the focus to the canvas area  */
  gtk_widget_grab_focus (gdisp->canvas);

}


/*
 *  String, integer, double and size query boxes
 */

typedef struct _QueryBox QueryBox;

struct _QueryBox
{
  GtkWidget *qbox;
  GtkWidget *vbox;
  GtkWidget *entry;
  GtkWidget *ok_button;
  GtkObject *object;
  QueryFunc  callback;
  gpointer   data;
};

static QueryBox * create_query_box (gchar *, gchar *, GtkObject *, gchar *,
				    QueryFunc, gpointer);
static gint query_box_delete_callback (GtkWidget *, GdkEvent *, gpointer);
static void query_box_cancel_callback (GtkWidget *, gpointer);
static void string_query_box_ok_callback (GtkWidget *, gpointer);
static void int_query_box_ok_callback (GtkWidget *, gpointer);
static void double_query_box_ok_callback (GtkWidget *, gpointer);
static void size_query_box_ok_callback (GtkWidget *, gpointer);

/*  create a generic query box without any entry widget  */
static QueryBox *
create_query_box (gchar       *title,
		  gchar       *message,
		  GtkObject   *object,
		  gchar       *signal,
		  QueryFunc    callback,
		  gpointer     data)
{
  QueryBox  *query_box;
  GtkWidget *qbox;
  GtkWidget *vbox;
  GtkWidget *label;

  static ActionAreaItem action_items[] =
  {
    { N_("OK"), (ActionCallback) NULL, NULL, NULL },
    { N_("Cancel"), (ActionCallback) query_box_cancel_callback, NULL, NULL }
  };

  query_box = (QueryBox *) g_malloc (sizeof (QueryBox));

  qbox = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (qbox), gettext(title));
  gtk_window_set_wmclass (GTK_WINDOW (qbox), "query_box", "Gimp");
  gtk_window_position (GTK_WINDOW (qbox), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (qbox), "delete_event",
		      (GtkSignalFunc) query_box_delete_callback,
		      query_box);

  /*  if we are associated with an object, connect to the provided signal  */
  if (object && GTK_IS_OBJECT (object) && signal)
    gtk_signal_connect (GTK_OBJECT (object), signal,
			(GtkSignalFunc) query_box_cancel_callback,
			query_box);
  else
    object = NULL;

  /*  The action area  */
  action_items[1].user_data = query_box;
  build_action_area (GTK_DIALOG (qbox), action_items, 2, 0);

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (qbox)->vbox), vbox);
  gtk_widget_show (vbox);

  label = gtk_label_new (gettext (message));
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, FALSE, 0);
  gtk_widget_show (label);

  query_box->qbox = qbox;
  query_box->vbox = vbox;
  query_box->entry = NULL;
  query_box->ok_button = action_items[0].widget;
  query_box->object = object;
  query_box->callback = callback;
  query_box->data = data;

  return query_box;
}

GtkWidget *
query_string_box (gchar       *title,
		  gchar       *message,
		  gchar       *initial,
		  GtkObject   *object,
		  gchar       *signal,
		  QueryFunc    callback,
		  gpointer     data)
{
  QueryBox  *query_box;
  GtkWidget *entry;

  query_box = create_query_box (title, message, object, signal, callback, data);

  gtk_signal_connect (GTK_OBJECT (query_box->ok_button), "clicked",
                      (GtkSignalFunc) string_query_box_ok_callback,
                      query_box);

  entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (query_box->vbox), entry, TRUE, TRUE, 0);
  if (initial)
    gtk_entry_set_text (GTK_ENTRY (entry), gettext (initial));
  gtk_widget_grab_focus (entry);
  gtk_widget_show (entry);

  query_box->entry = entry;

  gtk_widget_show (query_box->qbox);

  return query_box->qbox;
}

GtkWidget *
query_int_box (gchar       *title,
	       gchar       *message,
	       gint         initial,
	       gint         lower,
	       gint         upper,
	       GtkObject   *object,
	       gchar       *signal,
	       QueryFunc    callback,
	       gpointer     data)
{
  QueryBox  *query_box;
  GtkAdjustment* adjustment;
  GtkWidget *spinbutton;

  query_box = create_query_box (title, message, object, signal, callback, data);

  gtk_signal_connect (GTK_OBJECT (query_box->ok_button), "clicked",
                      (GtkSignalFunc) int_query_box_ok_callback,
                      query_box);
  adjustment =
    GTK_ADJUSTMENT (gtk_adjustment_new (initial, lower, upper, 1, 10, 0));
  spinbutton = gtk_spin_button_new (adjustment, 1.0, 0);
  gtk_box_pack_start (GTK_BOX (query_box->vbox), spinbutton, TRUE, TRUE, 0);
  gtk_widget_grab_focus (spinbutton);
  gtk_widget_show (spinbutton);

  query_box->entry = spinbutton;

  gtk_widget_show (query_box->qbox);

  return query_box->qbox;
}

GtkWidget *
query_double_box (gchar     *title,
		  gchar     *message,
		  gdouble    initial,
		  gdouble    lower,
		  gdouble    upper,
		  gint       digits,
		  GtkObject *object,
		  gchar     *signal,
		  QueryFunc  callback,
		  gpointer   data)
{
  QueryBox  *query_box;
  GtkAdjustment* adjustment;
  GtkWidget *spinbutton;

  query_box = create_query_box (title, message, object, signal, callback, data);

  gtk_signal_connect (GTK_OBJECT (query_box->ok_button), "clicked",
                      (GtkSignalFunc) double_query_box_ok_callback,
                      query_box);
  adjustment =
    GTK_ADJUSTMENT (gtk_adjustment_new (initial, lower, upper, 1, 10, 0));
  spinbutton = gtk_spin_button_new (adjustment, 1.0, digits);
  gtk_box_pack_start (GTK_BOX (query_box->vbox), spinbutton, TRUE, TRUE, 0);
  gtk_widget_grab_focus (spinbutton);
  gtk_widget_show (spinbutton);

  query_box->entry = spinbutton;

  gtk_widget_show (query_box->qbox);

  return query_box->qbox;
}

GtkWidget *
query_size_box (gchar     *title,
		gchar     *message,
		gdouble    initial,
		gdouble    lower,
		gdouble    upper,
		gint       digits,
		GUnit      unit,
		gdouble    resolution,
		gint       dot_for_dot,
		GtkObject *object,
		gchar     *signal,
		QueryFunc  callback,
		gpointer   data)
{
  QueryBox  *query_box;
  GtkWidget *sizeentry;

  query_box = create_query_box (title, message, object, signal, callback, data);

  gtk_signal_connect (GTK_OBJECT (query_box->ok_button), "clicked",
                      (GtkSignalFunc) size_query_box_ok_callback,
                      query_box);

  sizeentry = gimp_size_entry_new (1, unit, "%p", TRUE, FALSE, FALSE, 100,
				   GIMP_SIZE_ENTRY_UPDATE_SIZE);
  if (dot_for_dot)
    gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (sizeentry), UNIT_PIXEL);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (sizeentry), 0,
				  resolution, FALSE);
  gimp_size_entry_set_refval_digits (GIMP_SIZE_ENTRY (sizeentry), 0, digits);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (sizeentry), 0,
					 lower, upper);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (sizeentry), 0, initial);

  gtk_box_pack_start (GTK_BOX (query_box->vbox), sizeentry, TRUE, TRUE, 0);
  gimp_size_entry_grab_focus (GIMP_SIZE_ENTRY (sizeentry));
  gtk_widget_show (sizeentry);

  query_box->entry = sizeentry;

  gtk_widget_show (query_box->qbox);

  return query_box->qbox;
}

static gint
query_box_delete_callback (GtkWidget *w,
			   GdkEvent  *e,
			   gpointer   client_data)
{
  query_box_cancel_callback (w, client_data);

  return TRUE;
}

static void
query_box_cancel_callback (GtkWidget *w,
			   gpointer   client_data)
{
  QueryBox *query_box;

  query_box = (QueryBox *) client_data;

  /*  disconnect, if we are connected to some signal  */
  if (query_box->object)
    gtk_signal_disconnect_by_data (query_box->object, query_box);

  /*  Destroy the box  */
  gtk_widget_destroy (query_box->qbox);

  g_free (query_box);
}

static void
string_query_box_ok_callback (GtkWidget *w,
			      gpointer   client_data)
{
  QueryBox *query_box;
  gchar    *string;

  query_box = (QueryBox *) client_data;

  gtk_widget_set_sensitive (query_box->qbox, FALSE);

  /*  disconnect, if we are connected to some signal  */
  if (query_box->object)
    gtk_signal_disconnect_by_data (query_box->object, query_box);

  /*  Get the entry data  */
  string = g_strdup (gtk_entry_get_text (GTK_ENTRY (query_box->entry)));

  /*  Call the user defined callback  */
  (* query_box->callback) (w, query_box->data, (gpointer) string);

  /*  Destroy the box  */
  gtk_widget_destroy (query_box->qbox);

  g_free (query_box);
}

static void
int_query_box_ok_callback (GtkWidget *w,
			   gpointer   client_data)
{
  QueryBox *query_box;
  gint     *integer_value;

  query_box = (QueryBox *) client_data;

  gtk_widget_set_sensitive (query_box->qbox, FALSE);

  /*  disconnect, if we are connected to some signal  */
  if (query_box->object)
    gtk_signal_disconnect_by_data (query_box->object, query_box);

  /*  Get the spinbutton data  */
  integer_value = g_malloc (sizeof (gint));
  *integer_value =
    gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (query_box->entry));

  /*  Call the user defined callback  */
  (* query_box->callback) (w, query_box->data, (gpointer) integer_value);

  /*  Destroy the box  */
  gtk_widget_destroy (query_box->qbox);

  g_free (query_box);
}

static void
double_query_box_ok_callback (GtkWidget *w,
			      gpointer   client_data)
{
  QueryBox *query_box;
  gdouble  *double_value;

  query_box = (QueryBox *) client_data;

  gtk_widget_set_sensitive (query_box->qbox, FALSE);

  /*  disconnect, if we are connected to some signal  */
  if (query_box->object)
    gtk_signal_disconnect_by_data (query_box->object, query_box);

  /*  Get the spinbutton data  */
  double_value = g_malloc (sizeof (gdouble));
  *double_value =
    gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (query_box->entry));

  /*  Call the user defined callback  */
  (* query_box->callback) (w, query_box->data, (gpointer) double_value);

  /*  Destroy the box  */
  gtk_widget_destroy (query_box->qbox);

  g_free (query_box);
}

static void
size_query_box_ok_callback (GtkWidget *w,
			    gpointer   client_data)
{
  QueryBox *query_box;
  gdouble  *double_value;

  query_box = (QueryBox *) client_data;

  gtk_widget_set_sensitive (query_box->qbox, FALSE);

  /*  disconnect, if we are connected to some signal  */
  if (query_box->object)
    gtk_signal_disconnect_by_data (query_box->object, query_box);

  /*  Get the sizeentry data  */
  double_value = g_malloc (sizeof (gdouble));
  *double_value =
    gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (query_box->entry), 0);

  /*  Pass the selected unit to the callback  */
  gtk_object_set_data
    (GTK_OBJECT (w), "size_query_unit",
     (gpointer) gimp_size_entry_get_unit (GIMP_SIZE_ENTRY (query_box->entry)));

  /*  Call the user defined callback  */
  (* query_box->callback) (w, query_box->data, (gpointer) double_value);

  /*  Destroy the box  */
  gtk_widget_destroy (query_box->qbox);

  g_free (query_box);
}


/*
 *  Message Boxes...
 */

typedef struct _MessageBox MessageBox;

struct _MessageBox
{
  GtkWidget  *mbox;
  GtkCallback callback;
  gpointer    data;
};

static void message_box_close_callback (GtkWidget *, gpointer);
static gint message_box_delete_callback (GtkWidget *, GdkEvent *, gpointer);

GtkWidget *
message_box (char        *message,
	     GtkCallback  callback,
	     gpointer     data)
{
  MessageBox *msg_box;
  GtkWidget  *mbox;
  GtkWidget  *vbox;
  GtkWidget  *label_vbox;
  GtkWidget  *label;
  GtkWidget  *button;
  char *str, *orig;

  if (message)
    message = orig = g_strdup (message);
  else
    return NULL;

  msg_box = (MessageBox *) g_malloc (sizeof (MessageBox));

  mbox = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (mbox), "gimp_message", "Gimp");
  gtk_window_set_title (GTK_WINDOW (mbox), _("GIMP Message"));
  gtk_window_position (GTK_WINDOW (mbox), GTK_WIN_POS_MOUSE);

  gtk_signal_connect (GTK_OBJECT (mbox), "delete_event",
		      GTK_SIGNAL_FUNC (message_box_delete_callback),
		      msg_box);

  button = gtk_button_new_with_label (_("OK"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) message_box_close_callback,
                      msg_box);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (mbox)->action_area), button,
		      TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (mbox)->vbox), vbox);
  gtk_widget_show (vbox);

  label_vbox = gtk_vbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), label_vbox, TRUE, FALSE, 0);
  gtk_widget_show (label_vbox);

  str = message;
  while (*str != '\0')
    {
      if (*str == '\n')
	{
	  *str = '\0';
	  label = gtk_label_new (message);
	  gtk_box_pack_start (GTK_BOX (label_vbox), label, TRUE, FALSE, 0);
	  gtk_widget_show (label);
	  message = str + 1;
	}
      str++;
    }

  if (*message != '\0')
    {
      label = gtk_label_new (message);
      gtk_box_pack_start (GTK_BOX (label_vbox), label, TRUE, FALSE, 0);
      gtk_widget_show (label);
    }

  g_free (orig);

  msg_box->mbox = mbox;
  msg_box->callback = callback;
  msg_box->data = data;

  gtk_widget_show (mbox);

  return mbox;
}

static gint
message_box_delete_callback (GtkWidget *w, GdkEvent *e, gpointer client_data)
{
  message_box_close_callback (w, client_data);

  return TRUE;
}

static void
message_box_close_callback (GtkWidget *w,
			    gpointer   client_data)
{
  MessageBox *msg_box;

  msg_box = (MessageBox *) client_data;

  /*  If there is a valid callback, invoke it  */
  if (msg_box->callback)
    (* msg_box->callback) (w, msg_box->data);

  /*  Destroy the box  */
  gtk_widget_destroy (msg_box->mbox);

  g_free (msg_box);
}

/* DnD functions */ 
static void
gimp_set_drop_open (GtkWidget *object)
{
  gtk_drag_dest_set (object,
		     GTK_DEST_DEFAULT_ALL,
		     dnd_target_table, dnd_n_targets,
		     GDK_ACTION_COPY);
  gtk_signal_connect (GTK_OBJECT (object),
		      "drag_data_received",
		      GTK_SIGNAL_FUNC (gimp_dnd_data_received),
		      object);
}

static void
gimp_dnd_data_received (GtkWidget          *widget,
			GdkDragContext     *context,
			gint                x,
			gint                y,
			GtkSelectionData   *data,
			guint               info,
			guint               time)
{
  switch (context->action)
    {
    case GDK_ACTION_DEFAULT:
    case GDK_ACTION_COPY:
    case GDK_ACTION_MOVE:
    case GDK_ACTION_LINK:
    case GDK_ACTION_ASK:
    default:
      gimp_dnd_open_files ((gchar *) data->data);
      gtk_drag_finish (context, TRUE, FALSE, time);
      break;
    }
  return;
}

static gint
gimp_dnd_open_files (gchar *buffer)
{
  gchar	 name_buffer[1024];
  const gchar *data_type = "file:";
  const gint sig_len = strlen (data_type);

  while (*buffer)
    {
      gchar *name = name_buffer;
      gint len = 0;

      while ((*buffer != 0) && (*buffer != '\n'))
	{
	  *name++ = *buffer++;
	  len++;
	}
      if (len == 0)
	break;
      if (*(name - 1) == 0xd)
	*(name - 1) = 0;
      name = name_buffer;
      if ((sig_len < len) && (! strncmp (name, data_type, sig_len)))
	name += sig_len;

      file_open (name, name);

      if (*buffer)
	buffer++;
    }
  return TRUE;
}
