/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * GtkWrapBox: Wrapping box widget
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
#include <math.h>

#include "gtkwrapbox.h"


/* --- arguments --- */
enum {
  ARG_0,
  ARG_HOMOGENEOUS,
  ARG_JUSTIFY,
  ARG_HSPACING,
  ARG_VSPACING,
  ARG_LINE_JUSTIFY,
  ARG_ASPECT_RATIO,
  ARG_CURRENT_RATIO,
  ARG_CHILD_LIMIT
};
enum {
  CHILD_ARG_0,
  CHILD_ARG_POSITION,
  CHILD_ARG_HEXPAND,
  CHILD_ARG_HFILL,
  CHILD_ARG_VEXPAND,
  CHILD_ARG_VFILL
};


/* --- prototypes --- */
static void gtk_wrap_box_class_init    (GtkWrapBoxClass    *klass);
static void gtk_wrap_box_init          (GtkWrapBox         *wbox);
static void gtk_wrap_box_get_arg       (GtkObject          *object,
					GtkArg             *arg,
					guint               arg_id);
static void gtk_wrap_box_set_arg       (GtkObject          *object,
					GtkArg             *arg,
					guint               arg_id);
static void gtk_wrap_box_set_child_arg (GtkContainer       *container,
					GtkWidget          *child,
					GtkArg             *arg,
					guint               arg_id);
static void gtk_wrap_box_get_child_arg (GtkContainer       *container,
					GtkWidget          *child,
					GtkArg             *arg,
					guint               arg_id);
static void gtk_wrap_box_map           (GtkWidget          *widget);
static void gtk_wrap_box_unmap         (GtkWidget          *widget);
static void gtk_wrap_box_draw          (GtkWidget          *widget,
					GdkRectangle       *area);
static gint gtk_wrap_box_expose        (GtkWidget          *widget,
					GdkEventExpose     *event);
static void gtk_wrap_box_add           (GtkContainer       *container,
					GtkWidget          *widget);
static void gtk_wrap_box_remove        (GtkContainer       *container,
					GtkWidget          *widget);
static void gtk_wrap_box_forall        (GtkContainer       *container,
					gboolean            include_internals,
					GtkCallback         callback,
					gpointer            callback_data);
static GtkType gtk_wrap_box_child_type (GtkContainer       *container);


/* --- variables --- */
static gpointer parent_class = NULL;


