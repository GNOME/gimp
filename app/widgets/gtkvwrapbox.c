/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * GtkVWrapBox: Vertical wrapping box widget
 * Copyright (C) 1999 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include "gtkvwrapbox.h"

#include "libgimpmath/gimpmath.h"


/* --- prototypes --- */
static void    gtk_vwrap_box_class_init    (GtkVWrapBoxClass   *klass);
static void    gtk_vwrap_box_init          (GtkVWrapBox        *vwbox);
static void    gtk_vwrap_box_size_request  (GtkWidget          *widget,
					    GtkRequisition     *requisition);
static void    gtk_vwrap_box_size_allocate (GtkWidget          *widget,
					    GtkAllocation      *allocation);
static GSList* reverse_list_col_children   (GtkWrapBox         *wbox,
					    GtkWrapBoxChild   **child_p,
					    GtkAllocation      *area,
					    guint              *max_width,
					    gboolean           *can_hexpand);


/* --- variables --- */
static gpointer parent_class = NULL;


/* --- functions --- */
GType
gtk_vwrap_box_get_type (void)
{
  static GType vwrap_box_type = 0;
  
  if (!vwrap_box_type)
    {
      static const GTypeInfo vwrap_box_info =
      {
	sizeof (GtkVWrapBoxClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) gtk_vwrap_box_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (GtkVWrapBox),
	0,		/* n_preallocs */
	(GInstanceInitFunc) gtk_vwrap_box_init,
      };
      
      vwrap_box_type = g_type_register_static (GTK_TYPE_WRAP_BOX, "GtkVWrapBox",
					       &vwrap_box_info, 0);
    }
  
  return vwrap_box_type;
}

static void
gtk_vwrap_box_class_init (GtkVWrapBoxClass *class)
{
  GObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkContainerClass *container_class;
  GtkWrapBoxClass *wrap_box_class;
  
  object_class = G_OBJECT_CLASS (class);
  widget_class = GTK_WIDGET_CLASS (class);
  container_class = GTK_CONTAINER_CLASS (class);
  wrap_box_class = GTK_WRAP_BOX_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  widget_class->size_request = gtk_vwrap_box_size_request;
  widget_class->size_allocate = gtk_vwrap_box_size_allocate;

  wrap_box_class->rlist_line_children = reverse_list_col_children;
}

static void
gtk_vwrap_box_init (GtkVWrapBox *vwbox)
{
  vwbox->max_child_height = 0;
  vwbox->max_child_width = 0;
}

GtkWidget*
gtk_vwrap_box_new (gboolean homogeneous)
{
  return g_object_new (GTK_TYPE_VWRAP_BOX, "homogeneous", homogeneous, NULL);
}

static inline void
get_child_requisition (GtkWrapBox     *wbox,
		       GtkWidget      *child,
		       GtkRequisition *child_requisition)
{
  if (wbox->homogeneous)
    {
      GtkVWrapBox *vwbox = GTK_VWRAP_BOX (wbox);
      
      child_requisition->height = vwbox->max_child_height;
      child_requisition->width = vwbox->max_child_width;
    }
  else
    gtk_widget_get_child_requisition (child, child_requisition);
}

static gfloat
get_layout_size (GtkVWrapBox *this,
		 guint        max_height,
		 guint       *height_inc)
{
  GtkWrapBox *wbox = GTK_WRAP_BOX (this);
  GtkWrapBoxChild *child;
  guint n_cols, left_over = 0, total_width = 0;
  gboolean last_col_filled = TRUE;

  *height_inc = this->max_child_height + 1;

  n_cols = 0;
  for (child = wbox->children; child; child = child->next)
    {
      GtkWrapBoxChild *col_child;
      GtkRequisition child_requisition;
      guint col_height, col_width, n = 1;

      if (!GTK_WIDGET_VISIBLE (child->widget))
	continue;

      get_child_requisition (wbox, child->widget, &child_requisition);
      if (!last_col_filled)
	*height_inc = MIN (*height_inc, child_requisition.height - left_over);
      col_height = child_requisition.height;
      col_width = child_requisition.width;
      for (col_child = child->next; col_child && n < wbox->child_limit; col_child = col_child->next)
	{
	  if (GTK_WIDGET_VISIBLE (col_child->widget))
	    {
	      get_child_requisition (wbox, col_child->widget, &child_requisition);
	      if (col_height + wbox->vspacing + child_requisition.height > max_height)
		break;
	      col_height += wbox->vspacing + child_requisition.height;
	      col_width = MAX (col_width, child_requisition.width);
	      n++;
	    }
	  child = col_child;
	}
      last_col_filled = n >= wbox->child_limit;
      left_over = last_col_filled ? 0 : max_height - (col_height + wbox->vspacing);
      total_width += (n_cols ? wbox->hspacing : 0) + col_width;
      n_cols++;
    }

  if (*height_inc > this->max_child_height)
    *height_inc = 0;
  
  return MAX (total_width, 1);
}

