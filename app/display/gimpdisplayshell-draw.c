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

#include <string.h>
#include "appenv.h"
#include "app_procs.h"
#include "color_area.h"
#include "commands.h"
#include "devices.h"
#include "dialog_handler.h"
#include "disp_callbacks.h"
#include "fileops.h"
#include "gdisplay.h"
#include "gdisplay_ops.h"
#include "gimage.h"
#include "gimpdnd.h"
#include "gimphelp.h"
#include "gimprc.h"
#include "gimpui.h"
#include "gtkhwrapbox.h"
#include "gtkvwrapbox.h"
#include "indicator_area.h"
#include "interface.h"
#include "menus.h"
#include "nav_window.h"
#include "qmask.h"
#include "session.h"
#include "tools.h"

#include "pixmaps.h"
#include "pixmaps/qmasksel.xpm"
#include "pixmaps/qmasknosel.xpm"
#include "pixmaps/navbutton.xpm"

#include "libgimp/gimpintl.h"


/*  local functions  */
static void  tools_select_update   (GtkWidget      *widget,
				    gpointer        data);
static gint  tools_button_press    (GtkWidget      *widget,
				    GdkEventButton *bevent,
				    gpointer        data);
static void  gdisplay_destroy      (GtkWidget      *widget,
				    GDisplay       *display);

static gint  gdisplay_delete       (GtkWidget      *widget,
				    GdkEvent       *event,
				    GDisplay       *display);

static void  toolbox_destroy       (void);
static gint  toolbox_delete        (GtkWidget      *widget,
				    GdkEvent       *event,
				    gpointer        data);
static gint  toolbox_check_device  (GtkWidget      *widget,
				    GdkEvent       *event,
				    gpointer        data);

static GdkPixmap *create_pixmap    (GdkWindow      *parent,
				    GdkBitmap     **mask,
				    gchar         **data,
				    gint            width,
				    gint            height);

static void     toolbox_set_drag_dest      (GtkWidget        *widget);
static void     toolbox_drag_data_received (GtkWidget        *widget,
					    GdkDragContext   *context,
					    gint              x,
					    gint              y,
					    GtkSelectionData *data,
					    guint             info,
					    guint             time);
static gboolean toolbox_drag_drop          (GtkWidget        *widget,
					    GdkDragContext   *context,
					    gint              x,
					    gint              y,
					    guint             time);
static ToolType toolbox_drag_tool          (GtkWidget        *widget,
					    gpointer          data);
static void     toolbox_drop_tool          (GtkWidget        *widget,
					    ToolType          tool,
					    gpointer          data);
static void     gimp_dnd_open_files        (gchar            *buffer);

static gint pixmap_colors[8][3] =
{
  { 0x00, 0x00, 0x00 }, /* a -   0 */
  { 0x24, 0x24, 0x24 }, /* b -  36 */
  { 0x49, 0x49, 0x49 }, /* c -  73 */
  { 0x6D, 0x6D, 0x6D }, /* d - 109 */
  { 0x92, 0x92, 0x92 }, /* e - 146 */
  { 0xB6, 0xB6, 0xB6 }, /* f - 182 */
  { 0xDB, 0xDB, 0xDB }, /* g - 219 */
  { 0xFF, 0xFF, 0xFF }, /* h - 255 */
};

#define COLUMNS 3
#define ROWS    8
#define MARGIN  2

/*  local variables  */
static GdkColor    colors[11];
static GtkWidget * toolbox_shell = NULL;

static GtkTargetEntry toolbox_target_table[] =
{
  GIMP_TARGET_URI_LIST,
  GIMP_TARGET_TEXT_PLAIN,
  GIMP_TARGET_NETSCAPE_URL,
  GIMP_TARGET_LAYER,
  GIMP_TARGET_CHANNEL,
  GIMP_TARGET_LAYER_MASK,
  GIMP_TARGET_TOOL
};
static guint toolbox_n_targets = (sizeof (toolbox_target_table) /
				  sizeof (toolbox_target_table[0]));

static GtkTargetEntry tool_target_table[] =
{
  GIMP_TARGET_TOOL
};
static guint tool_n_targets = (sizeof (tool_target_table) /
			       sizeof (tool_target_table[0]));

static GtkTargetEntry display_target_table[] =
{
  GIMP_TARGET_LAYER,
  GIMP_TARGET_CHANNEL,
  GIMP_TARGET_LAYER_MASK,
  GIMP_TARGET_COLOR,
  GIMP_TARGET_PATTERN
};
static guint display_n_targets = (sizeof (display_target_table) /
				  sizeof (display_target_table[0]));

static void
tools_select_update (GtkWidget *widget,
		     gpointer   data)
{
  ToolType tool_type;

  tool_type = (ToolType) data;

  if ((tool_type != -1) && GTK_TOGGLE_BUTTON (widget)->active)
    gimp_context_set_tool (gimp_context_get_user (), tool_type);
}

static gint
tools_button_press (GtkWidget      *widget,
		    GdkEventButton *event,
		    gpointer        data)
{
  GDisplay * gdisp;
  gdisp = data;
 
  if ((event->type == GDK_2BUTTON_PRESS) && (event->button == 1))
    tool_options_dialog_show ();

  return FALSE;
}

static gint
toolbox_delete (GtkWidget *widget,
		GdkEvent  *event,
		gpointer   data)
{
  app_exit (FALSE);

  return TRUE;
}

static void
toolbox_destroy (void)
{
  app_exit_finish ();
}

