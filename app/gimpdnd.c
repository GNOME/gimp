/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
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
#include "gimage.h"
#include "gimpcontextpreview.h"
#include "gimpdnd.h"
#include "gimpbrushlist.h"
#include "gimprc.h"
#include "gradient_header.h"
#include "interface.h"
#include "tools.h"

#include "libgimp/gimplimits.h"

#define DRAG_PREVIEW_SIZE 32

#define DRAG_ICON_OFFSET  -8

typedef enum
{
  GIMP_DND_DATA_COLOR = 1,
  GIMP_DND_DATA_BRUSH,
  GIMP_DND_DATA_PATTERN,
  GIMP_DND_DATA_GRADIENT,
  GIMP_DND_DATA_PALETTE,
  GIMP_DND_DATA_TOOL,
  GIMP_DND_DATA_LAST = GIMP_DND_DATA_TOOL
} GimpDndDataType;

typedef GtkWidget * (* GimpDndGetIconFunc)  (GtkWidget     *widget,
					     GtkSignalFunc  get_data_func,
					     gpointer       get_data_data);
typedef guchar    * (* GimpDndDragDataFunc) (GtkWidget     *widget,
					     GtkSignalFunc  get_data_func,
					     gpointer       get_data_data,
					     gint          *format,
					     gint          *length);
typedef void        (* GimpDndDropDataFunc) (GtkWidget     *widget,
					     GtkSignalFunc  set_data_func,
					     gpointer       set_data_data,
					     guchar        *vals,
					     gint           format,
					     gint           length);

typedef struct _GimpDndDataDef GimpDndDataDef;

struct _GimpDndDataDef
{
  GtkTargetEntry       target_entry;

  gchar               *set_data_func_name;
  gchar               *set_data_data_name;

  GimpDndGetIconFunc   get_icon_func;
  GimpDndDragDataFunc  get_data_func;
  GimpDndDropDataFunc  set_data_func;
};

static GtkWidget * gimp_dnd_get_color_icon    (GtkWidget     *widget,
					       GtkSignalFunc  get_color_func,
					       gpointer       get_color_data);
static GtkWidget * gimp_dnd_get_brush_icon    (GtkWidget     *widget,
					       GtkSignalFunc  get_brush_func,
					       gpointer       get_brush_data);
static GtkWidget * gimp_dnd_get_pattern_icon  (GtkWidget     *widget,
					       GtkSignalFunc  get_pattern_func,
					       gpointer       get_pattern_data);
static GtkWidget * gimp_dnd_get_gradient_icon (GtkWidget     *widget,
					       GtkSignalFunc  get_gradient_func,
					       gpointer       get_gradient_data);
static GtkWidget * gimp_dnd_get_palette_icon  (GtkWidget     *widget,
					       GtkSignalFunc  get_gradient_func,
					       gpointer       get_gradient_data);
static GtkWidget * gimp_dnd_get_tool_icon     (GtkWidget     *widget,
					       GtkSignalFunc  get_tool_func,
					       gpointer       get_tool_data);

static guchar    * gimp_dnd_get_color_data    (GtkWidget     *widget,
					       GtkSignalFunc  get_color_func,
					       gpointer       get_color_data,
					       gint          *format,
					       gint          *length);
static guchar    * gimp_dnd_get_brush_data    (GtkWidget     *widget,
					       GtkSignalFunc  get_brush_func,
					       gpointer       get_brush_data,
					       gint          *format,
					       gint          *length);
static guchar    * gimp_dnd_get_pattern_data  (GtkWidget     *widget,
					       GtkSignalFunc  get_pattern_func,
					       gpointer       get_pattern_data,
					       gint          *format,
					       gint          *length);
static guchar    * gimp_dnd_get_gradient_data (GtkWidget     *widget,
					       GtkSignalFunc  get_gradient_func,
					       gpointer       get_gradient_data,
					       gint          *format,
					       gint          *length);
static guchar    * gimp_dnd_get_palette_data  (GtkWidget     *widget,
					       GtkSignalFunc  get_gradient_func,
					       gpointer       get_gradient_data,
					       gint          *format,
					       gint          *length);
static guchar    * gimp_dnd_get_tool_data     (GtkWidget     *widget,
					       GtkSignalFunc  get_tool_func,
					       gpointer       get_tool_data,
					       gint          *format,
					       gint          *length);

