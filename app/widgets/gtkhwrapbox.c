/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * GtkHWrapBox: Horizontal wrapping box widget
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

#include "gtkhwrapbox.h"

#include "libgimpmath/gimpmath.h"


/* --- prototypes --- */
static void    gtk_hwrap_box_class_init    (GtkHWrapBoxClass   *klass);
static void    gtk_hwrap_box_init          (GtkHWrapBox        *hwbox);
static void    gtk_hwrap_box_size_request  (GtkWidget          *widget,
					    GtkRequisition     *requisition);
static void    gtk_hwrap_box_size_allocate (GtkWidget          *widget,
					    GtkAllocation      *allocation);
static GSList* reverse_list_row_children   (GtkWrapBox         *wbox,
					    GtkWrapBoxChild   **child_p,
					    GtkAllocation      *area,
					    guint              *max_height,
					    gboolean           *can_vexpand);


/* --- variables --- */
static gpointer parent_class = NULL;


/* --- functions --- */
GType
gtk_hwrap_box_get_type (void)
{
  static GType hwrap_box_type = 0;
  
  if (!hwrap_box_type)
    {
      static const GTypeInfo hwrap_box_info =
      {
	sizeof (GtkHWrapBoxClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) gtk_hwrap_box_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (GtkHWrapBox),
	0,		/* n_preallocs */
	(GInstanceInitFunc) gtk_hwrap_box_init,
      };
      
      hwrap_box_type = g_type_register_static (GTK_TYPE_WRAP_BOX, "GtkHWrapBox",
					       &hwrap_box_info, 0);
    }
  
  return hwrap_box_type;
}

static void
gtk_hwrap_box_class_init (GtkHWrapBoxClass *class)
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
  
  widget_class->size_request = gtk_hwrap_box_size_request;
  widget_class->size_allocate = gtk_hwrap_box_size_allocate;

  wrap_box_class->rlist_line_children = reverse_list_row_children;
}

static void
gtk_hwrap_box_init (GtkHWrapBox *hwbox)
{
  hwbox->max_child_width = 0;
  hwbox->max_child_height = 0;
}

GtkWidget*
gtk_hwrap_box_new (gboolean homogeneous)
{
  return g_object_new (GTK_TYPE_HWRAP_BOX, "homogeneous", homogeneous, NULL);
}

static inline void
get_child_requisition (GtkWrapBox     *wbox,
		       GtkWidget      *child,
		       GtkRequisition *child_requisition)
{
  if (wbox->homogeneous)
    {
      GtkHWrapBox *hwbox = GTK_HWRAP_BOX (wbox);
      
      child_requisition->width = hwbox->max_child_width;
      child_requisition->height = hwbox->max_child_height;
    }
  else
    gtk_widget_get_child_requisition (child, child_requisition);
}

static gfloat
get_layout_size (GtkHWrapBox *this,
		 guint        max_width,
		 guint       *width_inc)
{
  GtkWrapBox *wbox = GTK_WRAP_BOX (this);
  GtkWrapBoxChild *child;
  guint n_rows, left_over = 0, total_height = 0;
  gboolean last_row_filled = TRUE;

  *width_inc = this->max_child_width + 1;

  n_rows = 0;
  for (child = wbox->children; child; child = child->next)
    {
      GtkWrapBoxChild *row_child;
      GtkRequisition child_requisition;
      guint row_width, row_height, n = 1;

      if (!GTK_WIDGET_VISIBLE (child->widget))
	continue;

      get_child_requisition (wbox, child->widget, &child_requisition);
      if (!last_row_filled)
	*width_inc = MIN (*width_inc, child_requisition.width - left_over);
      row_width = child_requisition.width;
      row_height = child_requisition.height;
      for (row_child = child->next; row_child && n < wbox->child_limit; row_child = row_child->next)
	{
	  if (GTK_WIDGET_VISIBLE (row_child->widget))
	    {
	      get_child_requisition (wbox, row_child->widget, &child_requisition);
	      if (row_width + wbox->hspacing + child_requisition.width > max_width)
		break;
	      row_width += wbox->hspacing + child_requisition.width;
	      row_height = MAX (row_height, child_requisition.height);
	      n++;
	    }
	  child = row_child;
	}
      last_row_filled = n >= wbox->child_limit;
      left_over = last_row_filled ? 0 : max_width - (row_width + wbox->hspacing);
      total_height += (n_rows ? wbox->vspacing : 0) + row_height;
      n_rows++;
    }

  if (*width_inc > this->max_child_width)
    *width_inc = 0;
  
  return MAX (total_height, 1);
}