static gint
toolbox_check_device (GtkWidget *widget,
		      GdkEvent  *event,
		      gpointer   data)
{
  devices_check_change (event);

  return FALSE;
}

static void
gdisplay_destroy (GtkWidget *widget,
		  GDisplay  *gdisp)
{
  gdisplay_remove_and_delete (gdisp);
}

static gint
gdisplay_delete (GtkWidget *widget,
		 GdkEvent  *event,
		 GDisplay  *gdisp)
{
  gdisplay_close_window (gdisp, FALSE);

  return TRUE;
}

static void
allocate_colors (GtkWidget *parent)
{
  GdkColormap *colormap;
  gint i;

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
}

static void
create_indicator_area (GtkWidget *parent)
{
  GtkWidget *frame;
  GtkWidget *alignment;
  GtkWidget *ind_area;

  if (! GTK_WIDGET_REALIZED (parent))
    gtk_widget_realize (parent);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_wrap_box_pack (GTK_WRAP_BOX (parent), frame, TRUE, TRUE, TRUE, TRUE);

  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_set_border_width (GTK_CONTAINER (alignment), 3);
  gtk_container_add (GTK_CONTAINER (frame), alignment);

  gimp_help_set_help_data (alignment, NULL, "#indicator_area");

  ind_area = indicator_area_create ();
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
  GdkBitmap *default_mask;
  GdkPixmap *swap_pixmap;
  GdkBitmap *swap_mask;

  if (! GTK_WIDGET_REALIZED (parent))
    gtk_widget_realize (parent);

  default_pixmap = create_pixmap (parent->window, &default_mask, default_bits,
				  default_width, default_height);
  swap_pixmap    = create_pixmap (parent->window, &swap_mask, swap_bits,
				  swap_width, swap_height);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_wrap_box_pack (GTK_WRAP_BOX (parent), frame, TRUE, TRUE, TRUE, TRUE);
  gtk_wrap_box_set_child_forced_break (GTK_WRAP_BOX (parent), frame, TRUE);

  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_set_border_width (GTK_CONTAINER (alignment), 3);
  gtk_container_add (GTK_CONTAINER (frame), alignment);

  gimp_help_set_help_data (alignment, NULL, "#color_area");

  col_area = color_area_create (54, 42,
				default_pixmap, default_mask,
				swap_pixmap, swap_mask);
  gtk_container_add (GTK_CONTAINER (alignment), col_area);
  gimp_help_set_help_data
    (col_area,
     _("Foreground & background colors.  The black "
       "and white squares reset colors.  The arrows swap colors. Double "
       "click to select a color from a colorrequester."), NULL);

  gtk_widget_show (col_area);
  gtk_widget_show (alignment);
  gtk_widget_show (frame);
}

/* creates all icons */
static void
create_tool_pixmaps (GtkWidget *parent)
{
  gint i;

  g_return_if_fail (parent != NULL);

  for (i = 0; i < num_tools; i++)
    {
      if (tool_info[i].icon_data)
	tool_info[i].icon_pixmap = create_pixmap (parent->window, 
						  &tool_info[i].icon_mask,
						  tool_info[i].icon_data,
						  22, 22);
      else
	tool_info[i].icon_pixmap = create_pixmap (parent->window,  
						  &tool_info[i].icon_mask,
						  dialog_bits,
						  22, 22);
    }
}

static void
create_tools (GtkWidget *parent)
{
  GtkWidget *wbox;
  GtkWidget *button;
  GtkWidget *alignment;
  GtkWidget *pixmap;
  GSList *group;
  gint    i, j;

  wbox = parent;

  if (! GTK_WIDGET_REALIZED (gtk_widget_get_toplevel (wbox)))
    gtk_widget_realize (gtk_widget_get_toplevel (wbox));

  create_tool_pixmaps (wbox);

  group = NULL;

  i = 0;
  for (j = 0; j < num_tools; j++)
    {
      if (j <= LAST_TOOLBOX_TOOL &&
	  j != SCALE && j!= SHEAR && j != PERSPECTIVE)
	{
	  tool_info[j].tool_widget = button = gtk_radio_button_new (group);
	  gtk_container_set_border_width (GTK_CONTAINER (button), 0);
	  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));

	  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);

	  gtk_wrap_box_pack (GTK_WRAP_BOX (wbox), button,
			     FALSE, FALSE, FALSE, FALSE);

	  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
	  gtk_container_set_border_width (GTK_CONTAINER (alignment), 0);
	  gtk_container_add (GTK_CONTAINER (button), alignment);

	  pixmap = gtk_pixmap_new (tool_get_pixmap ((ToolType)j), 
				   tool_get_mask ((ToolType)j));
	  gtk_container_add (GTK_CONTAINER (alignment), pixmap);

	  gtk_signal_connect (GTK_OBJECT (button), "toggled",
			      GTK_SIGNAL_FUNC (tools_select_update),
			      (gpointer) tool_info[j].tool_id);

	  gtk_signal_connect (GTK_OBJECT (button), "button_press_event",
			      GTK_SIGNAL_FUNC (tools_button_press),
			      (gpointer) tool_info[j].tool_id);

	  /*  dnd stuff  */
	  gtk_drag_source_set (tool_info[j].tool_widget,
			       GDK_BUTTON2_MASK,
			       tool_target_table, tool_n_targets,
			       GDK_ACTION_COPY);
	  gimp_dnd_tool_source_set (tool_info[j].tool_widget,
				    toolbox_drag_tool, (gpointer) j);

	  gimp_help_set_help_data (button,
				   gettext(tool_info[j].tool_desc),
				   tool_info[j].private_tip);

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
			      GTK_SIGNAL_FUNC (tools_select_update),
			      (gpointer) tool_info[j].tool_id);
	}
    }

  gtk_widget_show (wbox);
}