static void        gimp_dnd_set_color_data    (GtkWidget     *widget,
					       GtkSignalFunc  set_color_func,
					       gpointer       set_color_data,
					       guchar        *vals,
					       gint           format,
					       gint           length);
static void        gimp_dnd_set_brush_data    (GtkWidget     *widget,
					       GtkSignalFunc  set_brush_func,
					       gpointer       set_brush_data,
					       guchar        *vals,
					       gint           format,
					       gint           length);
static void        gimp_dnd_set_pattern_data  (GtkWidget     *widget,
					       GtkSignalFunc  set_pattern_func,
					       gpointer       set_pattern_data,
					       guchar        *vals,
					       gint           format,
					       gint           length);
static void        gimp_dnd_set_gradient_data (GtkWidget     *widget,
					       GtkSignalFunc  set_gradient_func,
					       gpointer       set_gradient_data,
					       guchar        *vals,
					       gint           format,
					       gint           length);
static void        gimp_dnd_set_palette_data  (GtkWidget     *widget,
					       GtkSignalFunc  set_gradient_func,
					       gpointer       set_gradient_data,
					       guchar        *vals,
					       gint           format,
					       gint           length);
static void        gimp_dnd_set_tool_data     (GtkWidget     *widget,
					       GtkSignalFunc  set_tool_func,
					       gpointer       set_tool_data,
					       guchar        *vals,
					       gint           format,
					       gint           length);

static GimpDndDataDef dnd_data_defs[] =
{
  {
    { NULL, 0, -1 },

    NULL,
    NULL,

    NULL,
    NULL,
  },

  {
    GIMP_TARGET_COLOR,

    "gimp_dnd_set_color_func",
    "gimp_dnd_set_color_data",

    gimp_dnd_get_color_icon,
    gimp_dnd_get_color_data,
    gimp_dnd_set_color_data
  },

  {
    GIMP_TARGET_BRUSH,

    "gimp_dnd_set_brush_func",
    "gimp_dnd_set_brush_data",

    gimp_dnd_get_brush_icon,
    gimp_dnd_get_brush_data,
    gimp_dnd_set_brush_data
  },

  {
    GIMP_TARGET_PATTERN,

    "gimp_dnd_set_pattern_func",
    "gimp_dnd_set_pattern_data",

    gimp_dnd_get_pattern_icon,
    gimp_dnd_get_pattern_data,
    gimp_dnd_set_pattern_data
  },

  {
    GIMP_TARGET_GRADIENT,

    "gimp_dnd_set_gradient_func",
    "gimp_dnd_set_gradient_data",

    gimp_dnd_get_gradient_icon,
    gimp_dnd_get_gradient_data,
    gimp_dnd_set_gradient_data
  },

  {
    GIMP_TARGET_PALETTE,

    "gimp_dnd_set_palette_func",
    "gimp_dnd_set_palette_data",

    gimp_dnd_get_palette_icon,
    gimp_dnd_get_palette_data,
    gimp_dnd_set_palette_data
  },

  {
    GIMP_TARGET_TOOL,

    "gimp_dnd_set_tool_func",
    "gimp_dnd_set_tool_data",

    gimp_dnd_get_tool_icon,
    gimp_dnd_get_tool_data,
    gimp_dnd_set_tool_data
  }
};

/********************************/
/*  general data dnd functions  */
/********************************/

static void
gimp_dnd_data_drag_begin (GtkWidget      *widget,
			  GdkDragContext *context,
			  gpointer        data)
{
  GimpDndDataType data_type;
  GtkSignalFunc   get_data_func;
  gpointer        get_data_data;
  GtkWidget      *icon_widget;

  data_type =
    (GimpDndDataType) gtk_object_get_data (GTK_OBJECT (widget),
					   "gimp_dnd_get_data_type");

  if (! data_type)
    return;

  get_data_func =
    (GtkSignalFunc) gtk_object_get_data (GTK_OBJECT (widget),
					 "gimp_dnd_get_data_func");
  get_data_data =
    (gpointer) gtk_object_get_data (GTK_OBJECT (widget),
				    "gimp_dnd_get_data_data");

  if (! get_data_func)
    return;

  icon_widget = (* dnd_data_defs[data_type].get_icon_func) (widget,
							    get_data_func,
							    get_data_data);

  if (icon_widget)
    {
      GtkWidget *frame;
      GtkWidget *window;

      window = gtk_window_new (GTK_WINDOW_POPUP);
      gtk_widget_realize (window);

      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
      gtk_container_add (GTK_CONTAINER (window), frame);
      gtk_widget_show (frame);

      gtk_container_add (GTK_CONTAINER (frame), icon_widget);
      gtk_widget_show (icon_widget);

      gtk_object_set_data_full (GTK_OBJECT (widget),
				"gimp-dnd-data-widget",
				window,
				(GtkDestroyNotify) gtk_widget_destroy);

      gtk_drag_set_icon_widget (context, window,
				DRAG_ICON_OFFSET, DRAG_ICON_OFFSET);
    }
}

