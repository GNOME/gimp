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

#include "config.h"

#include "gtkwrapbox.h"


/* --- properties --- */
enum {
  PROP_0,
  PROP_HOMOGENEOUS,
  PROP_JUSTIFY,
  PROP_HSPACING,
  PROP_VSPACING,
  PROP_LINE_JUSTIFY,
  PROP_ASPECT_RATIO,
  PROP_CURRENT_RATIO,
  PROP_CHILD_LIMIT
};

enum {
  CHILD_PROP_0,
  CHILD_PROP_POSITION,
  CHILD_PROP_HEXPAND,
  CHILD_PROP_HFILL,
  CHILD_PROP_VEXPAND,
  CHILD_PROP_VFILL,
  CHILD_PROP_WRAPPED
};


/* --- prototypes --- */
static void gtk_wrap_box_class_init    (GtkWrapBoxClass    *klass);
static void gtk_wrap_box_init          (GtkWrapBox         *wbox);
static void gtk_wrap_box_set_property  (GObject            *object,
					guint               property_id,
					const GValue       *value,
					GParamSpec         *pspec);
static void gtk_wrap_box_get_property  (GObject            *object,
					guint               property_id,
					GValue             *value,
					GParamSpec         *pspec);
static void gtk_wrap_box_set_child_property (GtkContainer    *container,
					     GtkWidget       *child,
					     guint            property_id,
					     const GValue    *value,
					     GParamSpec      *pspec);
static void gtk_wrap_box_get_child_property (GtkContainer    *container,
					     GtkWidget       *child,
					     guint            property_id,
					     GValue          *value,
					     GParamSpec      *pspec);
static void gtk_wrap_box_map           (GtkWidget          *widget);
static void gtk_wrap_box_unmap         (GtkWidget          *widget);
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
static GType gtk_wrap_box_child_type   (GtkContainer       *container);


/* --- variables --- */
static gpointer parent_class = NULL;


/* --- functions --- */
GType
gtk_wrap_box_get_type (void)
{
  static GType wrap_box_type = 0;
  
  if (!wrap_box_type)
    {
      static const GTypeInfo wrap_box_info =
      {
	sizeof (GtkWrapBoxClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) gtk_wrap_box_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (GtkWrapBox),
	0,		/* n_preallocs */
	(GInstanceInitFunc) gtk_wrap_box_init,
      };
      
      wrap_box_type = g_type_register_static (GTK_TYPE_CONTAINER, "GtkWrapBox",
					      &wrap_box_info, 0);
    }
  
  return wrap_box_type;
}