static void
gtk_hwrap_box_size_request (GtkWidget      *widget,
			    GtkRequisition *requisition)
{
  GtkHWrapBox *this = GTK_HWRAP_BOX (widget);
  GtkWrapBox *wbox = GTK_WRAP_BOX (widget);
  GtkWrapBoxChild *child;
  gfloat ratio_dist, layout_width = 0;
  guint row_inc = 0;
  
  g_return_if_fail (requisition != NULL);
  
  requisition->width = 0;
  requisition->height = 0;
  this->max_child_width = 0;
  this->max_child_height = 0;

  /* size_request all children */
  for (child = wbox->children; child; child = child->next)
    if (GTK_WIDGET_VISIBLE (child->widget))
      {
	GtkRequisition child_requisition;
	
	gtk_widget_size_request (child->widget, &child_requisition);

	this->max_child_width = MAX (this->max_child_width, child_requisition.width);
	this->max_child_height = MAX (this->max_child_height, child_requisition.height);
      }

  /* figure all possible layouts */
  ratio_dist = 32768;
  layout_width = this->max_child_width;
  do
    {
      gfloat layout_height;
      gfloat ratio, dist;

      layout_width += row_inc;
      layout_height = get_layout_size (this, layout_width, &row_inc);
      ratio = layout_width / layout_height;		/*<h2v-skip>*/
      dist = MAX (ratio, wbox->aspect_ratio) - MIN (ratio, wbox->aspect_ratio);
      if (dist < ratio_dist)
	{
	  ratio_dist = dist;
	  requisition->width = layout_width;
	  requisition->height = layout_height;
	}
      
      /* g_print ("ratio for width %d height %d = %f\n",
	 (gint) layout_width,
	 (gint) layout_height,
	 ratio);
      */
    }
  while (row_inc);

  requisition->width += GTK_CONTAINER (wbox)->border_width * 2; /*<h2v-skip>*/
  requisition->height += GTK_CONTAINER (wbox)->border_width * 2; /*<h2v-skip>*/
  /* g_print ("choosen: width %d, height %d\n",
     requisition->width,
     requisition->height);
  */
}