static void
gimp_dnd_data_drag_end (GtkWidget      *widget,
			GdkDragContext *context)
{
  gtk_object_set_data (GTK_OBJECT (widget),
		       "gimp-dnd-data-widget", NULL);
}

static void
gimp_dnd_data_drag_handle (GtkWidget        *widget, 
			   GdkDragContext   *context,
			   GtkSelectionData *selection_data,
			   guint             info,
			   guint             time,
			   gpointer          data)
{
  GimpDndDataType data_type;
  GtkSignalFunc   get_data_func;
  gpointer        get_data_data;
  gint    format;
  guchar *vals;
  gint    length;

  data_type =
    (GimpDndDataType) gtk_object_get_data (GTK_OBJECT (widget),
					   "gimp_dnd_get_data_type");

  if (! data_type)
    return;

  get_data_func =
    (GtkSignalFunc) gtk_object_get_data (GTK_OBJECT (widget),
					 "gimp_dnd_get_data_func");
  get_data_data =
    (gpointer) gtk_object_get_data (GTK_OBJECT (widget),
				    "gimp_dnd_get_data_data");

  if (! get_data_func)
    return;

  vals = (* dnd_data_defs[data_type].get_data_func) (widget,
						     get_data_func,
						     get_data_data,
						     &format,
						     &length);

  if (vals)
    {
      gtk_selection_data_set
	(selection_data,
	 gdk_atom_intern (dnd_data_defs[data_type].target_entry.target, FALSE),
	 format, vals, length);

      g_free (vals);
    }
}

static void
gimp_dnd_data_drop_handle (GtkWidget        *widget, 
			   GdkDragContext   *context,
			   gint              x,
			   gint              y,
			   GtkSelectionData *selection_data,
			   guint             info,
			   guint             time,
			   gpointer          data)
{
  GtkSignalFunc   set_data_func;
  gpointer        set_data_data;
  GimpDndDataType data_type;

  if (selection_data->length < 0)
    return;

  for (data_type = GIMP_DND_DATA_COLOR;
       data_type <= GIMP_DND_DATA_LAST;
       data_type++)
    {
      if (dnd_data_defs[data_type].target_entry.info == info)
	{
	  set_data_func = (GtkSignalFunc)
	    gtk_object_get_data (GTK_OBJECT (widget),
				 dnd_data_defs[data_type].set_data_func_name);
	  set_data_data = (gpointer)
	    gtk_object_get_data (GTK_OBJECT (widget),
				 dnd_data_defs[data_type].set_data_data_name);

	  if (! set_data_func)
	    return;

	  (* dnd_data_defs[data_type].set_data_func) (widget,
						      set_data_func,
						      set_data_data,
						      selection_data->data,
						      selection_data->format,
						      selection_data->length);

	  return;
	}
    }
}

static void
gimp_dnd_data_source_set (GimpDndDataType  data_type,
			  GtkWidget       *widget,
			  GtkSignalFunc    get_data_func,
			  gpointer         get_data_data)
{
  gboolean drag_connected;

  drag_connected =
    (gboolean) gtk_object_get_data (GTK_OBJECT (widget),
				    "gimp_dnd_drag_connected");

  if (! drag_connected)
    {
      gtk_signal_connect (GTK_OBJECT (widget), "drag_begin",
			  GTK_SIGNAL_FUNC (gimp_dnd_data_drag_begin),
			  NULL);
      gtk_signal_connect (GTK_OBJECT (widget), "drag_end",
			  GTK_SIGNAL_FUNC (gimp_dnd_data_drag_end),
			  NULL);
      gtk_signal_connect (GTK_OBJECT (widget), "drag_data_get",
			  GTK_SIGNAL_FUNC (gimp_dnd_data_drag_handle),
			  NULL);

      gtk_object_set_data (GTK_OBJECT (widget), "gimp_dnd_drag_connected",
			   (gpointer) TRUE);
    }

  gtk_object_set_data (GTK_OBJECT (widget), "gimp_dnd_get_data_type",
		       (gpointer) data_type);
  gtk_object_set_data (GTK_OBJECT (widget), "gimp_dnd_get_data_func",
		       get_data_func);
  gtk_object_set_data (GTK_OBJECT (widget), "gimp_dnd_get_data_data",
		       get_data_data);
}