static GdkPixmap *
create_pixmap (GdkWindow  *parent,
	       GdkBitmap **mask,
	       gchar     **data,
	       gint        width,
	       gint        height)
{
  GdkPixmap   *pixmap;
  GdkImage    *image;
  GdkGC       *gc;
  GdkVisual   *visual;
  GdkColormap *cmap;
  gint     r, s, t, cnt;
  guchar  *mem;
  guchar   value;
  guint32  pixel;

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
		*mem++ = (guchar) ((pixel >> (t * 8)) & 0xFF);
	    }
	  else
	    {
	      for (t = 0; t < image->bpp; t++)
		*mem++ = (guchar) ((pixel >> ((image->bpp - t - 1) * 8)) & 0xFF);
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


void
create_toolbox (void)
{
  GtkWidget *window;
  GtkWidget *main_vbox;
  GtkWidget *wbox;
  GtkWidget *menubar;
  GList     *list;
  GtkAccelGroup *table;
  GdkGeometry    geometry;
  GtkStyle      *style;
  gint           xthickness;
  gint           ythickness;


  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  /* Register dialog */
  dialog_register_toolbox (window);

  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
		      GTK_SIGNAL_FUNC (toolbox_delete),
		      NULL);

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (toolbox_destroy),
		      NULL);

  /* We need to know when the current device changes, so we can update
   * the correct tool - to do this we connect to motion events.
   * We can't just use EXTENSION_EVENTS_CURSOR though, since that
   * would get us extension events for the mouse pointer, and our
   * device would change to that and not change back. So we check
   * manually that all devices have a cursor, before establishing the check.
   */
  for (list = gdk_input_list_devices (); list; list = g_list_next (list))
    {
      if (!((GdkDeviceInfo *) (list->data))->has_cursor)
	break;
    }

  if (!list)  /* all devices have cursor */
    {
      gtk_signal_connect (GTK_OBJECT (window), "motion_notify_event",
			  GTK_SIGNAL_FUNC (toolbox_check_device),
			  NULL);

      gtk_widget_set_events (window, GDK_POINTER_MOTION_MASK);
      gtk_widget_set_extension_events (window, GDK_EXTENSION_EVENTS_CURSOR);
    }
  
  /* set up the window geometry after the events have been set, 
     since we need to realize the widget */
  gtk_window_set_wmclass (GTK_WINDOW (window), "toolbox", "Gimp");
  gtk_window_set_title (GTK_WINDOW (window), _("The GIMP"));
  gtk_window_set_policy (GTK_WINDOW (window), TRUE, TRUE, FALSE);

  gtk_widget_realize (window);
  style = gtk_widget_get_style (window);
  xthickness = ((GtkStyleClass *) style->klass)->xthickness;
  ythickness = ((GtkStyleClass *) style->klass)->ythickness;
  
  geometry.min_width  =   2 + 24 + 2 * xthickness;
  geometry.min_height = 100 + 24 + 2 * ythickness;
  geometry.width_inc  =       24 + 2 * xthickness;
  geometry.height_inc =       24 + 2 * ythickness;

  gtk_window_set_geometry_hints (GTK_WINDOW (window), 
				 NULL,
				 &geometry, 
				 GDK_HINT_MIN_SIZE | GDK_HINT_RESIZE_INC);

  session_set_window_geometry (window, &toolbox_session_info, TRUE);

  main_vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 1);
  gtk_container_add (GTK_CONTAINER (window), main_vbox);
  gtk_widget_show (main_vbox);

  /*  allocate the colors for creating pixmaps  */
  allocate_colors (main_vbox);

  /*  tooltips  */
  gimp_help_init ();
  if (!show_tool_tips)
    gimp_help_disable_tooltips ();

  /*  Build the menu bar with menus  */
  menus_get_toolbox_menubar (&menubar, &table);
  gtk_box_pack_start (GTK_BOX (main_vbox), menubar, FALSE, TRUE, 0);
  gtk_widget_show (menubar);

  /*  Install the accelerator table in the main window  */
  gtk_window_add_accel_group (GTK_WINDOW (window), table);

  /*  Connect the "F1" help key  */
  gimp_help_connect_help_accel (window,
				gimp_standard_help_func,
				"toolbox/toolbox.html");

  wbox = gtk_hwrap_box_new (FALSE);
  gtk_wrap_box_set_justify (GTK_WRAP_BOX (wbox), GTK_JUSTIFY_TOP);
  gtk_wrap_box_set_line_justify (GTK_WRAP_BOX (wbox), GTK_JUSTIFY_LEFT);
  /*  magic number to set a default 5x5 layout  */
  gtk_wrap_box_set_aspect_ratio (GTK_WRAP_BOX (wbox), 5.0 / 5.9);
  gtk_container_set_border_width (GTK_CONTAINER (wbox), 0);
  gtk_box_pack_start (GTK_BOX (main_vbox), wbox, TRUE, TRUE, 0);
  gtk_widget_show (wbox);

  create_tools (wbox);

  create_color_area (wbox);
  if (show_indicators)
    create_indicator_area (wbox);

  gtk_widget_show (window);
  toolbox_set_drag_dest (window);

  toolbox_shell = window;
}