static GSList*
reverse_list_row_children (GtkWrapBox       *wbox,
			   GtkWrapBoxChild **child_p,
			   GtkAllocation    *area,
			   guint            *max_child_size,
			   gboolean         *expand_line)
{
  GSList *slist = NULL;
  guint width = 0, row_width = area->width;
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
      width += child_requisition.width;
      *max_child_size = MAX (*max_child_size, child_requisition.height);
      *expand_line |= child->vexpand;
      slist = g_slist_prepend (slist, child);
      *child_p = child->next;
      child = *child_p;
      
      while (child && n < wbox->child_limit)
	{
	  if (GTK_WIDGET_VISIBLE (child->widget))
	    {
	      get_child_requisition (wbox, child->widget, &child_requisition);
	      if (width + wbox->hspacing + child_requisition.width > row_width ||
		  child->wrapped)
		break;
	      width += wbox->hspacing + child_requisition.width;
	      *max_child_size = MAX (*max_child_size, child_requisition.height);
	      *expand_line |= child->vexpand;
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
layout_row (GtkWrapBox    *wbox,
	    GtkAllocation *area,
	    GSList        *children,
	    guint          children_per_line,
	    gboolean       vexpand)
{
  GSList *slist;
  guint n_children = 0, n_expand_children = 0, have_expand_children = 0;
  gint total_width = 0;
  gfloat x, width, extra;
  GtkAllocation child_allocation;
  
  for (slist = children; slist; slist = slist->next)
    {
      GtkWrapBoxChild *child = slist->data;
      GtkRequisition child_requisition;
      
      n_children++;
      if (child->hexpand)
	n_expand_children++;
      
      get_child_requisition (wbox, child->widget, &child_requisition);
      total_width += child_requisition.width;
    }
  
  width = MAX (1, area->width - (n_children - 1) * wbox->hspacing);
  if (width > total_width)
    extra = width - total_width;
  else
    extra = 0;
  have_expand_children = n_expand_children && extra;
  
  x = area->x;
  if (wbox->homogeneous)
    {
      width = MAX (1, area->width - (children_per_line - 1) * wbox->hspacing);
      width /= ((gdouble) children_per_line);
      extra = 0;
    }
  else if (have_expand_children && wbox->justify != GTK_JUSTIFY_FILL)
    {
      width = extra;
      extra /= ((gdouble) n_expand_children);
    }
  else
    {
      if (wbox->justify == GTK_JUSTIFY_FILL)
	{
	  width = extra;
	  have_expand_children = TRUE;
	  n_expand_children = n_children;
	  extra /= ((gdouble) n_expand_children);
	}
      else if (wbox->justify == GTK_JUSTIFY_CENTER)
	{
	  x += extra / 2;
	  width = 0;
	  extra = 0;
	}
      else if (wbox->justify == GTK_JUSTIFY_LEFT)
	{
	  width = 0;
	  extra = 0;
	}
      else if (wbox->justify == GTK_JUSTIFY_RIGHT)
	{
	  x += extra;
	  width = 0;
	  extra = 0;
	}
    }
  
  n_children = 0;
  for (slist = children; slist; slist = slist->next)
    {
      GtkWrapBoxChild *child = slist->data;
      
      child_allocation.x = x;
      child_allocation.y = area->y;
      if (wbox->homogeneous)
	{
	  child_allocation.height = area->height;
	  child_allocation.width = width;
	  x += child_allocation.width + wbox->hspacing;
	}
      else
	{
	  GtkRequisition child_requisition;
	  
	  get_child_requisition (wbox, child->widget, &child_requisition);
	  
	  if (child_requisition.height >= area->height)
	    child_allocation.height = area->height;
	  else
	    {
	      child_allocation.height = child_requisition.height;
	      if (wbox->line_justify == GTK_JUSTIFY_FILL || child->vfill)
		child_allocation.height = area->height;
	      else if (child->vexpand || wbox->line_justify == GTK_JUSTIFY_CENTER)
		child_allocation.y += (area->height - child_requisition.height) / 2;
	      else if (wbox->line_justify == GTK_JUSTIFY_BOTTOM)
		child_allocation.y += area->height - child_requisition.height;
	    }
	  
	  if (have_expand_children)
	    {
	      child_allocation.width = child_requisition.width;
	      if (child->hexpand || wbox->justify == GTK_JUSTIFY_FILL)
		{
		  guint space;
		  
		  n_expand_children--;
		  space = extra * n_expand_children;
		  space = width - space;
		  width -= space;
		  if (child->hfill)
		    child_allocation.width += space;
		  else
		    {
		      child_allocation.x += space / 2;
		      x += space;
		    }
		}
	    }
	  else
	    {
	      /* g_print ("child_allocation.x %d += %d * %f ",
		       child_allocation.x, n_children, extra); */
	      child_allocation.x += n_children * extra;
	      /* g_print ("= %d\n",
		       child_allocation.x); */
	      child_allocation.width = MIN (child_requisition.width,
					    area->width - child_allocation.x + area->x);
	    }
	}
      
      x += child_allocation.width + wbox->hspacing;
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
layout_rows (GtkWrapBox    *wbox,
	     GtkAllocation *area)
{
  GtkWrapBoxChild *next_child;
  guint min_height;
  gboolean vexpand;
  GSList *slist;
  Line *line_list = NULL;
  guint total_height = 0, n_expand_lines = 0, n_lines = 0;
  gfloat shrink_height;
  guint children_per_line;
  
  next_child = wbox->children;
  slist = GTK_WRAP_BOX_GET_CLASS (wbox)->rlist_line_children (wbox,
							      &next_child,
							      area,
							      &min_height,
							      &vexpand);
  slist = g_slist_reverse (slist);

  children_per_line = g_slist_length (slist);
  while (slist)
    {
      Line *line = g_new (Line, 1);
      
      line->children = slist;
      line->min_size = min_height;
      total_height += min_height;
      line->expand = vexpand;
      if (vexpand)
	n_expand_lines++;
      line->next = line_list;
      line_list = line;
      n_lines++;
      
      slist = GTK_WRAP_BOX_GET_CLASS (wbox)->rlist_line_children (wbox,
								  &next_child,
								  area,
								  &min_height,
								  &vexpand);
      slist = g_slist_reverse (slist);
    }
  
  if (total_height > area->height)
    shrink_height = total_height - area->height;
  else
    shrink_height = 0;
  
  if (1) /* reverse lines and shrink */
    {
      Line *prev = NULL, *last = NULL;
      gfloat n_shrink_lines = n_lines;
      
      while (line_list)
	{
	  Line *tmp = line_list->next;

	  if (shrink_height)
	    {
	      Line *line = line_list;
	      guint shrink_fract = shrink_height / n_shrink_lines + 0.5;

	      if (line->min_size > shrink_fract)
		{
		  shrink_height -= shrink_fract;
		  line->min_size -= shrink_fract;
		}
	      else
		{
		  shrink_height -= line->min_size - 1;
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
      gfloat y, height, extra = 0;
      
      height = area->height;
      height = MAX (n_lines, height - (n_lines - 1) * wbox->vspacing);
      
      if (wbox->homogeneous)
	height /= ((gdouble) n_lines);
      else if (n_expand_lines)
	{
	  height = MAX (0, height - total_height);
	  extra = height / ((gdouble) n_expand_lines);
	}
      else
	height = 0;
      
      y = area->y;
      line = line_list;
      while (line)
	{
	  GtkAllocation row_allocation;
	  Line *next_line = line->next;
	  
	  row_allocation.x = area->x;
	  row_allocation.width = area->width;
	  if (wbox->homogeneous)
	    row_allocation.height = height;
	  else
	    {
	      row_allocation.height = line->min_size;
	      
	      if (line->expand)
		row_allocation.height += extra;
	    }
	  
	  row_allocation.y = y;
	  
	  y += row_allocation.height + wbox->vspacing;
	  layout_row (wbox,
		      &row_allocation,
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
gtk_hwrap_box_size_allocate (GtkWidget     *widget,
			     GtkAllocation *allocation)
{
  GtkWrapBox *wbox = GTK_WRAP_BOX (widget);
  GtkAllocation area;
  gint border = GTK_CONTAINER (wbox)->border_width; /*<h2v-skip>*/
  
  widget->allocation = *allocation;
  area.x = allocation->x + border;
  area.y = allocation->y + border;
  area.width = MAX (1, (gint) allocation->width - border * 2);
  area.height = MAX (1, (gint) allocation->height - border * 2);
  
  /*<h2v-off>*/
  /* g_print ("got: width %d, height %d\n",
     allocation->width,
     allocation->height);
  */
  /*<h2v-on>*/
  
  layout_rows (wbox, &area);
}