static void
gimp_dnd_data_dest_set (GimpDndDataType  data_type,
			GtkWidget       *widget,
			gpointer         set_data_func,
			gpointer         set_data_data)
{
  gboolean drop_connected;

  drop_connected =
    (gboolean) gtk_object_get_data (GTK_OBJECT (widget),
				    "gimp_dnd_drop_connected");

  if (! drop_connected)
    {
      gtk_signal_connect (GTK_OBJECT (widget), "drag_data_received",
			  GTK_SIGNAL_FUNC (gimp_dnd_data_drop_handle),
			  NULL);

      gtk_object_set_data (GTK_OBJECT (widget), "gimp_dnd_drop_connected",
			   (gpointer) TRUE);
    }

  gtk_object_set_data (GTK_OBJECT (widget),
		       dnd_data_defs[data_type].set_data_func_name,
		       set_data_func);
  gtk_object_set_data (GTK_OBJECT (widget),
		       dnd_data_defs[data_type].set_data_data_name,
		       set_data_data);
}

/*************************/
/*  color dnd functions  */
/*************************/

static GtkWidget *
gimp_dnd_get_color_icon (GtkWidget     *widget,
			 GtkSignalFunc  get_color_func,
			 gpointer       get_color_data)
{
  GtkWidget *preview;
  guchar r, g, b;
  guchar row[DRAG_PREVIEW_SIZE * 3];
  gint   i;

  (* (GimpDndDragColorFunc) get_color_func) (widget, &r, &g, &b,
					     get_color_data);

  for (i = 0; i < DRAG_PREVIEW_SIZE; i++)
    {
      row[i * 3]     = r;
      row[i * 3 + 1] = g;
      row[i * 3 + 2] = b;
    }

  preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (preview), 
                    DRAG_PREVIEW_SIZE, DRAG_PREVIEW_SIZE);      

  for (i = 0; i < DRAG_PREVIEW_SIZE; i++)
    {
      gtk_preview_draw_row (GTK_PREVIEW (preview), row,
			    0, i, DRAG_PREVIEW_SIZE);
    }

  return preview;
}

static guchar *
gimp_dnd_get_color_data (GtkWidget     *widget,
			 GtkSignalFunc  get_color_func,
			 gpointer       get_color_data,
			 gint          *format,
			 gint          *length)
{
  guint16 *vals;
  guchar r, g, b;

  (* (GimpDndDragColorFunc) get_color_func) (widget, &r, &g, &b,
					     get_color_data);

  vals = g_new (guint16, 4);

  vals[0] = r + (r << 8);
  vals[1] = g + (g << 8);
  vals[2] = b + (b << 8);
  vals[3] = 0xffff;

  *format = 16;
  *length = 8;

  return (guchar *) vals;
}

static void
gimp_dnd_set_color_data (GtkWidget     *widget,
			 GtkSignalFunc  set_color_func,
			 gpointer       set_color_data,
			 guchar        *vals,
			 gint           format,
			 gint           length)
{
  guint16 *color_vals;
  guchar r, g, b;

  if ((format != 16) || (length != 8))
    {
      g_warning ("Received invalid color data\n");
      return;
    }

  color_vals = (guint16 *) vals;

  r = color_vals[0] >> 8;
  g = color_vals[1] >> 8;
  b = color_vals[2] >> 8;

  (* (GimpDndDropColorFunc) set_color_func) (widget, r, g, b,
					     set_color_data);
}

void
gimp_dnd_color_source_set (GtkWidget            *widget,
			   GimpDndDragColorFunc  get_color_func,
			   gpointer              data)
{
  gimp_dnd_data_source_set (GIMP_DND_DATA_COLOR, widget,
			    GTK_SIGNAL_FUNC (get_color_func),
			    data);
}

void
gimp_dnd_color_dest_set (GtkWidget            *widget,
			 GimpDndDropColorFunc  set_color_func,
			 gpointer              data)
{
  gimp_dnd_data_dest_set (GIMP_DND_DATA_COLOR, widget,
			  GTK_SIGNAL_FUNC (set_color_func),
			  data);
}

/*************************/
/*  brush dnd functions  */
/*************************/