static void
gtk_vwrap_box_size_request (GtkWidget      *widget,
			    GtkRequisition *requisition)
{
  GtkVWrapBox *this = GTK_VWRAP_BOX (widget);
  GtkWrapBox *wbox = GTK_WRAP_BOX (widget);
  GtkWrapBoxChild *child;
  gfloat ratio_dist, layout_height = 0;
  guint col_inc = 0;
  
  g_return_if_fail (requisition != NULL);
  
  requisition->height = 0;
  requisition->width = 0;
  this->max_child_height = 0;
  this->max_child_width = 0;

  /* size_request all children */
  for (child = wbox->children; child; child = child->next)
    if (GTK_WIDGET_VISIBLE (child->widget))
      {
	GtkRequisition child_requisition;
	
	gtk_widget_size_request (child->widget, &child_requisition);

	this->max_child_height = MAX (this->max_child_height, child_requisition.height);
	this->max_child_width = MAX (this->max_child_width, child_requisition.width);
      }

  /* figure all possible layouts */
  ratio_dist = 32768;
  layout_height = this->max_child_height;
  do
    {
      gfloat layout_width;
      gfloat ratio, dist;

      layout_height += col_inc;
      layout_width = get_layout_size (this, layout_height, &col_inc);
      ratio = layout_width / layout_height;		/*<h2v-skip>*/
      dist = MAX (ratio, wbox->aspect_ratio) - MIN (ratio, wbox->aspect_ratio);
      if (dist < ratio_dist)
	{
	  ratio_dist = dist;
	  requisition->height = layout_height;
	  requisition->width = layout_width;
	}
      
      /* g_print ("ratio for height %d width %d = %f\n",
	 (gint) layout_height,
	 (gint) layout_width,
	 ratio);
      */
    }
  while (col_inc);

  requisition->width += GTK_CONTAINER (wbox)->border_width * 2; /*<h2v-skip>*/
  requisition->height += GTK_CONTAINER (wbox)->border_width * 2; /*<h2v-skip>*/
  /* g_print ("choosen: height %d, width %d\n",
     requisition->height,
     requisition->width);
  */
}

static GSList*
reverse_list_col_children (GtkWrapBox       *wbox,
			   GtkWrapBoxChild **child_p,
			   GtkAllocation    *area,
			   guint            *max_child_size,
			   gboolean         *expand_line)
{
  GSList *slist = NULL;
  guint height = 0, col_height = area->height;
  GtkWrapBoxChild *child = *child_p;
  
  *max_child_size = 0;
  *expand_line = FALSE;
  
  while (child && !GTK_WIDGET_VISIBLE (child->widget))
    {
      *child_p = child->next;
      child = *child_p;
    }
  
  if (child)
    {
      GtkRequisition child_requisition;
      guint n = 1;
      
      get_child_requisition (wbox, child->widget, &child_requisition);
      height += child_requisition.height;
      *max_child_size = MAX (*max_child_size, child_requisition.width);
      *expand_line |= child->hexpand;
      slist = g_slist_prepend (slist, child);
      *child_p = child->next;
      child = *child_p;
      
      while (child && n < wbox->child_limit)
	{
	  if (GTK_WIDGET_VISIBLE (child->widget))
	    {
	      get_child_requisition (wbox, child->widget, &child_requisition);
	      if (height + wbox->vspacing + child_requisition.height > col_height ||
		  child->wrapped)
		break;
	      height += wbox->vspacing + child_requisition.height;
	      *max_child_size = MAX (*max_child_size, child_requisition.width);
	      *expand_line |= child->hexpand;
	      slist = g_slist_prepend (slist, child);
	      n++;
	    }
	  *child_p = child->next;
	  child = *child_p;
	}
    }
  
  return slist;
}