/* --- functions --- */
GtkType
gtk_wrap_box_get_type (void)
{
  static GtkType wrap_box_type = 0;
  
  if (!wrap_box_type)
    {
      static const GtkTypeInfo wrap_box_info =
      {
	"GtkWrapBox",
	sizeof (GtkWrapBox),
	sizeof (GtkWrapBoxClass),
	(GtkClassInitFunc) gtk_wrap_box_class_init,
	(GtkObjectInitFunc) gtk_wrap_box_init,
        /* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };
      
      wrap_box_type = gtk_type_unique (GTK_TYPE_CONTAINER, &wrap_box_info);
    }
  
  return wrap_box_type;
}

static void
gtk_wrap_box_class_init (GtkWrapBoxClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkContainerClass *container_class;
  
  object_class = GTK_OBJECT_CLASS (class);
  widget_class = GTK_WIDGET_CLASS (class);
  container_class = GTK_CONTAINER_CLASS (class);
  
  parent_class = gtk_type_class (GTK_TYPE_CONTAINER);
  
  object_class->set_arg = gtk_wrap_box_set_arg;
  object_class->get_arg = gtk_wrap_box_get_arg;
  
  widget_class->map = gtk_wrap_box_map;
  widget_class->unmap = gtk_wrap_box_unmap;
  widget_class->draw = gtk_wrap_box_draw;
  widget_class->expose_event = gtk_wrap_box_expose;
  
  container_class->add = gtk_wrap_box_add;
  container_class->remove = gtk_wrap_box_remove;
  container_class->forall = gtk_wrap_box_forall;
  container_class->child_type = gtk_wrap_box_child_type;
  container_class->set_child_arg = gtk_wrap_box_set_child_arg;
  container_class->get_child_arg = gtk_wrap_box_get_child_arg;

  class->rlist_line_children = NULL;
  
  gtk_object_add_arg_type ("GtkWrapBox::homogeneous",
			   GTK_TYPE_BOOL, GTK_ARG_READWRITE, ARG_HOMOGENEOUS);
  gtk_object_add_arg_type ("GtkWrapBox::justify",
			   GTK_TYPE_JUSTIFICATION, GTK_ARG_READWRITE, ARG_JUSTIFY);
  gtk_object_add_arg_type ("GtkWrapBox::hspacing",
			   GTK_TYPE_UINT, GTK_ARG_READWRITE, ARG_HSPACING);
  gtk_object_add_arg_type ("GtkWrapBox::vspacing",
			   GTK_TYPE_UINT, GTK_ARG_READWRITE, ARG_VSPACING);
  gtk_object_add_arg_type ("GtkWrapBox::line_justify",
			   GTK_TYPE_JUSTIFICATION, GTK_ARG_READWRITE, ARG_LINE_JUSTIFY);
  gtk_object_add_arg_type ("GtkWrapBox::aspect_ratio",
			   GTK_TYPE_FLOAT, GTK_ARG_READWRITE, ARG_ASPECT_RATIO);
  gtk_object_add_arg_type ("GtkWrapBox::current_ratio",
			   GTK_TYPE_FLOAT, GTK_ARG_READABLE, ARG_CURRENT_RATIO);
  gtk_object_add_arg_type ("GtkWrapBox::max_children_per_line",
			   GTK_TYPE_UINT, GTK_ARG_READWRITE, ARG_CHILD_LIMIT);
  gtk_container_add_child_arg_type ("GtkWrapBox::position",
				    GTK_TYPE_INT, GTK_ARG_READWRITE, CHILD_ARG_POSITION);
  gtk_container_add_child_arg_type ("GtkWrapBox::hexpand",
				    GTK_TYPE_BOOL, GTK_ARG_READWRITE, CHILD_ARG_HEXPAND);
  gtk_container_add_child_arg_type ("GtkWrapBox::hfill",
				    GTK_TYPE_BOOL, GTK_ARG_READWRITE, CHILD_ARG_HFILL);
  gtk_container_add_child_arg_type ("GtkWrapBox::vexpand",
				    GTK_TYPE_BOOL, GTK_ARG_READWRITE, CHILD_ARG_VEXPAND);
  gtk_container_add_child_arg_type ("GtkWrapBox::vfill",
				    GTK_TYPE_BOOL, GTK_ARG_READWRITE, CHILD_ARG_VFILL);
}

static void
gtk_wrap_box_init (GtkWrapBox *wbox)
{
  GTK_WIDGET_SET_FLAGS (wbox, GTK_NO_WINDOW);
  
  wbox->homogeneous = FALSE;
  wbox->hspacing = 0;
  wbox->vspacing = 0;
  wbox->justify = GTK_JUSTIFY_LEFT;
  wbox->line_justify = GTK_JUSTIFY_BOTTOM;
  wbox->n_children = 0;
  wbox->children = NULL;
  wbox->aspect_ratio = 1;
  wbox->child_limit = 32767;
}

static void
gtk_wrap_box_set_arg (GtkObject *object,
		      GtkArg    *arg,
		      guint      arg_id)
{
  GtkWrapBox *wbox = GTK_WRAP_BOX (object);
  
  switch (arg_id)
    {
    case ARG_HOMOGENEOUS:
      gtk_wrap_box_set_homogeneous (wbox, GTK_VALUE_BOOL (*arg));
      break;
    case ARG_JUSTIFY:
      gtk_wrap_box_set_justify (wbox, GTK_VALUE_ENUM (*arg));
      break;
    case ARG_LINE_JUSTIFY:
      gtk_wrap_box_set_line_justify (wbox, GTK_VALUE_ENUM (*arg));
      break;
    case ARG_HSPACING:
      gtk_wrap_box_set_hspacing (wbox, GTK_VALUE_UINT (*arg));
      break;
    case ARG_VSPACING:
      gtk_wrap_box_set_vspacing (wbox, GTK_VALUE_UINT (*arg));
      break;
    case ARG_ASPECT_RATIO:
      gtk_wrap_box_set_aspect_ratio (wbox, GTK_VALUE_FLOAT (*arg));
      break;
    case ARG_CHILD_LIMIT:
      if (wbox->child_limit != GTK_VALUE_UINT (*arg))
	{
	  wbox->child_limit = CLAMP (GTK_VALUE_UINT (*arg), 1, 32767);
	  gtk_widget_queue_resize (GTK_WIDGET (wbox));
	}
      break;
    }
}

static void
gtk_wrap_box_get_arg (GtkObject *object,
		      GtkArg    *arg,
		      guint      arg_id)
{
  GtkWrapBox *wbox = GTK_WRAP_BOX (object);
  GtkWidget *widget = GTK_WIDGET (object);
  
  switch (arg_id)
    {
    case ARG_HOMOGENEOUS:
      GTK_VALUE_BOOL (*arg) = wbox->homogeneous;
      break;
    case ARG_JUSTIFY:
      GTK_VALUE_ENUM (*arg) = wbox->justify;
      break;
    case ARG_LINE_JUSTIFY:
      GTK_VALUE_ENUM (*arg) = wbox->line_justify;
      break;
    case ARG_HSPACING:
      GTK_VALUE_UINT (*arg) = wbox->hspacing;
      break;
    case ARG_VSPACING:
      GTK_VALUE_UINT (*arg) = wbox->vspacing;
      break;
    case ARG_ASPECT_RATIO:
      GTK_VALUE_FLOAT (*arg) = wbox->aspect_ratio;
      break;
    case ARG_CURRENT_RATIO:
      GTK_VALUE_FLOAT (*arg) = (((gfloat) widget->allocation.width) /
				((gfloat) widget->allocation.height));
      break;
    case ARG_CHILD_LIMIT:
      GTK_VALUE_UINT (*arg) = wbox->child_limit;
      break;
    default:
      arg->type = GTK_TYPE_INVALID;
      break;
    }
}

static void
gtk_wrap_box_set_child_arg (GtkContainer *container,
			    GtkWidget    *child,
			    GtkArg       *arg,
			    guint         arg_id)
{
  GtkWrapBox *wbox = GTK_WRAP_BOX (container);
  gboolean hexpand = FALSE, hfill = FALSE, vexpand = FALSE, vfill = FALSE;
  
  if (arg_id != CHILD_ARG_POSITION)
    gtk_wrap_box_query_child_packing (wbox, child, &hexpand, &hfill, &vexpand, &vfill);
  
  switch (arg_id)
    {
    case CHILD_ARG_POSITION:
      gtk_wrap_box_reorder_child (wbox, child, GTK_VALUE_INT (*arg));
      break;
    case CHILD_ARG_HEXPAND:
      gtk_wrap_box_set_child_packing (wbox, child,
				      GTK_VALUE_BOOL (*arg), hfill,
				      vexpand, vfill);
      break;
    case CHILD_ARG_HFILL:
      gtk_wrap_box_set_child_packing (wbox, child,
				      hexpand, GTK_VALUE_BOOL (*arg),
				      vexpand, vfill);
      break;
    case CHILD_ARG_VEXPAND:
      gtk_wrap_box_set_child_packing (wbox, child,
				      hexpand, hfill,
				      GTK_VALUE_BOOL (*arg), vfill);
      break;
    case CHILD_ARG_VFILL:
      gtk_wrap_box_set_child_packing (wbox, child,
				      hexpand, hfill,
				      vexpand, GTK_VALUE_BOOL (*arg));
      break;
    default:
      break;
    }
}

static void
gtk_wrap_box_get_child_arg (GtkContainer *container,
			    GtkWidget    *child,
			    GtkArg       *arg,
			    guint         arg_id)
{
  GtkWrapBox *wbox = GTK_WRAP_BOX (container);
  gboolean hexpand = FALSE, hfill = FALSE, vexpand = FALSE, vfill = FALSE;
  
  if (arg_id != CHILD_ARG_POSITION)
    gtk_wrap_box_query_child_packing (wbox, child, &hexpand, &hfill, &vexpand, &vfill);
  
  switch (arg_id)
    {
      GtkWrapBoxChild *child_info;
    case CHILD_ARG_POSITION:
      GTK_VALUE_INT (*arg) = 0;
      for (child_info = wbox->children; child_info; child_info = child_info->next)
	{
	  if (child_info->widget == child)
	    break;
	  GTK_VALUE_INT (*arg)++;
	}
      if (!child_info)
	GTK_VALUE_INT (*arg) = -1;
      break;
    case CHILD_ARG_HEXPAND:
      GTK_VALUE_BOOL (*arg) = hexpand;
      break;
    case CHILD_ARG_HFILL:
      GTK_VALUE_BOOL (*arg) = hfill;
      break;
    case CHILD_ARG_VEXPAND:
      GTK_VALUE_BOOL (*arg) = vexpand;
      break;
    case CHILD_ARG_VFILL:
      GTK_VALUE_BOOL (*arg) = vfill;
      break;
    default:
      arg->type = GTK_TYPE_INVALID;
      break;
    }
}

static GtkType
gtk_wrap_box_child_type	(GtkContainer *container)
{
  return GTK_TYPE_WIDGET;
}

void
gtk_wrap_box_set_homogeneous (GtkWrapBox *wbox,
			      gboolean    homogeneous)
{
  g_return_if_fail (GTK_IS_WRAP_BOX (wbox));
  
  homogeneous = homogeneous != FALSE;
  if (wbox->homogeneous != homogeneous)
    {
      wbox->homogeneous = homogeneous;
      gtk_widget_queue_resize (GTK_WIDGET (wbox));
    }
}

void
gtk_wrap_box_set_hspacing (GtkWrapBox *wbox,
			   guint       hspacing)
{
  g_return_if_fail (GTK_IS_WRAP_BOX (wbox));
  
  if (wbox->hspacing != hspacing)
    {
      wbox->hspacing = hspacing;
      gtk_widget_queue_resize (GTK_WIDGET (wbox));
    }
}

void
gtk_wrap_box_set_vspacing (GtkWrapBox *wbox,
			   guint       vspacing)
{
  g_return_if_fail (GTK_IS_WRAP_BOX (wbox));
  
  if (wbox->vspacing != vspacing)
    {
      wbox->vspacing = vspacing;
      gtk_widget_queue_resize (GTK_WIDGET (wbox));
    }
}

void
gtk_wrap_box_set_justify (GtkWrapBox      *wbox,
			  GtkJustification justify)
{
  g_return_if_fail (GTK_IS_WRAP_BOX (wbox));
  g_return_if_fail (justify <= GTK_JUSTIFY_FILL);
  
  if (wbox->justify != justify)
    {
      wbox->justify = justify;
      gtk_widget_queue_resize (GTK_WIDGET (wbox));
    }
}

void
gtk_wrap_box_set_line_justify (GtkWrapBox      *wbox,
			       GtkJustification line_justify)
{
  g_return_if_fail (GTK_IS_WRAP_BOX (wbox));
  g_return_if_fail (line_justify <= GTK_JUSTIFY_FILL);
  
  if (wbox->line_justify != line_justify)
    {
      wbox->line_justify = line_justify;
      gtk_widget_queue_resize (GTK_WIDGET (wbox));
    }
}

void
gtk_wrap_box_set_aspect_ratio (GtkWrapBox *wbox,
			       gfloat      aspect_ratio)
{
  g_return_if_fail (GTK_IS_WRAP_BOX (wbox));
  
  aspect_ratio = CLAMP (aspect_ratio, 1.0 / 256.0, 256.0);
  
  if (wbox->aspect_ratio != aspect_ratio)
    {
      wbox->aspect_ratio = aspect_ratio;
      gtk_widget_queue_resize (GTK_WIDGET (wbox));
    }
}

void
gtk_wrap_box_pack (GtkWrapBox *wbox,
		   GtkWidget  *child,
		   gboolean    hexpand,
		   gboolean    hfill,
		   gboolean    vexpand,
		   gboolean    vfill)
{
  GtkWrapBoxChild *child_info;
  
  g_return_if_fail (GTK_IS_WRAP_BOX (wbox));
  g_return_if_fail (GTK_IS_WIDGET (child));
  g_return_if_fail (child->parent == NULL);
  
  child_info = g_new (GtkWrapBoxChild, 1);
  child_info->widget = child;
  child_info->hexpand = hexpand ? TRUE : FALSE;
  child_info->hfill = hfill ? TRUE : FALSE;
  child_info->vexpand = vexpand ? TRUE : FALSE;
  child_info->vfill = vfill ? TRUE : FALSE;
  child_info->next = NULL;
  if (wbox->children)
    {
      GtkWrapBoxChild *last = wbox->children;
      
      while (last->next)
	last = last->next;
      last->next = child_info;
    }
  else
    wbox->children = child_info;
  wbox->n_children++;
  
  gtk_widget_set_parent (child, GTK_WIDGET (wbox));
  
  if (GTK_WIDGET_REALIZED (wbox))
    gtk_widget_realize (child);
  
  if (GTK_WIDGET_VISIBLE (wbox) && GTK_WIDGET_VISIBLE (child))
    {
      if (GTK_WIDGET_MAPPED (wbox))
	gtk_widget_map (child);
      
      gtk_widget_queue_resize (child);
    }
}

void
gtk_wrap_box_reorder_child (GtkWrapBox *wbox,
			    GtkWidget  *child,
			    gint        position)
{
  GtkWrapBoxChild *child_info, *last = NULL;
  
  g_return_if_fail (GTK_IS_WRAP_BOX (wbox));
  g_return_if_fail (GTK_IS_WIDGET (child));
  
  for (child_info = wbox->children; child_info; last = child_info, child_info = last->next)
    if (child_info->widget == child)
      break;
  
  if (child_info && wbox->children->next)
    {
      GtkWrapBoxChild *tmp;
      
      if (last)
	last->next = child_info->next;
      else
	wbox->children = child_info->next;
      
      last = NULL;
      tmp = wbox->children;
      while (position && tmp->next)
	{
	  position--;
	  last = tmp;
	  tmp = last->next;
	}
      
      if (position)
	{
	  tmp->next = child_info;
	  child_info->next = NULL;
	}
      else
	{
	  child_info->next = tmp;
	  if (last)
	    last->next = child_info;
	  else
	    wbox->children = child_info;
	}
      
      if (GTK_WIDGET_VISIBLE (child) && GTK_WIDGET_VISIBLE (wbox))
	gtk_widget_queue_resize (child);
    }
}

void
gtk_wrap_box_query_child_packing (GtkWrapBox *wbox,
				  GtkWidget  *child,
				  gboolean   *hexpand,
				  gboolean   *hfill,
				  gboolean   *vexpand,
				  gboolean   *vfill)
{
  GtkWrapBoxChild *child_info;
  
  g_return_if_fail (GTK_IS_WRAP_BOX (wbox));
  g_return_if_fail (GTK_IS_WIDGET (child));
  
  for (child_info = wbox->children; child_info; child_info = child_info->next)
    if (child_info->widget == child)
      break;
  
  if (child_info)
    {
      if (hexpand)
	*hexpand = child_info->hexpand;
      if (hfill)
	*hfill = child_info->hfill;
      if (vexpand)
	*vexpand = child_info->vexpand;
      if (vfill)
	*vfill = child_info->vfill;
    }
}

void
gtk_wrap_box_set_child_packing (GtkWrapBox *wbox,
				GtkWidget  *child,
				gboolean    hexpand,
				gboolean    hfill,
				gboolean    vexpand,
				gboolean    vfill)
{
  GtkWrapBoxChild *child_info;
  
  g_return_if_fail (GTK_IS_WRAP_BOX (wbox));
  g_return_if_fail (GTK_IS_WIDGET (child));
  
  hexpand = hexpand != FALSE;
  hfill = hfill != FALSE;
  vexpand = vexpand != FALSE;
  vfill = vfill != FALSE;
  
  for (child_info = wbox->children; child_info; child_info = child_info->next)
    if (child_info->widget == child)
      break;
  
  if (child_info &&
      (child_info->hexpand != hexpand || child_info->vexpand != vexpand ||
       child_info->hfill != hfill || child_info->vfill != vfill))
    {
      child_info->hexpand = hexpand;
      child_info->hfill = hfill;
      child_info->vexpand = vexpand;
      child_info->vfill = vfill;
      
      if (GTK_WIDGET_VISIBLE (child) && GTK_WIDGET_VISIBLE (wbox))
	gtk_widget_queue_resize (child);
    }
}

guint*
gtk_wrap_box_query_line_lengths (GtkWrapBox *wbox,
				 guint      *_n_lines)
{
  GtkWrapBoxChild *next_child = NULL;
  GtkAllocation area, *allocation;
  gboolean expand_line;
  GSList *slist;
  guint max_child_size, border, n_lines = 0, *lines = NULL;

  if (_n_lines)
    *_n_lines = 0;
  g_return_val_if_fail (GTK_IS_WRAP_BOX (wbox), NULL);

  allocation = &GTK_WIDGET (wbox)->allocation;
  border = GTK_CONTAINER (wbox)->border_width;
  area.x = allocation->x + border;
  area.y = allocation->y + border;
  area.width = MAX (1, (gint) allocation->width - border * 2);
  area.height = MAX (1, (gint) allocation->height - border * 2);

  next_child = wbox->children;
  slist = GTK_WRAP_BOX_GET_CLASS (wbox)->rlist_line_children (wbox,
							      &next_child,
							      &area,
							      &max_child_size,
							      &expand_line);
  while (slist)
    {
      guint l = n_lines++;

      lines = g_renew (guint, lines, n_lines);
      lines[l] = g_slist_length (slist);
      g_slist_free (slist);

      slist = GTK_WRAP_BOX_GET_CLASS (wbox)->rlist_line_children (wbox,
								  &next_child,
								  &area,
								  &max_child_size,
								  &expand_line);
    }

  if (_n_lines)
    *_n_lines = n_lines;

  return lines;
}

static void
gtk_wrap_box_map (GtkWidget *widget)
{
  GtkWrapBox *wbox = GTK_WRAP_BOX (widget);
  GtkWrapBoxChild *child;
  
  GTK_WIDGET_SET_FLAGS (wbox, GTK_MAPPED);
  
  for (child = wbox->children; child; child = child->next)
    if (GTK_WIDGET_VISIBLE (child->widget) &&
	!GTK_WIDGET_MAPPED (child->widget))
      gtk_widget_map (child->widget);
}

static void
gtk_wrap_box_unmap (GtkWidget *widget)
{
  GtkWrapBox *wbox = GTK_WRAP_BOX (widget);
  GtkWrapBoxChild *child;
  
  GTK_WIDGET_UNSET_FLAGS (wbox, GTK_MAPPED);
  
  for (child = wbox->children; child; child = child->next)
    if (GTK_WIDGET_VISIBLE (child->widget) &&
	GTK_WIDGET_MAPPED (child->widget))
      gtk_widget_unmap (child->widget);
}

static void
gtk_wrap_box_draw (GtkWidget    *widget,
		   GdkRectangle *area)
{
  GtkWrapBox *wbox = GTK_WRAP_BOX (widget);
  GtkWrapBoxChild *child;
  GdkRectangle child_area;
  
  if (GTK_WIDGET_DRAWABLE (widget))
    for (child = wbox->children; child; child = child->next)
      if (GTK_WIDGET_DRAWABLE (child->widget) &&
	  gtk_widget_intersect (child->widget, area, &child_area))
	gtk_widget_draw (child->widget, &child_area);
}

static gint
gtk_wrap_box_expose (GtkWidget      *widget,
		     GdkEventExpose *event)
{
  GtkWrapBox *wbox = GTK_WRAP_BOX (widget);
  GtkWrapBoxChild *child;
  GdkEventExpose child_event = *event;
  
  g_return_val_if_fail (event != NULL, FALSE);
  
  if (GTK_WIDGET_DRAWABLE (widget))
    for (child = wbox->children; child; child = child->next)
      if (GTK_WIDGET_DRAWABLE (child->widget) &&
	  GTK_WIDGET_NO_WINDOW (child->widget) &&
	  gtk_widget_intersect (child->widget, &event->area, &child_event.area))
	gtk_widget_event (child->widget, (GdkEvent*) &child_event);
  
  return TRUE;
}

static void
gtk_wrap_box_add (GtkContainer *container,
		  GtkWidget    *widget)
{
  gtk_wrap_box_pack (GTK_WRAP_BOX (container), widget, FALSE, TRUE, FALSE, TRUE);
}

static void
gtk_wrap_box_remove (GtkContainer *container,
		     GtkWidget    *widget)
{
  GtkWrapBox *wbox = GTK_WRAP_BOX (container);
  GtkWrapBoxChild *child, *last = NULL;
  
  child = wbox->children;
  while (child)
    {
      if (child->widget == widget)
	{
	  gboolean was_visible;
	  
	  was_visible = GTK_WIDGET_VISIBLE (widget);
	  gtk_widget_unparent (widget);
	  
	  if (last)
	    last->next = child->next;
	  else
	    wbox->children = child->next;
	  g_free (child);
	  wbox->n_children--;
	  
	  if (was_visible)
	    gtk_widget_queue_resize (GTK_WIDGET (container));
	  
	  break;
	}
      
      last = child;
      child = last->next;
    }
}

static void
gtk_wrap_box_forall (GtkContainer *container,
		     gboolean      include_internals,
		     GtkCallback   callback,
		     gpointer      callback_data)
{
  GtkWrapBox *wbox = GTK_WRAP_BOX (container);
  GtkWrapBoxChild *child;
  
  child = wbox->children;
  while (child)
    {
      GtkWidget *widget = child->widget;
      
      child = child->next;
      
      callback (widget, callback_data);
    }
}