static GtkWidget *
gimp_dnd_get_brush_icon (GtkWidget     *widget,
			 GtkSignalFunc  get_brush_func,
			 gpointer       get_brush_data)
{
  GtkWidget *preview;
  GimpBrush *brush;

  brush = (* (GimpDndDragBrushFunc) get_brush_func) (widget, get_brush_data);

  if (! brush)
    return NULL;

  preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (preview), 
                    DRAG_PREVIEW_SIZE, DRAG_PREVIEW_SIZE);      

  draw_brush (GTK_PREVIEW (preview), brush, 
	      DRAG_PREVIEW_SIZE, DRAG_PREVIEW_SIZE, FALSE);

  return preview;
}

static guchar *
gimp_dnd_get_brush_data (GtkWidget     *widget,
			 GtkSignalFunc  get_brush_func,
			 gpointer       get_brush_data,
			 gint          *format,
			 gint          *length)
{
  GimpBrush *brush;
  gchar *name;

  brush = (* (GimpDndDragBrushFunc) get_brush_func) (widget, get_brush_data);

  if (! brush)
    return NULL;

  name = g_strdup (gimp_brush_get_name (brush));

  if (! name)
    return NULL;

  *format = 8;
  *length = strlen (name) + 1;

  return (guchar *) name;
}

static void
gimp_dnd_set_brush_data (GtkWidget     *widget,
			 GtkSignalFunc  set_brush_func,
			 gpointer       set_brush_data,
			 guchar        *vals,
			 gint           format,
			 gint           length)
{
  GimpBrush *brush;
  gchar *name;

  if ((format != 8) || (length < 1))
    {
      g_warning ("Received invalid brush data\n");
      return;
    }

  name = (gchar *) vals;

  if (strcmp (name, "Standard") == 0)
    brush = brushes_get_standard_brush ();
  else
    brush = gimp_brush_list_get_brush (brush_list, name);

  if (brush)
    (* (GimpDndDropBrushFunc) set_brush_func) (widget, brush, set_brush_data);
}

void
gimp_dnd_brush_source_set (GtkWidget            *widget,
			   GimpDndDragBrushFunc  get_brush_func,
			   gpointer              data)
{
  gimp_dnd_data_source_set (GIMP_DND_DATA_BRUSH, widget,
			    GTK_SIGNAL_FUNC (get_brush_func),
			    data);
}

void
gimp_dnd_brush_dest_set (GtkWidget            *widget,
			 GimpDndDropBrushFunc  set_brush_func,
			 gpointer              data)
{
  gimp_dnd_data_dest_set (GIMP_DND_DATA_BRUSH, widget,
			  GTK_SIGNAL_FUNC (set_brush_func),
			  data);
}

/***************************/
/*  pattern dnd functions  */
/***************************/

static GtkWidget *
gimp_dnd_get_pattern_icon (GtkWidget     *widget,
			   GtkSignalFunc  get_pattern_func,
			   gpointer       get_pattern_data)
{
  GtkWidget *preview;
  GPattern  *pattern;

  pattern = (* (GimpDndDragPatternFunc) get_pattern_func) (widget,
							   get_pattern_data);

  if (! pattern)
    return NULL;

  preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (preview), 
                    DRAG_PREVIEW_SIZE, DRAG_PREVIEW_SIZE);      

  draw_pattern (GTK_PREVIEW (preview), pattern,
		DRAG_PREVIEW_SIZE, DRAG_PREVIEW_SIZE);

  return preview;
}

static guchar *
gimp_dnd_get_pattern_data (GtkWidget     *widget,
			   GtkSignalFunc  get_pattern_func,
			   gpointer       get_pattern_data,
			   gint          *format,
			   gint          *length)
{
  GPattern *pattern;
  gchar *name;

  pattern = (* (GimpDndDragPatternFunc) get_pattern_func) (widget,
							   get_pattern_data);

  if (! pattern)
    return NULL;

  name = g_strdup (pattern->name);

  *format = 8;
  *length = strlen (name) + 1;

  return (guchar *) name;
}

static void
gimp_dnd_set_pattern_data (GtkWidget     *widget,
			   GtkSignalFunc  set_pattern_func,
			   gpointer       set_pattern_data,
			   guchar        *vals,
			   gint           format,
			   gint           length)
{
  GPattern *pattern;
  gchar *name;

  if ((format != 8) || (length < 1))
    {
      g_warning ("Received invalid pattern data\n");
      return;
    }

  name = (gchar *) vals;

  if (strcmp (name, "Standard") == 0)
    pattern = patterns_get_standard_pattern ();
  else
    pattern = pattern_list_get_pattern (pattern_list, name);

  if (pattern)
    (* (GimpDndDropPatternFunc) set_pattern_func) (widget, pattern,
						   set_pattern_data);
}