void
toolbox_free (void)
{
  gint i;

  session_get_window_info (toolbox_shell, &toolbox_session_info);

  gtk_widget_destroy (toolbox_shell);
  for (i = 0; i < num_tools; i++)
    {
      if (tool_info[i].icon_pixmap)
	gdk_pixmap_unref (tool_info[i].icon_pixmap);
      
      if (!tool_info[i].icon_data)
	gtk_object_sink (GTK_OBJECT (tool_info[i].tool_widget));
    }
  gimp_help_free ();
}

void
toolbox_raise_callback (GtkWidget *widget,
			gpointer   data)
{
  gdk_window_raise (toolbox_shell->window);
}

void
create_display_shell (GDisplay *gdisp,
		      gint      width,
		      gint      height,
		      gchar    *title,
		      gint      type)
{
  static GtkWidget     *image_popup_menu  = NULL;
  static GtkAccelGroup *image_accel_group = NULL;

  static GdkPixmap *qmasksel_pixmap   = NULL;
  static GdkBitmap *qmasksel_mask     = NULL;
  static GdkPixmap *qmasknosel_pixmap = NULL;
  static GdkBitmap *qmasknosel_mask   = NULL;
  static GdkPixmap *navbutton_pixmap  = NULL;
  static GdkBitmap *navbutton_mask    = NULL;

  GtkWidget *main_vbox;
  GtkWidget *disp_vbox;
  GtkWidget *upper_hbox;
  GtkWidget *lower_hbox;
  GtkWidget *inner_table;
  GtkWidget *status_hbox;
  GtkWidget *arrow;
  GtkWidget *pixmap;
  GtkWidget *label_frame;
  GtkWidget *nav_ebox;

  GSList *group = NULL;

  gint n_width, n_height;
  gint s_width, s_height;
  gint scalesrc, scaledest;
  gint contextid;

  /*  adjust the initial scale -- so that window fits on screen */
  s_width  = gdk_screen_width ();
  s_height = gdk_screen_height ();

  scalesrc  = SCALESRC (gdisp);
  scaledest = SCALEDEST (gdisp);

  n_width  = SCALEX (gdisp, width);
  n_height = SCALEX (gdisp, height);

  /*  Limit to the size of the screen...  */
  while (n_width > s_width || n_height > s_height)
    {
      if (scaledest > 1)
	scaledest--;
      else
	if (scalesrc < 0xff)
	  scalesrc++;

      n_width  = width * 
	(scaledest * SCREEN_XRES (gdisp)) / (scalesrc * gdisp->gimage->xresolution);
      n_height = height *
	(scaledest * SCREEN_XRES (gdisp)) / (scalesrc * gdisp->gimage->xresolution);
    }

  gdisp->scale = (scaledest << 8) + scalesrc;

  /*  the toplevel shell */
  gdisp->shell = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_ref  (gdisp->shell);
  gtk_window_set_title (GTK_WINDOW (gdisp->shell), title);
  gtk_window_set_wmclass (GTK_WINDOW (gdisp->shell), "image_window", "Gimp");
  gtk_window_set_policy (GTK_WINDOW (gdisp->shell), TRUE, TRUE, TRUE);
  gtk_object_set_user_data (GTK_OBJECT (gdisp->shell), (gpointer) gdisp);
  gtk_widget_set_events (gdisp->shell,
			 GDK_POINTER_MOTION_MASK |
			 GDK_POINTER_MOTION_HINT_MASK |
			 GDK_BUTTON_PRESS_MASK |
			 GDK_KEY_PRESS_MASK |
			 GDK_KEY_RELEASE_MASK);
  gtk_signal_connect (GTK_OBJECT (gdisp->shell), "delete_event",
		      GTK_SIGNAL_FUNC (gdisplay_delete),
		      gdisp);
  gtk_signal_connect (GTK_OBJECT (gdisp->shell), "destroy",
		      GTK_SIGNAL_FUNC (gdisplay_destroy),
		      gdisp);

  /*  active display callback  */
  gtk_signal_connect (GTK_OBJECT (gdisp->shell), "button_press_event",
		      GTK_SIGNAL_FUNC (gdisplay_shell_events),
		      gdisp);
  gtk_signal_connect (GTK_OBJECT (gdisp->shell), "key_press_event",
		      GTK_SIGNAL_FUNC (gdisplay_shell_events),
		      gdisp);

  /*  dnd stuff  */
  gtk_drag_dest_set (gdisp->shell,
		     GTK_DEST_DEFAULT_ALL,
		     display_target_table, display_n_targets,
		     GDK_ACTION_COPY);
  gtk_signal_connect (GTK_OBJECT (gdisp->shell), "drag_drop",
		      GTK_SIGNAL_FUNC (gdisplay_drag_drop),
		      gdisp);
  gimp_dnd_color_dest_set (gdisp->shell, gdisplay_drop_color, gdisp);
  gimp_dnd_pattern_dest_set (gdisp->shell, gdisplay_drop_pattern, gdisp);

  if (! image_popup_menu)
    menus_get_image_menu (&image_popup_menu, &image_accel_group);

  /*  the popup menu  */
  gdisp->popup = image_popup_menu;

  /*  The accelerator table for images  */
  gtk_window_add_accel_group (GTK_WINDOW (gdisp->shell), image_accel_group);

  /*  connect the "F1" help key  */
  gimp_help_connect_help_accel (gdisp->shell,
				gimp_standard_help_func,
				"image/image_window.html");

  /*  GtkTable widgets are not able to shrink a row/column correctly if
   *  widgets are attached with GTK_EXPAND even if those widgets have
   *  other rows/columns in their rowspan/colspan where they could
   *  nicely expand without disturbing the row/column which is supposed
   *  to shrink. --Mitch
   *
   *  Changed the packing to use hboxes and vboxes which behave nicer:
   *
   *  main_vbox
   *     |
   *     +-- disp_vbox
   *     |      |
   *     |      +-- upper_hbox
   *     |      |      |
   *     |      |      +-- inner_table
   *     |      |      |      |
   *     |      |      |      +-- origin
   *     |      |      |      +-- hruler
   *     |      |      |      +-- vruler
   *     |      |      |      +-- canvas
   *     |      |      |     
   *     |      |      +-- vscrollbar
   *     |      |    
   *     |      +-- lower_hbox
   *     |             |
   *     |             +-- qmaskoff
   *     |             +-- qmaskon
   *     |             +-- hscrollbar
   *     |             +-- navbutton
   *     |
   *     +-- statusarea
   *            |
   *            +-- cursorlabel
   *            +-- statusbar
   *            +-- progressbar
   *            +-- cancelbutton
   */

  /*  first, set up the container hierarchy  *********************************/

  /*  the vbox containing all widgets  */
  main_vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 2);
  gtk_container_add (GTK_CONTAINER (gdisp->shell), main_vbox);

  /*  another vbox for everything except the statusbar  */
  disp_vbox = gtk_vbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (main_vbox), disp_vbox, TRUE, TRUE, 0);
  gtk_widget_show (disp_vbox);

  /*  a hbox for the inner_table and the vertical scrollbar  */
  upper_hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (disp_vbox), upper_hbox, TRUE, TRUE, 0);
  gtk_widget_show (upper_hbox);

  /*  the table containing origin, rulers and the canvas  */
  inner_table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (inner_table), 0, 1);
  gtk_table_set_row_spacing (GTK_TABLE (inner_table), 0, 1);
  gtk_box_pack_start (GTK_BOX (upper_hbox), inner_table, TRUE, TRUE, 0);
  gtk_widget_show (inner_table);

  /*  the hbox containing qmask buttons, vertical scrollbar and nav button  */
  lower_hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (disp_vbox), lower_hbox, FALSE, FALSE, 0);
  gtk_widget_show (lower_hbox);

  /*  eventbox and hbox for status area  */
  gdisp->statusarea = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (main_vbox), gdisp->statusarea, FALSE, FALSE, 0);

  gimp_help_set_help_data (gdisp->statusarea, NULL, "#status_area");

  status_hbox = gtk_hbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (gdisp->statusarea), status_hbox);
  gtk_widget_show (status_hbox);

  gtk_container_set_resize_mode (GTK_CONTAINER (status_hbox),
				 GTK_RESIZE_QUEUE);

  /*  create the scrollbars  *************************************************/

  /*  the horizontal scrollbar  */
  gdisp->hsbdata =
    GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, width, 1, 1, width));
  gdisp->hsb = gtk_hscrollbar_new (gdisp->hsbdata);
  GTK_WIDGET_UNSET_FLAGS (gdisp->hsb, GTK_CAN_FOCUS);

  /*  the vertical scrollbar  */
  gdisp->vsbdata =
    GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, height, 1, 1, height));
  gdisp->vsb = gtk_vscrollbar_new (gdisp->vsbdata);
  GTK_WIDGET_UNSET_FLAGS (gdisp->vsb, GTK_CAN_FOCUS);

  /*  create the contents of the inner_table  ********************************/

  /*  the menu popup button  */
  gdisp->origin = gtk_button_new ();
  GTK_WIDGET_UNSET_FLAGS (gdisp->origin, GTK_CAN_FOCUS);
  gtk_widget_set_events (GTK_WIDGET (gdisp->origin),
			 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  gtk_signal_connect (GTK_OBJECT (gdisp->origin), "button_press_event",
		      GTK_SIGNAL_FUNC (gdisplay_origin_button_press),
		      gdisp);

  gimp_help_set_help_data (gdisp->origin, NULL, "#origin_button");

  arrow = gtk_arrow_new (GTK_ARROW_RIGHT, GTK_SHADOW_OUT);
  gtk_container_set_border_width (GTK_CONTAINER (gdisp->origin), 0);
  gtk_container_add (GTK_CONTAINER (gdisp->origin), arrow);
  gtk_widget_show (arrow);

  /*  the horizontal ruler  */
  gdisp->hrule = gtk_hruler_new ();
  gtk_widget_set_events (GTK_WIDGET (gdisp->hrule),
			 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  gtk_signal_connect_object (GTK_OBJECT (gdisp->shell), "motion_notify_event",
			     GTK_SIGNAL_FUNC (GTK_WIDGET_CLASS (GTK_OBJECT (gdisp->hrule)->klass)->motion_notify_event),
			     GTK_OBJECT (gdisp->hrule));
  gtk_signal_connect (GTK_OBJECT (gdisp->hrule), "button_press_event",
		      GTK_SIGNAL_FUNC (gdisplay_hruler_button_press),
		      gdisp);

  gimp_help_set_help_data (gdisp->hrule, NULL, "#ruler");

  /*  the vertical ruler  */
  gdisp->vrule = gtk_vruler_new ();
  gtk_widget_set_events (GTK_WIDGET (gdisp->vrule),
			 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  gtk_signal_connect_object (GTK_OBJECT (gdisp->shell), "motion_notify_event",
			     GTK_SIGNAL_FUNC (GTK_WIDGET_CLASS (GTK_OBJECT (gdisp->vrule)->klass)->motion_notify_event),
			     GTK_OBJECT (gdisp->vrule));
  gtk_signal_connect (GTK_OBJECT (gdisp->vrule), "button_press_event",
		      GTK_SIGNAL_FUNC (gdisplay_vruler_button_press),
		      gdisp);

  gimp_help_set_help_data (gdisp->vrule, NULL, "#ruler");

  /*  the canvas  */
  gdisp->canvas = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (gdisp->canvas), n_width, n_height);
  gtk_widget_set_events (gdisp->canvas, CANVAS_EVENT_MASK);
  gtk_widget_set_extension_events (gdisp->canvas, GDK_EXTENSION_EVENTS_ALL);
  GTK_WIDGET_SET_FLAGS (gdisp->canvas, GTK_CAN_FOCUS);
  gtk_object_set_user_data (GTK_OBJECT (gdisp->canvas), (gpointer) gdisp);

  /*  set the active display before doing any other canvas event processing  */
  gtk_signal_connect (GTK_OBJECT (gdisp->canvas), "event",
		      GTK_SIGNAL_FUNC (gdisplay_shell_events),
		      gdisp);
  gtk_signal_connect (GTK_OBJECT (gdisp->canvas), "event",
		      GTK_SIGNAL_FUNC (gdisplay_canvas_events),
		      gdisp);

  /*  create the contents of the lower_hbox  *********************************/

  /*  the qmask buttons  */
  gdisp->qmaskoff = gtk_radio_button_new (group);
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (gdisp->qmaskoff));
  gtk_widget_set_usize (GTK_WIDGET (gdisp->qmaskoff), 15, 15);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (gdisp->qmaskoff), FALSE);
  GTK_WIDGET_UNSET_FLAGS (gdisp->qmaskoff, GTK_CAN_FOCUS);
  gtk_signal_connect (GTK_OBJECT (gdisp->qmaskoff), "toggled",
		      GTK_SIGNAL_FUNC (qmask_deactivate),
		      gdisp);
  gtk_signal_connect (GTK_OBJECT (gdisp->qmaskoff), "button_press_event",
		      GTK_SIGNAL_FUNC (qmask_click_handler),
		      gdisp);

  gimp_help_set_help_data (gdisp->qmaskoff, NULL, "#qmask_off_button");

  gdisp->qmaskon = gtk_radio_button_new (group);
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (gdisp->qmaskon));
  gtk_widget_set_usize (GTK_WIDGET (gdisp->qmaskon), 15, 15);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (gdisp->qmaskon), FALSE);
  GTK_WIDGET_UNSET_FLAGS (gdisp->qmaskon, GTK_CAN_FOCUS);
  gtk_signal_connect (GTK_OBJECT (gdisp->qmaskon), "toggled",
		      GTK_SIGNAL_FUNC (qmask_activate),
		      gdisp);
  gtk_signal_connect (GTK_OBJECT (gdisp->qmaskon), "button_press_event",
		      GTK_SIGNAL_FUNC (qmask_click_handler),
		      gdisp);

  gimp_help_set_help_data (gdisp->qmaskon, NULL, "#qmask_on_button");

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gdisp->qmaskoff), TRUE);

  /*  the navigation window button  */
  nav_ebox = gtk_event_box_new ();
  gtk_signal_connect (GTK_OBJECT (nav_ebox), "button_press_event",
		      GTK_SIGNAL_FUNC (nav_popup_click_handler),
		      gdisp);

  gimp_help_set_help_data (nav_ebox, NULL, "#nav_window_button");

  /*  create the pixmaps  ****************************************************/
  if (!qmasksel_pixmap)
    {
      GtkStyle *style;

      gtk_widget_realize (gdisp->shell);
      style = gtk_widget_get_style (gdisp->shell);

      qmasksel_pixmap =
	gdk_pixmap_create_from_xpm_d (gdisp->shell->window,
				      &qmasksel_mask,
				      &style->bg[GTK_STATE_NORMAL],
				      qmasksel_xpm);   
      qmasknosel_pixmap =
	gdk_pixmap_create_from_xpm_d (gdisp->shell->window,
				      &qmasknosel_mask,
				      &style->bg[GTK_STATE_NORMAL],
				      qmasknosel_xpm);   
      navbutton_pixmap =
	gdk_pixmap_create_from_xpm_d (gdisp->shell->window,
				      &navbutton_mask,
				      &style->bg[GTK_STATE_NORMAL],
				      navbutton_xpm);   
    }

  /*  create the GtkPixmaps  */
  pixmap = gtk_pixmap_new (qmasksel_pixmap, qmasksel_mask);
  gtk_container_add (GTK_CONTAINER (gdisp->qmaskon), pixmap);
  gtk_widget_show (pixmap);

  pixmap = gtk_pixmap_new (qmasknosel_pixmap, qmasknosel_mask);
  gtk_container_add (GTK_CONTAINER (gdisp->qmaskoff), pixmap);
  gtk_widget_show (pixmap);

  pixmap = gtk_pixmap_new (navbutton_pixmap, navbutton_mask);
  gtk_container_add (GTK_CONTAINER (nav_ebox), pixmap); 
  gtk_widget_show (pixmap);

  /*  create the contents of the status area *********************************/

  /*  the cursor label  */
  label_frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (label_frame), GTK_SHADOW_IN);

  gdisp->cursor_label = gtk_label_new (" ");
  gtk_container_add (GTK_CONTAINER (label_frame), gdisp->cursor_label);
  gtk_widget_show (gdisp->cursor_label);

  /*  the statusbar  */
  gdisp->statusbar = gtk_statusbar_new ();
  gtk_widget_set_usize (gdisp->statusbar, 1, -1);
  gtk_container_set_resize_mode (GTK_CONTAINER (gdisp->statusbar),
				 GTK_RESIZE_QUEUE);
  contextid = gtk_statusbar_get_context_id (GTK_STATUSBAR (gdisp->statusbar),
					    "title");
  gtk_statusbar_push (GTK_STATUSBAR (gdisp->statusbar),
		      contextid,
		      title);

  /*  the progress bar  */
  gdisp->progressbar = gtk_progress_bar_new ();
  gtk_widget_set_usize (gdisp->progressbar, 80, -1);

  /*  the cancel button  */
  gdisp->cancelbutton = gtk_button_new_with_label (_("Cancel"));
  gtk_widget_set_sensitive (gdisp->cancelbutton, FALSE);

  /*  pack all the widgets  **************************************************/

  /*  fill the upper_hbox  */
  gtk_box_pack_start (GTK_BOX (upper_hbox), gdisp->vsb, FALSE, FALSE, 0);

  /*  fill the inner_table  */
  gtk_table_attach (GTK_TABLE (inner_table), gdisp->origin, 0, 1, 0, 1,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (inner_table), gdisp->hrule, 1, 2, 0, 1,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (inner_table), gdisp->vrule, 0, 1, 1, 2,
		    GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (inner_table), gdisp->canvas, 1, 2, 1, 2,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

  /*  fill the lower_hbox  */
  gtk_box_pack_start (GTK_BOX (lower_hbox), gdisp->qmaskoff, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (lower_hbox), gdisp->qmaskon, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (lower_hbox), gdisp->hsb, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (lower_hbox), nav_ebox, FALSE, FALSE, 0);

  /*  fill the status area  */
  gtk_box_pack_start (GTK_BOX (status_hbox), label_frame, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (status_hbox), gdisp->statusbar, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (status_hbox), gdisp->progressbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (status_hbox), gdisp->cancelbutton, FALSE, FALSE, 0);

  /*  show everything  *******************************************************/

  if (show_rulers)
    {
      gtk_widget_show (gdisp->origin);
      gtk_widget_show (gdisp->hrule);
      gtk_widget_show (gdisp->vrule);
    }
  gtk_widget_show (gdisp->canvas);

  gtk_widget_show (gdisp->vsb);
  gtk_widget_show (gdisp->hsb);

  gtk_widget_show (gdisp->qmaskoff);
  gtk_widget_show (gdisp->qmaskon);
  gtk_widget_show (nav_ebox);

  gtk_widget_show (label_frame);
  gtk_widget_show (gdisp->statusbar);
  gtk_widget_show (gdisp->progressbar);
  gtk_widget_show (gdisp->cancelbutton);
  if (show_statusbar)
    {
      gtk_widget_show (gdisp->statusarea);
    }

  gtk_widget_realize (gdisp->canvas);
  gdk_window_set_back_pixmap (gdisp->canvas->window, NULL, FALSE);

  /*  we need to realize the cursor_label widget here, so the size gets
   *  computed correctly
   */
  gtk_widget_realize (gdisp->cursor_label);
  gdisplay_resize_cursor_label (gdisp);

  gtk_widget_show (main_vbox);
  gtk_widget_show (gdisp->shell);

  /*  set the focus to the canvas area  */
  gtk_widget_grab_focus (gdisp->canvas);
}