static void
layout_col (GtkWrapBox    *wbox,
	    GtkAllocation *area,
	    GSList        *children,
	    guint          children_per_line,
	    gboolean       hexpand)
{
  GSList *slist;
  guint n_children = 0, n_expand_children = 0, have_expand_children = 0;
  gint total_height = 0;
  gfloat y, height, extra;
  GtkAllocation child_allocation;
  
  for (slist = children; slist; slist = slist->next)
    {
      GtkWrapBoxChild *child = slist->data;
      GtkRequisition child_requisition;
      
      n_children++;
      if (child->vexpand)
	n_expand_children++;
      
      get_child_requisition (wbox, child->widget, &child_requisition);
      total_height += child_requisition.height;
    }
  
  height = MAX (1, area->height - (n_children - 1) * wbox->vspacing);
  if (height > total_height)
    extra = height - total_height;
  else
    extra = 0;
  have_expand_children = n_expand_children && extra;
  
  y = area->y;
  if (wbox->homogeneous)
    {
      height = MAX (1, area->height - (children_per_line - 1) * wbox->vspacing);
      height /= ((gdouble) children_per_line);
      extra = 0;
    }
  else if (have_expand_children && wbox->justify != GTK_JUSTIFY_FILL)
    {
      height = extra;
      extra /= ((gdouble) n_expand_children);
    }
  else
    {
      if (wbox->justify == GTK_JUSTIFY_FILL)
	{
	  height = extra;
	  have_expand_children = TRUE;
	  n_expand_children = n_children;
	  extra /= ((gdouble) n_expand_children);
	}
      else if (wbox->justify == GTK_JUSTIFY_CENTER)
	{
	  y += extra / 2;
	  height = 0;
	  extra = 0;
	}
      else if (wbox->justify == GTK_JUSTIFY_LEFT)
	{
	  height = 0;
	  extra = 0;
	}
      else if (wbox->justify == GTK_JUSTIFY_RIGHT)
	{
	  y += extra;
	  height = 0;
	  extra = 0;
	}
    }
  
  n_children = 0;
  for (slist = children; slist; slist = slist->next)
    {
      GtkWrapBoxChild *child = slist->data;
      
      child_allocation.y = y;
      child_allocation.x = area->x;
      if (wbox->homogeneous)
	{
	  child_allocation.width = area->width;
	  child_allocation.height = height;
	  y += child_allocation.height + wbox->vspacing;
	}
      else
	{
	  GtkRequisition child_requisition;
	  
	  get_child_requisition (wbox, child->widget, &child_requisition);
	  
	  if (child_requisition.width >= area->width)
	    child_allocation.width = area->width;
	  else
	    {
	      child_allocation.width = child_requisition.width;
	      if (wbox->line_justify == GTK_JUSTIFY_FILL || child->hfill)
		child_allocation.width = area->width;
	      else if (child->hexpand || wbox->line_justify == GTK_JUSTIFY_CENTER)
		child_allocation.x += (area->width - child_requisition.width) / 2;
	      else if (wbox->line_justify == GTK_JUSTIFY_BOTTOM)
		child_allocation.x += area->width - child_requisition.width;
	    }
	  
	  if (have_expand_children)
	    {
	      child_allocation.height = child_requisition.height;
	      if (child->vexpand || wbox->justify == GTK_JUSTIFY_FILL)
		{
		  guint space;
		  
		  n_expand_children--;
		  space = extra * n_expand_children;
		  space = height - space;
		  height -= space;
		  if (child->vfill)
		    child_allocation.height += space;
		  else
		    {
		      child_allocation.y += space / 2;
		      y += space;
		    }
		}
	    }
	  else
	    {
	      /* g_print ("child_allocation.y %d += %d * %f ",
		       child_allocation.y, n_children, extra); */
	      child_allocation.y += n_children * extra;
	      /* g_print ("= %d\n",
		       child_allocation.y); */
	      child_allocation.height = MIN (child_requisition.height,
					    area->height - child_allocation.y + area->y);
	    }
	}
      
      y += child_allocation.height + wbox->vspacing;
      gtk_widget_size_allocate (child->widget, &child_allocation);
      n_children++;
    }
}