void
gimp_dnd_pattern_source_set (GtkWidget              *widget,
			     GimpDndDragPatternFunc  get_pattern_func,
			     gpointer                data)
{
  gimp_dnd_data_source_set (GIMP_DND_DATA_PATTERN, widget,
			    GTK_SIGNAL_FUNC (get_pattern_func),
			    data);
}

void
gimp_dnd_pattern_dest_set (GtkWidget              *widget,
			   GimpDndDropPatternFunc  set_pattern_func,
			   gpointer                data)
{
  gimp_dnd_data_dest_set (GIMP_DND_DATA_PATTERN, widget,
			  GTK_SIGNAL_FUNC (set_pattern_func),
			  data);
}

/****************************/
/*  gradient dnd functions  */
/****************************/

static GtkWidget *
gimp_dnd_get_gradient_icon (GtkWidget     *widget,
			    GtkSignalFunc  get_gradient_func,
			    gpointer       get_gradient_data)
{
  GtkWidget  *preview;
  gradient_t *gradient;

  gradient =
    (* (GimpDndDragGradientFunc) get_gradient_func) (widget, get_gradient_data);

  if (! gradient)
    return NULL;

  preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (preview), 
                    DRAG_PREVIEW_SIZE * 2, DRAG_PREVIEW_SIZE / 2);      

  draw_gradient (GTK_PREVIEW (preview), gradient,
		 DRAG_PREVIEW_SIZE * 2, DRAG_PREVIEW_SIZE / 2);

  return preview;
}

static guchar *
gimp_dnd_get_gradient_data (GtkWidget     *widget,
			    GtkSignalFunc  get_gradient_func,
			    gpointer       get_gradient_data,
			    gint          *format,
			    gint          *length)
{
  gradient_t *gradient;
  gchar *name;

  gradient =
    (* (GimpDndDragGradientFunc) get_gradient_func) (widget, get_gradient_data);

  if (! gradient)
    return NULL;

  name = g_strdup (gradient->name);

  *format = 8;
  *length = strlen (name) + 1;

  return (guchar *) name;
}

static void
gimp_dnd_set_gradient_data (GtkWidget     *widget,
			    GtkSignalFunc  set_gradient_func,
			    gpointer       set_gradient_data,
			    guchar        *vals,
			    gint           format,
			    gint           length)
{
  gradient_t *gradient;
  gchar *name;

  if ((format != 8) || (length < 1))
    {
      g_warning ("Received invalid gradient data\n");
      return;
    }

  name = (gchar *) vals;

  if (strcmp (name, "Standard") == 0)
    gradient = gradients_get_standard_gradient ();
  else
    gradient = gradient_list_get_gradient (gradients_list, name);

  if (gradient)
    (* (GimpDndDropGradientFunc) set_gradient_func) (widget, gradient,
						     set_gradient_data);
}

void
gimp_dnd_gradient_source_set (GtkWidget               *widget,
			      GimpDndDragGradientFunc  get_gradient_func,
			      gpointer                 data)
{
  gimp_dnd_data_source_set (GIMP_DND_DATA_GRADIENT, widget,
			    GTK_SIGNAL_FUNC (get_gradient_func),
			    data);
}

void
gimp_dnd_gradient_dest_set (GtkWidget               *widget,
			    GimpDndDropGradientFunc  set_gradient_func,
			    gpointer                 data)
{
  gimp_dnd_data_dest_set (GIMP_DND_DATA_GRADIENT, widget,
			  GTK_SIGNAL_FUNC (set_gradient_func),
			  data);
}

/***************************/
/*  palette dnd functions  */
/***************************/

static GtkWidget *
gimp_dnd_get_palette_icon (GtkWidget     *widget,
			   GtkSignalFunc  get_gradient_func,
			   gpointer       get_gradient_data)
{
  return NULL;
}

static guchar *
gimp_dnd_get_palette_data (GtkWidget     *widget,
			   GtkSignalFunc  get_gradient_func,
			   gpointer       get_gradient_data,
			   gint          *format,
			   gint          *length)
{
  return NULL;
}