/* DnD functions */ 
static void
toolbox_set_drag_dest (GtkWidget *object)
{
  gtk_drag_dest_set (object,
		     GTK_DEST_DEFAULT_ALL,
		     toolbox_target_table, toolbox_n_targets,
		     GDK_ACTION_COPY);

  gtk_signal_connect (GTK_OBJECT (object), "drag_data_received",
		      GTK_SIGNAL_FUNC (toolbox_drag_data_received),
		      object);
  gtk_signal_connect (GTK_OBJECT (object), "drag_drop",
		      GTK_SIGNAL_FUNC (toolbox_drag_drop),
		      NULL);

  gimp_dnd_tool_dest_set (object, toolbox_drop_tool, NULL);
}

static void
toolbox_drag_data_received (GtkWidget        *widget,
			    GdkDragContext   *context,
			    gint              x,
			    gint              y,
			    GtkSelectionData *data,
			    guint             info,
			    guint             time)
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

static gboolean
toolbox_drag_drop (GtkWidget      *widget,
		   GdkDragContext *context,
		   gint            x,
		   gint            y,
		   guint           time)
{
  GtkWidget *src_widget;
  gboolean return_val = FALSE;

  if ((src_widget = gtk_drag_get_source_widget (context)))
    {
      GimpDrawable *drawable       = NULL;
      Layer        *layer          = NULL;
      Channel      *channel        = NULL;
      LayerMask    *layer_mask     = NULL;
      GImage       *component      = NULL;
      ChannelType   component_type = -1;

      layer = (Layer *) gtk_object_get_data (GTK_OBJECT (src_widget),
					     "gimp_layer");
      channel = (Channel *) gtk_object_get_data (GTK_OBJECT (src_widget),
						 "gimp_channel");
      layer_mask = (LayerMask *) gtk_object_get_data (GTK_OBJECT (src_widget),
						      "gimp_layer_mask");
      component = (GImage *) gtk_object_get_data (GTK_OBJECT (src_widget),
						  "gimp_component");

      if (layer)
	{
	  drawable = GIMP_DRAWABLE (layer);
	}
      else if (channel)
	{
	  drawable = GIMP_DRAWABLE (channel);
	}
      else if (layer_mask)
	{
	  drawable = GIMP_DRAWABLE (layer_mask);
	}
      else if (component)
	{
	  component_type =
	    (ChannelType) gtk_object_get_data (GTK_OBJECT (src_widget),
					       "gimp_component_type");
	}

      if (drawable)
	{
          GImage *gimage;
	  GImage *new_gimage;
	  Layer  *new_layer;
          gint    width, height;
	  gint    off_x, off_y;
	  gint    bytes;

	  GimpImageBaseType type;

	  gimage = gimp_drawable_gimage (drawable);
          width  = gimp_drawable_width  (drawable);
          height = gimp_drawable_height (drawable);
	  bytes  = gimp_drawable_bytes  (drawable);

	  switch (gimp_drawable_type (drawable))
	    {
	    case RGB_GIMAGE: case RGBA_GIMAGE:
	      type = RGB; break;
	    case GRAY_GIMAGE: case GRAYA_GIMAGE:
	      type = GRAY; break;
	    case INDEXED_GIMAGE: case INDEXEDA_GIMAGE:
	      type = INDEXED; break;
	    default:
	      type = RGB; break;
	    }

	  new_gimage = gimage_new (width, height, type);
	  gimage_disable_undo (new_gimage);

	  if (type == INDEXED) /* copy the colormap */
	    {
	      new_gimage->num_cols = gimage->num_cols;
	      memcpy (new_gimage->cmap, gimage->cmap, COLORMAP_SIZE);
	    }

	  gimage_set_resolution (new_gimage, gimage->xresolution, gimage->yresolution);
	  gimage_set_unit (new_gimage, gimage->unit);

	  if (layer)
	    {
	      new_layer = layer_copy (layer, FALSE);
	    }
	  else
	    {
	      /*  a non-layer drawable can't have an alpha channel,
	       *  so add one
	       */
	      PixelRegion  srcPR, destPR;
	      TileManager *tiles;

	      tiles = tile_manager_new (width, height, bytes + 1);

	      pixel_region_init (&srcPR, gimp_drawable_data (drawable),
				 0, 0, width, height, FALSE);
	      pixel_region_init (&destPR, tiles,
				 0, 0, width, height, TRUE);

	      add_alpha_region (&srcPR, &destPR);

	      new_layer = layer_new_from_tiles (new_gimage, 
                                                gimp_image_base_type_with_alpha(new_gimage), 
                                                tiles,
						"", OPAQUE_OPACITY, NORMAL_MODE);

	      tile_manager_destroy (tiles);
	    }

	  gimp_drawable_set_gimage (GIMP_DRAWABLE (new_layer), new_gimage);

	  layer_set_name (GIMP_LAYER (new_layer),
			  gimp_drawable_get_name (drawable));

	  if (layer)
	    {
	      LayerMask *mask;
	      LayerMask *new_mask;

	      mask     = layer_get_mask (layer);
	      new_mask = layer_get_mask (new_layer);

	      if (new_mask)
		{
		  gimp_drawable_set_name (GIMP_DRAWABLE (new_mask),
					  gimp_drawable_get_name (GIMP_DRAWABLE (mask)));
		}
	    }

	  gimp_drawable_offsets (GIMP_DRAWABLE (new_layer), &off_x, &off_y);
	  layer_translate (new_layer, -off_x, -off_y);

	  gimage_add_layer (new_gimage, new_layer, 0);

	  gimp_context_set_display (gimp_context_get_user (),
				    gdisplay_new (new_gimage, 0x0101));

	  gimage_enable_undo (new_gimage);

	  return_val = TRUE;
	}
    }

  gtk_drag_finish (context, return_val, FALSE, time);

  return return_val;
}

static ToolType
toolbox_drag_tool (GtkWidget *widget,
		   gpointer   data)
{
  return (ToolType) data;
}

static void
toolbox_drop_tool (GtkWidget *widget,
		   ToolType   tool,
		   gpointer   data)
{
  gimp_context_set_tool (gimp_context_get_user (), tool);
}

static void
gimp_dnd_open_files (gchar *buffer)
{
  gchar	 name_buffer[1024];
  const gchar *data_type = "file:";
  const gint sig_len = strlen (data_type);

  while (*buffer)
    {
      gchar *name = name_buffer;
      gint len = 0;

      while ((*buffer != 0) && (*buffer != '\n') && len < 1024)
	{
	  *name++ = *buffer++;
	  len++;
	}
      if (len == 0)
	break;

      if (*(name - 1) == 0xd)	/* gmc uses RETURN+NEWLINE as delimiter */
	*(name - 1) = '\0';
      else
	*name = '\0';
      name = name_buffer;
      if ((sig_len < len) && (! strncmp (name, data_type, sig_len)))
	name += sig_len;
      
      file_open (name, name);
      
      if (*buffer)
	buffer++;
    }
}