typedef struct _Line Line;
struct _Line
{
  GSList  *children;
  guint16  min_size;
  guint    expand : 1;
  Line     *next;
};

static void
layout_cols (GtkWrapBox    *wbox,
	     GtkAllocation *area)
{
  GtkWrapBoxChild *next_child;
  guint min_width;
  gboolean hexpand;
  GSList *slist;
  Line *line_list = NULL;
  guint total_width = 0, n_expand_lines = 0, n_lines = 0;
  gfloat shrink_width;
  guint children_per_line;
  
  next_child = wbox->children;
  slist = GTK_WRAP_BOX_GET_CLASS (wbox)->rlist_line_children (wbox,
							      &next_child,
							      area,
							      &min_width,
							      &hexpand);
  slist = g_slist_reverse (slist);

  children_per_line = g_slist_length (slist);
  while (slist)
    {
      Line *line = g_new (Line, 1);
      
      line->children = slist;
      line->min_size = min_width;
      total_width += min_width;
      line->expand = hexpand;
      if (hexpand)
	n_expand_lines++;
      line->next = line_list;
      line_list = line;
      n_lines++;
      
      slist = GTK_WRAP_BOX_GET_CLASS (wbox)->rlist_line_children (wbox,
								  &next_child,
								  area,
								  &min_width,
								  &hexpand);
      slist = g_slist_reverse (slist);
    }
  
  if (total_width > area->width)
    shrink_width = total_width - area->width;
  else
    shrink_width = 0;
  
  if (1) /* reverse lines and shrink */
    {
      Line *prev = NULL, *last = NULL;
      gfloat n_shrink_lines = n_lines;
      
      while (line_list)
	{
	  Line *tmp = line_list->next;

	  if (shrink_width)
	    {
	      Line *line = line_list;
	      guint shrink_fract = shrink_width / n_shrink_lines + 0.5;

	      if (line->min_size > shrink_fract)
		{
		  shrink_width -= shrink_fract;
		  line->min_size -= shrink_fract;
		}
	      else
		{
		  shrink_width -= line->min_size - 1;
		  line->min_size = 1;
		}
	    }
	  n_shrink_lines--;

	  last = line_list;
	  line_list->next = prev;
	  prev = line_list;
	  line_list = tmp;
	}
      line_list = last;
    }
  
  if (n_lines)
    {
      Line *line;
      gfloat x, width, extra = 0;
      
      width = area->width;
      width = MAX (n_lines, width - (n_lines - 1) * wbox->hspacing);
      
      if (wbox->homogeneous)
	width /= ((gdouble) n_lines);
      else if (n_expand_lines)
	{
	  width = MAX (0, width - total_width);
	  extra = width / ((gdouble) n_expand_lines);
	}
      else
	width = 0;
      
      x = area->x;
      line = line_list;
      while (line)
	{
	  GtkAllocation col_allocation;
	  Line *next_line = line->next;
	  
	  col_allocation.y = area->y;
	  col_allocation.height = area->height;
	  if (wbox->homogeneous)
	    col_allocation.width = width;
	  else
	    {
	      col_allocation.width = line->min_size;
	      
	      if (line->expand)
		col_allocation.width += extra;
	    }
	  
	  col_allocation.x = x;
	  
	  x += col_allocation.width + wbox->hspacing;
	  layout_col (wbox,
		      &col_allocation,
		      line->children,
		      children_per_line,
		      line->expand);

	  g_slist_free (line->children);
	  g_free (line);
	  line = next_line;
	}
    }
}

static void
gtk_vwrap_box_size_allocate (GtkWidget     *widget,
			     GtkAllocation *allocation)
{
  GtkWrapBox *wbox = GTK_WRAP_BOX (widget);
  GtkAllocation area;
  gint border = GTK_CONTAINER (wbox)->border_width; /*<h2v-skip>*/
  
  widget->allocation = *allocation;
  area.y = allocation->y + border;
  area.x = allocation->x + border;
  area.height = MAX (1, (gint) allocation->height - border * 2);
  area.width = MAX (1, (gint) allocation->width - border * 2);
  
  /*<h2v-off>*/
  /* g_print ("got: width %d, height %d\n",
     allocation->width,
     allocation->height);
  */
  /*<h2v-on>*/
  
  layout_cols (wbox, &area);
}