static void
gimp_dnd_set_palette_data (GtkWidget     *widget,
			   GtkSignalFunc  set_gradient_func,
			   gpointer       set_gradient_data,
			   guchar        *vals,
			   gint           format,
			   gint           length)
{
}

void
gimp_dnd_palette_source_set (GtkWidget              *widget,
			     GimpDndDragPaletteFunc  get_palette_func,
			     gpointer                data)
{
  gimp_dnd_data_source_set (GIMP_DND_DATA_PALETTE, widget,
			    GTK_SIGNAL_FUNC (get_palette_func),
			    data);
}

void
gimp_dnd_palette_dest_set (GtkWidget              *widget,
			   GimpDndDropPaletteFunc  set_palette_func,
			   gpointer                data)
{
  gimp_dnd_data_dest_set (GIMP_DND_DATA_PALETTE, widget,
			  GTK_SIGNAL_FUNC (set_palette_func),
			  data);
}

/************************/
/*  tool dnd functions  */
/************************/

static GtkWidget *
gimp_dnd_get_tool_icon (GtkWidget     *widget,
			GtkSignalFunc  get_tool_func,
			gpointer       get_tool_data)
{
  GtkWidget *tool_icon;
  ToolType   tool_type;

  tool_type =
    (* (GimpDndDragToolFunc) get_tool_func) (widget, get_tool_data);

  if (((gint) tool_type < 0) || ((gint) tool_type >= num_tools))
    return NULL;

  tool_icon = gtk_pixmap_new (tool_get_pixmap (tool_type),
			      tool_get_mask (tool_type));

  return tool_icon;
}

static guchar *
gimp_dnd_get_tool_data (GtkWidget     *widget,
			GtkSignalFunc  get_tool_func,
			gpointer       get_tool_data,
			gint          *format,
			gint          *length)
{
  ToolType  tool_type;
  guint16  *val;

  tool_type =
    (* (GimpDndDragToolFunc) get_tool_func) (widget, get_tool_data);

  if (((gint) tool_type < 0) || ((gint) tool_type >= num_tools))
    return NULL;

  val = g_new (guint16, 1);

  val[0] = (guint16) tool_type;

  *format = 16;
  *length = 2;

  return (guchar *) val;
}

static void
gimp_dnd_set_tool_data (GtkWidget     *widget,
			GtkSignalFunc  set_tool_func,
			gpointer       set_tool_data,
			guchar        *vals,
			gint           format,
			gint           length)
{
  ToolType tool_type;
  guint16 val;

  if ((format != 16) || (length != 2))
    {
      g_warning ("Received invalid tool data\n");
      return;
    }

  val = *((guint16 *) vals);

  tool_type = (ToolType) val;

  if (((gint) tool_type < 0) || ((gint) tool_type >= num_tools))
    return;

  (* (GimpDndDropToolFunc) set_tool_func) (widget, tool_type, set_tool_data);
}

void
gimp_dnd_tool_source_set (GtkWidget           *widget,
			  GimpDndDragToolFunc  get_tool_func,
			  gpointer             data)
{
  gimp_dnd_data_source_set (GIMP_DND_DATA_TOOL, widget,
			    GTK_SIGNAL_FUNC (get_tool_func),
			    data);
}

void
gimp_dnd_tool_dest_set (GtkWidget           *widget,
			GimpDndDropToolFunc  set_tool_func,
			gpointer             data)
{
  gimp_dnd_data_dest_set (GIMP_DND_DATA_TOOL, widget,
			  GTK_SIGNAL_FUNC (set_tool_func),
			  data);
}

/****************************/
/*  drawable dnd functions  */
/****************************/

