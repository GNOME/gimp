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

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "apptypes.h"

#include "app_procs.h"
#include "color_area.h"
#include "devices.h"
#include "dialog_handler.h"
#include "gdisplay.h"
#include "gimage.h"
#include "gimpcontext.h"
#include "gimpdnd.h"
#include "gimplayer.h"
#include "gimprc.h"
#include "gtkhwrapbox.h"
#include "indicator_area.h"
#include "menus.h"
#include "paint_funcs.h"
#include "session.h"
#include "pixel_region.h"
#include "tile_manager.h"

#include "tools/tool_options_dialog.h"
#include "tools/tool_manager.h"

#include "libgimp/gimpintl.h"

#include "pixmaps.h"


/*  local functions  */
static void        tools_select_update        (GtkWidget      *widget,
					       gpointer        data);
static gint        tools_button_press         (GtkWidget      *widget,
					       GdkEventButton *bevent,
					       gpointer        data);

static void        toolbox_destroy            (void);
static gint        toolbox_delete             (GtkWidget      *widget,
					       GdkEvent       *event,
					       gpointer        data);
static gint        toolbox_check_device       (GtkWidget      *widget,
					       GdkEvent       *event,
					       gpointer        data);

static GdkPixmap * create_pixmap              (GdkWindow      *parent,
					       GdkBitmap     **mask,
					       gchar         **data,
					       gint            width,
					       gint            height);

static void        toolbox_style_set_callback (GtkWidget      *window,
					       GtkStyle       *previous_style,
					       gpointer        data);
static void        toolbox_set_drag_dest      (GtkWidget      *widget);
static gboolean    toolbox_drag_drop          (GtkWidget      *widget,
					       GdkDragContext *context,
					       gint            x,
					       gint            y,
					       guint           time);
static GimpTool  * toolbox_drag_tool          (GtkWidget      *widget,
					       gpointer        data);
static void        toolbox_drop_tool          (GtkWidget      *widget,
					       GimpTool       *tool,
					       gpointer        data);


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