static void
gtk_wrap_box_class_init (GtkWrapBoxClass *class)
{
  GObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkContainerClass *container_class;
  
  object_class = G_OBJECT_CLASS (class);
  widget_class = GTK_WIDGET_CLASS (class);
  container_class = GTK_CONTAINER_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  object_class->set_property = gtk_wrap_box_set_property;
  object_class->get_property = gtk_wrap_box_get_property;
  
  widget_class->map = gtk_wrap_box_map;
  widget_class->unmap = gtk_wrap_box_unmap;
  widget_class->expose_event = gtk_wrap_box_expose;
  
  container_class->add = gtk_wrap_box_add;
  container_class->remove = gtk_wrap_box_remove;
  container_class->forall = gtk_wrap_box_forall;
  container_class->child_type = gtk_wrap_box_child_type;
  container_class->set_child_property = gtk_wrap_box_set_child_property;
  container_class->get_child_property = gtk_wrap_box_get_child_property;

  class->rlist_line_children = NULL;

  g_object_class_install_property (object_class,
				   PROP_HOMOGENEOUS,
				   g_param_spec_boolean ("homogeneous",
							 NULL,
							 NULL,
							 FALSE,
							 G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_JUSTIFY,
				   g_param_spec_enum ("justify",
						      NULL,
						      NULL,
						      GTK_TYPE_JUSTIFICATION,
						      GTK_JUSTIFY_LEFT,
						      G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_HSPACING,
				   g_param_spec_uint ("hspacing",
						      NULL,
						      NULL,
						      0,
						      G_MAXINT,
						      0,
						      G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_VSPACING,
				   g_param_spec_uint ("vspacing",
						      NULL,
						      NULL,
						      0,
						      G_MAXINT,
						      0,
						      G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_LINE_JUSTIFY,
				   g_param_spec_enum ("line_justify",
						      NULL,
						      NULL,
						      GTK_TYPE_JUSTIFICATION,
						      GTK_JUSTIFY_BOTTOM,
						      G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_ASPECT_RATIO,
				   g_param_spec_float ("aspect_ratio",
						       NULL,
						       NULL,
						       0.0,
						       G_MAXFLOAT,
						       1.0,
						       G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_CURRENT_RATIO,
				   g_param_spec_float ("current_ratio",
						       NULL,
						       NULL,
						       0.0,
						       G_MAXFLOAT,
						       1.0,
						       G_PARAM_READABLE));

  g_object_class_install_property (object_class,
				   PROP_CHILD_LIMIT,
				   g_param_spec_uint ("max_children_per_line",
						      NULL,
						      NULL,
						      1,
						      32767,
						      32767,
						      G_PARAM_READWRITE));

  gtk_container_class_install_child_property (container_class,
					      CHILD_PROP_POSITION,
					      g_param_spec_int ("position",
								NULL,
								NULL,
								-1, G_MAXINT, 0,
								G_PARAM_READWRITE));
  gtk_container_class_install_child_property (container_class,
					      CHILD_PROP_HEXPAND,
					      g_param_spec_boolean ("hexpand",
								    NULL,
								    NULL,
								    FALSE,
								    G_PARAM_READWRITE));
  gtk_container_class_install_child_property (container_class,
					      CHILD_PROP_HFILL,
					      g_param_spec_boolean ("hfill",
								    NULL,
								    NULL,
								    FALSE,
								    G_PARAM_READWRITE));
  gtk_container_class_install_child_property (container_class,
					      CHILD_PROP_VEXPAND,
					      g_param_spec_boolean ("vexpand",
								    NULL,
								    NULL,
								    FALSE,
								    G_PARAM_READWRITE));
  gtk_container_class_install_child_property (container_class,
					      CHILD_PROP_VFILL,
					      g_param_spec_boolean ("vfill",
								    NULL,
								    NULL,
								    FALSE,
								    G_PARAM_READWRITE));
  gtk_container_class_install_child_property (container_class,
					      CHILD_PROP_WRAPPED,
					      g_param_spec_boolean ("wrapped",
								    NULL,
								    NULL,
								    FALSE,
								    G_PARAM_READWRITE));
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
  wbox->aspect_ratio = 1.0;
  wbox->child_limit = 32767;
}

static void
gtk_wrap_box_set_property (GObject      *object,
			   guint         property_id,
			   const GValue *value,
			   GParamSpec   *pspec)
{
  GtkWrapBox *wbox = GTK_WRAP_BOX (object);
  
  switch (property_id)
    {
    case PROP_HOMOGENEOUS:
      gtk_wrap_box_set_homogeneous (wbox, g_value_get_boolean (value));
      break;
    case PROP_JUSTIFY:
      gtk_wrap_box_set_justify (wbox, g_value_get_enum (value));
      break;
    case PROP_LINE_JUSTIFY:
      gtk_wrap_box_set_line_justify (wbox, g_value_get_enum (value));
      break;
    case PROP_HSPACING:
      gtk_wrap_box_set_hspacing (wbox, g_value_get_uint (value));
      break;
    case PROP_VSPACING:
      gtk_wrap_box_set_vspacing (wbox, g_value_get_uint (value));
      break;
    case PROP_ASPECT_RATIO:
      gtk_wrap_box_set_aspect_ratio (wbox, g_value_get_float (value));
      break;
    case PROP_CHILD_LIMIT:
      if (wbox->child_limit != g_value_get_uint (value))
	gtk_widget_queue_resize (GTK_WIDGET (wbox));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gtk_wrap_box_get_property (GObject    *object,
			   guint       property_id,
			   GValue     *value,
			   GParamSpec *pspec)
{
  GtkWrapBox *wbox = GTK_WRAP_BOX (object);
  GtkWidget *widget = GTK_WIDGET (object);
  
  switch (property_id)
    {
    case PROP_HOMOGENEOUS:
      g_value_set_boolean (value, wbox->homogeneous);
      break;
    case PROP_JUSTIFY:
      g_value_set_enum (value, wbox->justify);
      break;
    case PROP_LINE_JUSTIFY:
      g_value_set_enum (value, wbox->line_justify);
      break;
    case PROP_HSPACING:
      g_value_set_uint (value, wbox->hspacing);
      break;
    case PROP_VSPACING:
      g_value_set_uint (value, wbox->vspacing);
      break;
    case PROP_ASPECT_RATIO:
      g_value_set_float (value, wbox->aspect_ratio);
      break;
    case PROP_CURRENT_RATIO:
      g_value_set_float (value, (((gfloat) widget->allocation.width) /
				 ((gfloat) widget->allocation.height)));
      break;
    case PROP_CHILD_LIMIT:
      g_value_set_uint (value, wbox->child_limit);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gtk_wrap_box_set_child_property (GtkContainer    *container,
				 GtkWidget       *child,
				 guint            property_id,
				 const GValue    *value,
				 GParamSpec      *pspec)
{
  GtkWrapBox *wbox = GTK_WRAP_BOX (container);
  gboolean hexpand = FALSE, hfill = FALSE;
  gboolean vexpand = FALSE, vfill = FALSE;
  gboolean wrapped = FALSE;

  if (property_id != CHILD_PROP_POSITION)
    gtk_wrap_box_query_child_packing (wbox, child,
				      &hexpand, &hfill,
				      &vexpand, &vfill,
				      &wrapped);
  
  switch (property_id)
    {
    case CHILD_PROP_POSITION:
      gtk_wrap_box_reorder_child (wbox, child, g_value_get_int (value));
      break;
    case CHILD_PROP_HEXPAND:
      gtk_wrap_box_set_child_packing (wbox, child,
				      g_value_get_boolean (value), hfill,
				      vexpand, vfill,
				      wrapped);
      break;
    case CHILD_PROP_HFILL:
      gtk_wrap_box_set_child_packing (wbox, child,
				      hexpand, g_value_get_boolean (value),
				      vexpand, vfill,
				      wrapped);
      break;
    case CHILD_PROP_VEXPAND:
      gtk_wrap_box_set_child_packing (wbox, child,
				      hexpand, hfill,
				      g_value_get_boolean (value), vfill,
				      wrapped);
      break;
    case CHILD_PROP_VFILL:
      gtk_wrap_box_set_child_packing (wbox, child,
				      hexpand, hfill,
				      vexpand, g_value_get_boolean (value),
				      wrapped);
      break;
    case CHILD_PROP_WRAPPED:
      gtk_wrap_box_set_child_packing (wbox, child,
				      hexpand, hfill,
				      vexpand, vfill,
				      g_value_get_boolean (value));
      break;
    default:
      GTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static void
gtk_wrap_box_get_child_property (GtkContainer    *container,
				 GtkWidget       *child,
				 guint            property_id,
				 GValue 	 *value,
				 GParamSpec      *pspec)
{
  GtkWrapBox *wbox = GTK_WRAP_BOX (container);
  gboolean hexpand = FALSE, hfill = FALSE;
  gboolean vexpand = FALSE, vfill = FALSE;
  gboolean wrapped = FALSE;
  
  if (property_id != CHILD_PROP_POSITION)
    gtk_wrap_box_query_child_packing (wbox, child,
				      &hexpand, &hfill,
				      &vexpand, &vfill,
				      &wrapped);
  
  switch (property_id)
    {
      GtkWrapBoxChild *child_info;
      guint i;
    case CHILD_PROP_POSITION:
      i = 0;
      for (child_info = wbox->children; child_info; child_info = child_info->next)
	{
	  if (child_info->widget == child)
	    break;
	  i += 1;
	}
      g_value_set_int (value, child_info ? i : -1);
      break;
    case CHILD_PROP_HEXPAND:
      g_value_set_boolean (value, hexpand);
      break;
    case CHILD_PROP_HFILL:
      g_value_set_boolean (value, hfill);
      break;
    case CHILD_PROP_VEXPAND:
      g_value_set_boolean (value, vexpand);
      break;
    case CHILD_PROP_VFILL:
      g_value_set_boolean (value, vfill);
      break;
    case CHILD_PROP_WRAPPED:
      g_value_set_boolean (value, wrapped);
      break;
    default:
      GTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static GType
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
  g_return_if_fail (GTK_IS_WRAP_BOX (wbox));
  g_return_if_fail (GTK_IS_WIDGET (child));
  g_return_if_fail (child->parent == NULL);

  gtk_wrap_box_pack_wrapped (wbox, child, hexpand, hfill, vexpand, vfill, FALSE);
}

void
gtk_wrap_box_pack_wrapped (GtkWrapBox *wbox,
			   GtkWidget  *child,
			   gboolean    hexpand,
			   gboolean    hfill,
			   gboolean    vexpand,
			   gboolean    vfill,
			   gboolean    wrapped)
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
  child_info->wrapped = wrapped ? TRUE : FALSE;
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
				  gboolean   *vfill,
				  gboolean   *wrapped)
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
      if (wrapped)
	*wrapped = child_info->wrapped;
    }
}

void
gtk_wrap_box_set_child_packing (GtkWrapBox *wbox,
				GtkWidget  *child,
				gboolean    hexpand,
				gboolean    hfill,
				gboolean    vexpand,
				gboolean    vfill,
				gboolean    wrapped)
{
  GtkWrapBoxChild *child_info;
  
  g_return_if_fail (GTK_IS_WRAP_BOX (wbox));
  g_return_if_fail (GTK_IS_WIDGET (child));
  
  hexpand = hexpand != FALSE;
  hfill = hfill != FALSE;
  vexpand = vexpand != FALSE;
  vfill = vfill != FALSE;
  wrapped = wrapped != FALSE;

  for (child_info = wbox->children; child_info; child_info = child_info->next)
    if (child_info->widget == child)
      break;
  
  if (child_info &&
      (child_info->hexpand != hexpand || child_info->vexpand != vexpand ||
       child_info->hfill != hfill || child_info->vfill != vfill ||
       child_info->wrapped != wrapped))
    {
      child_info->hexpand = hexpand;
      child_info->hfill = hfill;
      child_info->vexpand = vexpand;
      child_info->vfill = vfill;
      child_info->wrapped = wrapped;
      
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

static gint
gtk_wrap_box_expose (GtkWidget      *widget,
		     GdkEventExpose *event)
{
  return GTK_WIDGET_CLASS (parent_class)->expose_event (widget, event);
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