void
gimp_dnd_set_drawable_preview_icon (GtkWidget      *widget,
				    GdkDragContext *context,
				    GimpDrawable   *drawable,
				    GdkGC          *gc)
{
  GtkWidget *window;
  GtkWidget *frame;
  GtkWidget *preview;

  gboolean  drag_connected;

  TempBuf   *tmpbuf;
  gint       bpp;
  gint       x, y;
  guchar    *src;
  gdouble    r, g, b, a;
  gdouble    c0, c1;
  guchar    *p0, *p1, *even, *odd;
  gint       width;
  gint       height;
  gdouble    ratio;

  if (! preview_size)
    return;

  if (gimp_drawable_width (drawable) > gimp_drawable_height (drawable))
    ratio = (gdouble) DRAG_PREVIEW_SIZE / 
            (gdouble) (gimp_drawable_width (drawable));
  else
    ratio = (gdouble) DRAG_PREVIEW_SIZE /
            (gdouble) (gimp_drawable_height (drawable));

  width =  (gint) (ratio * gimp_drawable_width (drawable));
  height = (gint) (ratio * gimp_drawable_height (drawable));

  if (width < 1) 
    width = 1;
  if (height < 1)
    height = 1;

  preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (preview), width, height);

  if (GIMP_IS_LAYER (drawable))
    {
      tmpbuf = layer_preview (GIMP_LAYER (drawable), width, height);
    }
  else if (GIMP_IS_LAYER_MASK (drawable))
    {
      tmpbuf =
	layer_mask_preview (layer_mask_get_layer (GIMP_LAYER_MASK (drawable)),
			    width, height);
    }
  else if (GIMP_IS_CHANNEL (drawable))
    {
      tmpbuf = channel_preview (GIMP_CHANNEL (drawable), width, height);
    }
  else
    {
      gtk_widget_destroy (preview);
      return;
    }

  bpp = tmpbuf->bytes;

  /*  Draw the thumbnail with checks  */
  src = temp_buf_data (tmpbuf);

  even = g_new (guchar, width * 3);
  odd  = g_new (guchar, width * 3);
  
  for (y = 0; y < height; y++)
    {
      p0 = even;
      p1 = odd;

      for (x = 0; x < width; x++)
	{
	  if (bpp == 4)
	    {
	      r = ((gdouble) src[x*4+0]) / 255.0;
	      g = ((gdouble) src[x*4+1]) / 255.0;
	      b = ((gdouble) src[x*4+2]) / 255.0;
	      a = ((gdouble) src[x*4+3]) / 255.0;
	    }
	  else if (bpp == 3)
	    {
	      r = ((gdouble) src[x*3+0]) / 255.0;
	      g = ((gdouble) src[x*3+1]) / 255.0;
	      b = ((gdouble) src[x*3+2]) / 255.0;
	      a = 1.0;
	    }
	  else
	    {
	      r = ((gdouble) src[x*bpp+0]) / 255.0;
	      g = b = r;
	      if (bpp == 2)
		a = ((gdouble) src[x*bpp+1]) / 255.0;
	      else
		a = 1.0;
	    }

	  if ((x / GIMP_CHECK_SIZE_SM) & 1)
	    {
	      c0 = GIMP_CHECK_LIGHT;
	      c1 = GIMP_CHECK_DARK;
	    }
	  else
	    {
	      c0 = GIMP_CHECK_DARK;
	      c1 = GIMP_CHECK_LIGHT;
	    }

	  *p0++ = (c0 + (r - c0) * a) * 255.0;
	  *p0++ = (c0 + (g - c0) * a) * 255.0;
	  *p0++ = (c0 + (b - c0) * a) * 255.0;

	  *p1++ = (c1 + (r - c1) * a) * 255.0;
	  *p1++ = (c1 + (g - c1) * a) * 255.0;
	  *p1++ = (c1 + (b - c1) * a) * 255.0;

	}

      if ((y / GIMP_CHECK_SIZE_SM) & 1)
	{
	  gtk_preview_draw_row (GTK_PREVIEW (preview), odd,
				0, y, width);
	}
      else
	{
	  gtk_preview_draw_row (GTK_PREVIEW (preview), even,
				0, y, width);
	}
      src += width * bpp;
    }

  g_free (even);
  g_free (odd);

  window = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_widget_realize (window);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (window), frame);
  gtk_widget_show (frame);

  gtk_container_add (GTK_CONTAINER (frame), preview);
  gtk_widget_show (preview);

  drag_connected =
    (gboolean) gtk_object_get_data (GTK_OBJECT (widget),
				    "gimp_dnd_drag_connected");

  if (! drag_connected)
    {
      gtk_signal_connect (GTK_OBJECT (widget), "drag_end",
			  GTK_SIGNAL_FUNC (gimp_dnd_data_drag_end),
			  NULL);

      gtk_object_set_data (GTK_OBJECT (widget), "gimp_dnd_drag_connected",
			   (gpointer) TRUE);
    }

  gtk_object_set_data_full (GTK_OBJECT (widget),
			    "gimp-dnd-data-widget",
			    window,
			    (GtkDestroyNotify) gtk_widget_destroy);

  gtk_drag_set_icon_widget (context, window,
			    DRAG_ICON_OFFSET, DRAG_ICON_OFFSET);
}