static void
tools_select_update (GtkWidget *widget,
		     gpointer   data)
{
  GimpToolClass *tool_type;

  tool_type = GIMP_TOOL_CLASS (data);

  if ((tool_type) && GTK_TOGGLE_BUTTON (widget)->active)
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
allocate_colors (GtkWidget *parent)
{
  GdkColormap *colormap;
  gint i;

  gtk_widget_realize (parent);
  colormap = gdk_window_get_colormap (parent->window);

  for (i = 0; i < 8; i++)
    {
      colors[i].red   = pixmap_colors[i][0] << 8;
      colors[i].green = pixmap_colors[i][1] << 8;
      colors[i].blue  = pixmap_colors[i][2] << 8;

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
  GSList *tools;
  GimpToolClass *tool;

  g_return_if_fail (parent != NULL);

  for (tools = registered_tools; tools; tools=tools->next)
    {
      tool = GIMP_TOOL_CLASS (tools->data);
      if (tool->icon_data)
	tool->icon_pixmap = create_pixmap (parent->window, 
						  &tool->icon_mask,
						  tool->icon_data,
						  22, 22);
      else
	tool->icon_pixmap = create_pixmap (parent->window,  
						  &tool->icon_mask,
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
  GSList *group, *tools;
  GimpToolClass *tool;
  gint    i, j;

  wbox = parent;

  if (! GTK_WIDGET_REALIZED (gtk_widget_get_toplevel (wbox)))
    gtk_widget_realize (gtk_widget_get_toplevel (wbox));

  create_tool_pixmaps (wbox);

  group = NULL;

  i = 0;
  for (tools = registered_tools; tools; tools=tools->next)
    {
      tool = GIMP_TOOL_CLASS(tools->data);
	  tool->tool_widget = button = gtk_radio_button_new (group);
	  gtk_container_set_border_width (GTK_CONTAINER (button), 0);
	  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));

	  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);

	  gtk_wrap_box_pack (GTK_WRAP_BOX (wbox), button,
			     FALSE, FALSE, FALSE, FALSE);

	  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
	  gtk_container_set_border_width (GTK_CONTAINER (alignment), 0);
	  gtk_container_add (GTK_CONTAINER (button), alignment);

	  pixmap = gtk_pixmap_new (gimp_tool_get_pixmap (tool), 
				   gimp_tool_get_mask (tool));
	  gtk_container_add (GTK_CONTAINER (alignment), pixmap);

	  gtk_signal_connect (GTK_OBJECT (button), "toggled",
			      GTK_SIGNAL_FUNC (tools_select_update),
			      (gpointer) tool);

	  gtk_signal_connect (GTK_OBJECT (button), "button_press_event",
			      GTK_SIGNAL_FUNC (tools_button_press),
			      (gpointer) tool);

	  /*  dnd stuff  */
	  gtk_drag_source_set (tool->tool_widget,
			       GDK_BUTTON2_MASK,
			       tool_target_table, tool_n_targets,
			       GDK_ACTION_COPY);
	  gimp_dnd_tool_source_set (tool->tool_widget,
				    toolbox_drag_tool, tool);

	  gimp_help_set_help_data (button,
				   gettext(tool->tool_desc),
				   tool->help_data);

	  gtk_widget_show (pixmap);
	  gtk_widget_show (alignment);
	  gtk_widget_show (button);
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
  gint         r, s, t, cnt;
  guchar      *mem;
  guchar       value;
  guint32      pixel;

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
toolbox_create (void)
{
  GtkWidget     *window;
  GtkWidget     *main_vbox;
  GtkWidget     *wbox;
  GtkWidget     *menubar;
  GList         *list;
  GtkAccelGroup *table;

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

  gtk_signal_connect (GTK_OBJECT (window), "style_set",
		      GTK_SIGNAL_FUNC (toolbox_style_set_callback),
		      NULL);

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
  GSList *tools;
  GimpToolClass *tool;

  session_get_window_info (toolbox_shell, &toolbox_session_info);

  gtk_widget_destroy (toolbox_shell);
  for (tools = registered_tools; tools; tools=tools->next)
    {
      tool = GIMP_TOOL_CLASS (tools->data);
      if (tool->icon_pixmap)
	gdk_pixmap_unref (tool->icon_pixmap);
      
      if (!tool->icon_data)
	gtk_object_sink (GTK_OBJECT (tool->tool_widget));
    }
  gimp_help_free ();
}

void
toolbox_raise_callback (GtkWidget *widget,
			gpointer   data)
{
  gdk_window_raise (toolbox_shell->window);
}

static void
toolbox_style_set_callback (GtkWidget *window,
			    GtkStyle  *previous_style,
			    gpointer   data)
{
  GdkGeometry  geometry;
  GtkStyle    *style;
  gint         xthickness;
  gint         ythickness;

  style = gtk_widget_get_style (window);
  xthickness = ((GtkStyleClass *) style->klass)->xthickness;
  ythickness = ((GtkStyleClass *) style->klass)->ythickness;
  
  geometry.min_width  =  2 + 24 + 2 * xthickness;
  geometry.min_height = 80 + 24 + 2 * ythickness;
  geometry.width_inc  =      24 + 2 * xthickness;
  geometry.height_inc =      24 + 2 * ythickness;

  gtk_window_set_geometry_hints (GTK_WINDOW (window), 
				 NULL,
				 &geometry, 
				 GDK_HINT_MIN_SIZE | GDK_HINT_RESIZE_INC);
}

static void
toolbox_set_drag_dest (GtkWidget *object)
{
  gtk_drag_dest_set (object,
		     GTK_DEST_DEFAULT_ALL,
		     toolbox_target_table, toolbox_n_targets,
		     GDK_ACTION_COPY);

  gimp_dnd_file_dest_set (object);

  gtk_signal_connect (GTK_OBJECT (object), "drag_drop",
		      GTK_SIGNAL_FUNC (toolbox_drag_drop),
		      NULL);

  gimp_dnd_tool_dest_set (object, toolbox_drop_tool, NULL);
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
      GimpDrawable  *drawable       = NULL;
      GimpLayer     *layer          = NULL;
      GimpChannel   *channel        = NULL;
      GimpLayerMask *layer_mask     = NULL;
      GimpImage     *component      = NULL;
      ChannelType    component_type = -1;

      layer = (GimpLayer *) gtk_object_get_data (GTK_OBJECT (src_widget),
						 "gimp_layer");
      channel = (GimpChannel *) gtk_object_get_data (GTK_OBJECT (src_widget),
						     "gimp_channel");
      layer_mask = (GimpLayerMask *) gtk_object_get_data (GTK_OBJECT (src_widget),
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
          GimpImage *gimage;
	  GimpImage *new_gimage;
	  GimpLayer *new_layer;
          gint       width, height;
	  gint       off_x, off_y;
	  gint       bytes;

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
	  gimp_image_undo_disable (new_gimage);

	  if (type == INDEXED) /* copy the colormap */
	    {
	      new_gimage->num_cols = gimage->num_cols;
	      memcpy (new_gimage->cmap, gimage->cmap, COLORMAP_SIZE);
	    }

	  gimp_image_set_resolution (new_gimage,
				     gimage->xresolution, gimage->yresolution);
	  gimp_image_set_unit (new_gimage, gimage->unit);

	  if (layer)
	    {
	      new_layer = gimp_layer_copy (layer, FALSE);
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

	      new_layer =
		gimp_layer_new_from_tiles (new_gimage, 
					   gimp_image_base_type_with_alpha (new_gimage), 
					   tiles,
					   "", OPAQUE_OPACITY, NORMAL_MODE);

	      tile_manager_destroy (tiles);
	    }

	  gimp_drawable_set_gimage (GIMP_DRAWABLE (new_layer), new_gimage);

	  gimp_object_set_name (GIMP_OBJECT (new_layer),
				gimp_object_get_name (GIMP_OBJECT (drawable)));

	  if (layer)
	    {
	      GimpLayerMask *mask;
	      GimpLayerMask *new_mask;

	      mask     = gimp_layer_get_mask (layer);
	      new_mask = gimp_layer_get_mask (new_layer);

	      if (new_mask)
		{
		  gimp_object_set_name (GIMP_OBJECT (new_mask),
					gimp_object_get_name (GIMP_OBJECT (mask)));
		}
	    }

	  gimp_drawable_offsets (GIMP_DRAWABLE (new_layer), &off_x, &off_y);
	  gimp_layer_translate (new_layer, -off_x, -off_y);

	  gimp_image_add_layer (new_gimage, new_layer, 0);

	  gimp_context_set_display (gimp_context_get_user (),
				    gdisplay_new (new_gimage, 0x0101));

	  gimp_image_undo_enable (new_gimage);

	  return_val = TRUE;
	}
    }

  gtk_drag_finish (context, return_val, FALSE, time);

  return return_val;
}

static GimpTool *
toolbox_drag_tool (GtkWidget *widget,
		   gpointer   data)
{
  return GIMP_TOOL(data);
}

static void
toolbox_drop_tool (GtkWidget *widget,
		   GimpTool   *tool,
		   gpointer   data)
{
  gimp_context_set_tool (gimp_context_get_user (), tool);
}
