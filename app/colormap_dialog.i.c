#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "gdk/gdkkeysyms.h"

#include "color_notebook.h"
#include "image_render.h"
#include "dialog_handler.h"
#include "buildmenu.h"
#include "colormaps.h"
#include "color_area.h"
/*#include "gdisplay.h"*/
#include "general.h"
#include "gimpdnd.h"
#include "gimpui.h"

#include "libgimp/gimpintl.h"

/*  Add these features:
 *
 *  load/save colormaps
 *  requantize
 *  add color--by clicking in the checked region
 *  all changes need to flush colormap lookup cache
 */


/*  indexed palette routines  */
static void ipal_draw (GimpColormapDialog*);
static void ipal_clear (GimpColormapDialog*);
static void ipal_set_image (GimpColormapDialog*, GimpImage*);
static void ipal_update_entries(GimpColormapDialog* ipal);
static void ipal_set_index (GimpColormapDialog* ipal, gint i);
static void ipal_draw_cell (GimpColormapDialog* ipal, gint col);
static void ipal_update_image_list (GimpColormapDialog* ipal);


/*  indexed palette menu callbacks  */
static void ipal_add_callback (GtkWidget *, gpointer);
static void ipal_edit_callback (GtkWidget *, gpointer);
static void ipal_close_callback (GtkWidget *, gpointer);
static void ipal_select_callback (int, int, int, ColorNotebookState, void *);

/*  event callback  */
static gint ipal_area_events (GtkWidget *, GdkEvent *, GimpColormapDialog *);

/*  create image menu  */
static void image_menu_callback (GtkWidget *, gpointer);
static GtkWidget * create_image_menu (GimpColormapDialog*,
				      GimpImage**,
				      int *,
				      MenuItemCallback);
static void frame_size_alloc_cb(GtkFrame* frame, GtkAllocation* alloc,
				GimpColormapDialog* ipal);
static void window_size_req_cb(GtkWindow* win, GtkRequisition* req,
			       GimpColormapDialog* ipal);

static void index_entry_change_cb(GtkEntry* entry, GimpColormapDialog* ipal);
static void hex_entry_change_cb(GtkEntry* entry, GimpColormapDialog* ipal);

static void
set_addrem_cb(GimpSet* set, GimpImage* image, GimpColormapDialog* ipal);
static void
image_rename_cb(GimpImage* img, GimpColormapDialog* ipal);
static void
image_cmap_change_cb(GimpImage* img, gint ncol, GimpColormapDialog* ipal);
	       
/*  dnd stuff  */
static GtkTargetEntry color_palette_target_table[] =
{
  GIMP_TARGET_COLOR
};
static guint n_color_palette_targets = (sizeof (color_palette_target_table) /
					sizeof (color_palette_target_table[0]));

static void
palette_drag_color (GtkWidget *widget,
		    guchar    *r,
		    guchar    *g,
		    guchar    *b,
		    gpointer   data)
{
  GimpColormapDialog* ipal = (GimpColormapDialog*)data;
  guint col = ipal->col_index;
  GimpImage *gimage;

  gimage = ipal->image;

  *r = gimage->cmap[col * 3 + 0];
  *g = gimage->cmap[col * 3 + 1];
  *b = gimage->cmap[col * 3 + 2];
}

static void
palette_drop_color (GtkWidget *widget,
		    guchar      r,
		    guchar      g,
		    guchar      b,
		    gpointer    data)
{
  GimpColormapDialog* ipal = (GimpColormapDialog*)data;

  if(!GTK_WIDGET_IS_SENSITIVE (ipal->vbox) || 
     !(ipal->image->num_cols < 256))
    return;

  ipal->image->cmap[ipal->image->num_cols * 3 + 0] = r;
  ipal->image->cmap[ipal->image->num_cols * 3 + 1] = g;
  ipal->image->cmap[ipal->image->num_cols * 3 + 2] = b;
  ipal->image->num_cols++;
  gimp_image_colormap_changed (ipal->image, -1);
}

/**************************************/
/*  Public indexed palette functions  */
/**************************************/

#define COLORMAP_DIALOG_CREATE ipal_create

GimpColormapDialog*
ipal_create (GimpSet* context)
{
  gint i;
  GtkWidget *vbox;
  GtkWidget *frame;
  GtkWidget *util_box;
  GtkWidget *label;
  GtkWidget *arrow_hbox;
  GtkWidget *arrow;
  GtkWidget *ops_menu;
  GtkWidget *menu_bar;
  GtkWidget *menu_bar_item;
  GtkWidget *hbox;
  GtkWidget *hbox2;
  GtkWidget *entry;
  GtkWidget *evbox;
  GtkAccelGroup *accel_group;
  GimpColormapDialog* ipal;

  MenuItem indexed_color_ops[] =
  {
    { N_("Add"), 'N', GDK_CONTROL_MASK,
      ipal_add_callback, NULL, NULL, NULL },
    { N_("Edit"), 'E', GDK_CONTROL_MASK,
      ipal_edit_callback, NULL, NULL, NULL },
    { N_("Close"), 'W', GDK_CONTROL_MASK,
      ipal_close_callback, NULL, NULL, NULL },
    { NULL, 0, 0, NULL, NULL, NULL, NULL },
  };
  
  ipal = gtk_type_new (GIMP_TYPE_COLORMAP_DIALOG);

  /*  The action area  */
  gimp_dialog_create_action_area (GTK_DIALOG (ipal),

				  _("Close"), ipal_close_callback,
				  ipal, NULL, TRUE, TRUE,

				  NULL);

  ipal->image = NULL;
  ipal->context = context;
  ipal->cmap_changed_handler
    = gimp_set_add_handler(context, "colormap_changed",
			   image_cmap_change_cb, ipal);
  ipal->rename_handler
    = gimp_set_add_handler(context, "rename",
			   image_rename_cb, ipal);

  accel_group = gtk_accel_group_new ();
  gtk_window_set_wmclass (GTK_WINDOW (ipal), "indexed_color_palette", "Gimp");
  dialog_register (GTK_WIDGET (ipal));
  gtk_window_set_policy (GTK_WINDOW (ipal), TRUE, TRUE, TRUE); 
  gtk_window_set_title (GTK_WINDOW (ipal), _("Indexed Color Palette"));
  gtk_window_add_accel_group (GTK_WINDOW (ipal), accel_group);
  gtk_signal_connect (GTK_OBJECT (ipal), "delete_event",
		      GTK_SIGNAL_FUNC (gtk_widget_hide_on_delete),
		      NULL);
  
  ipal->vbox = vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 1);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (ipal)->vbox), vbox, TRUE, TRUE, 0);
  
  /*  The hbox to hold the command menu and image option menu box  */
  util_box = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), util_box, FALSE, FALSE, 0);
  
  /*  The GIMP image option menu  */
  label = gtk_label_new (_("Image:"));
  gtk_box_pack_start (GTK_BOX (util_box), label, FALSE, FALSE, 2);
  ipal->option_menu = GTK_OPTION_MENU(gtk_option_menu_new ());
  gtk_box_pack_start (GTK_BOX (util_box),
		      GTK_WIDGET (ipal->option_menu),
		      TRUE, TRUE, 2);
  
  /*  The indexed palette commands pulldown menu  */
  for(i=0;i<sizeof(indexed_color_ops)/sizeof(indexed_color_ops[0]);i++)
    indexed_color_ops[i].user_data = ipal;
  ops_menu = build_menu (indexed_color_ops, accel_group);
  ipal->add_item = indexed_color_ops[0].widget;
  menu_bar = gtk_menu_bar_new ();
  gtk_box_pack_start (GTK_BOX (util_box), menu_bar, FALSE, FALSE, 2);
  menu_bar_item = gtk_menu_item_new ();
  gtk_container_add (GTK_CONTAINER (menu_bar), menu_bar_item);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_bar_item), ops_menu);
  arrow_hbox = gtk_hbox_new (FALSE, 1);
  gtk_container_add (GTK_CONTAINER (menu_bar_item), arrow_hbox);
  label = gtk_label_new (_("Operations"));
  arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_OUT);
  gtk_box_pack_start (GTK_BOX (arrow_hbox), arrow, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (arrow_hbox), label, FALSE, FALSE, 4);
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
  gtk_misc_set_alignment (GTK_MISC (arrow), 0.5, 0.5);
  
  hbox2 = gtk_hbox_new (TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox2, TRUE, TRUE, 4);
  /*  The palette frame  */
  evbox = gtk_event_box_new ();
  gtk_container_set_resize_mode (GTK_CONTAINER(evbox),
				 GTK_RESIZE_QUEUE); 
  gtk_widget_set_usize(GTK_WIDGET(evbox), -2, 60); /* give it some initial 
                                                      size */
  gtk_signal_connect(GTK_OBJECT(evbox), "size_request",
    GTK_SIGNAL_FUNC(window_size_req_cb), ipal);
  gtk_signal_connect(GTK_OBJECT(evbox), "size_allocate",
		     GTK_SIGNAL_FUNC(frame_size_alloc_cb), ipal);
  gtk_box_pack_start (GTK_BOX (hbox2), evbox, TRUE, TRUE, 2);
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER(evbox), frame);
  ipal->palette = GTK_PREVIEW (gtk_preview_new (GTK_PREVIEW_COLOR));
  gtk_preview_size(ipal->palette, 256, 256);
  gtk_widget_set_events (GTK_WIDGET(ipal->palette), GDK_BUTTON_PRESS_MASK);
  ipal->event_handler =
  gtk_signal_connect (GTK_OBJECT (ipal->palette), "event",
		      (GtkSignalFunc) ipal_area_events,
		      ipal);
  gtk_signal_handler_block(GTK_OBJECT(ipal->palette),
			   ipal->event_handler);

  gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET(ipal->palette));

  /*  dnd stuff  */
  gtk_drag_source_set (GTK_WIDGET(ipal->palette),
                       GDK_BUTTON1_MASK | GDK_BUTTON3_MASK,
                       color_palette_target_table, n_color_palette_targets,
                       GDK_ACTION_COPY | GDK_ACTION_MOVE);
  gimp_dnd_color_source_set (GTK_WIDGET (ipal->palette),
			     palette_drag_color, ipal);

  gtk_drag_dest_set (GTK_WIDGET(ipal->palette),
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_DROP,
                     color_palette_target_table, n_color_palette_targets,
                     GDK_ACTION_COPY);
  gimp_dnd_color_dest_set (GTK_WIDGET (ipal->palette),
			   palette_drop_color, ipal);
  
  /* some helpful hints */
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 1);
  label = gtk_label_new (_("Index:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 1);
  entry = gtk_entry_new_with_max_length (3);
  ipal->index_entry = GTK_ENTRY(entry);
  gtk_signal_connect (GTK_OBJECT(entry), "activate",
		      GTK_SIGNAL_FUNC(index_entry_change_cb), ipal);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 1);
  label = gtk_label_new (_("Hex Triplet:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 1);
  entry = gtk_entry_new_with_max_length (7);
  ipal->color_entry = GTK_ENTRY(entry);
  gtk_signal_connect (GTK_OBJECT(entry), "activate",
		      GTK_SIGNAL_FUNC(hex_entry_change_cb), ipal);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 1);
  
    /*  Connect the "F1" help key  */
  gimp_help_connect_help_accel (GTK_WIDGET (ipal),
				gimp_standard_help_func,
				"dialogs/indexed_palette.html");

  gtk_widget_show_all (vbox);
  /* gtk_widget_show (ipal); */
  
  ipal_update_image_list (ipal);
  gtk_signal_connect(GTK_OBJECT(context), "add",
		     GTK_SIGNAL_FUNC(set_addrem_cb), ipal);
  gtk_signal_connect(GTK_OBJECT(context), "remove",
		     GTK_SIGNAL_FUNC(set_addrem_cb), ipal);
  return ipal;
}

static void
ipal_update_image_list (GimpColormapDialog* ipal)
{
  int default_index;
  GimpImage* default_gimage;

  default_gimage = ipal->image;
  ipal->image_menu =
  create_image_menu (ipal,
		     &default_gimage,
		     &default_index,
		     image_menu_callback);
  gtk_option_menu_set_menu (ipal->option_menu,
			    ipal->image_menu);
  
  if (default_index != -1)
    {
      if (! GTK_WIDGET_IS_SENSITIVE (ipal->vbox))
	gtk_widget_set_sensitive (ipal->vbox, TRUE);
      gtk_option_menu_set_history (ipal->option_menu,
				   default_index);
    }
  else
    {
      if (GTK_WIDGET_IS_SENSITIVE (ipal->vbox))
	{
	  gtk_widget_set_sensitive (ipal->vbox, FALSE);
	}
    }
  ipal_set_image (ipal, default_gimage);
}

static void
frame_size_alloc_cb(GtkFrame* frame, GtkAllocation* alloc,
		    GimpColormapDialog* ipal){
  GtkWidget* w = GTK_WIDGET(frame);
  GtkAllocation a = *alloc;
  GtkRequisition r;
  if(ipal->image)
    ipal_draw(ipal);
  else
    ipal_clear(ipal);
  w = GTK_BIN(w)->child;
  gtk_widget_size_request(w, &r);
  a.x = MAX(0, a.width - r.width) / 2;
  a.y = MAX(0, a.height - r.height) / 2;
  a.width = MIN(a.width, r.width);
  a.height = MIN(a.height, r.height);
  gtk_widget_size_allocate(w, &a);
}

static void
window_size_req_cb(GtkWindow* win, GtkRequisition* req,
		   GimpColormapDialog* ipal){
  req->width = GTK_WIDGET(win)->allocation.width;
  req->height = GTK_WIDGET(win)->allocation.height;
}

#define MIN_CELL_SIZE 4

static void
ipal_draw (GimpColormapDialog* ipal)
{
  GimpImage *gimage;
  int i, j, k, l, b;
  int col;
  guchar* row;
  gint cellsize, ncol, xn, yn, width, height;
  GtkWidget *palette;
  GtkContainer *parent;
  g_return_if_fail(ipal);
  gimage = ipal->image;

  g_return_if_fail(gimage);

  palette = GTK_WIDGET(ipal->palette);
  parent = GTK_CONTAINER(palette->parent);
  width = GTK_WIDGET(parent)->allocation.width - parent->border_width;
  height = GTK_WIDGET(parent)->allocation.height - parent->border_width;
  ncol = gimage->num_cols;

  if(!ncol){
	  ipal_clear(ipal);
	  return;
  }
  
  cellsize = sqrt(width * height / ncol);
  while(cellsize>= MIN_CELL_SIZE
	&& (xn = width/cellsize) * (yn = height/cellsize) < ncol)
	  cellsize--;

  if(cellsize < MIN_CELL_SIZE){
	  cellsize = MIN_CELL_SIZE;
	  xn = yn = ceil(sqrt(ncol));
  }

  yn = ((ncol + xn - 1) / xn);
  width = xn * cellsize;
  height = yn * cellsize;
  ipal->xn = xn;
  ipal->yn = yn;
  ipal->cellsize = cellsize;
  
  gtk_preview_size(ipal->palette, width, height);
  /*gtk_container_resize_children(GTK_WIDGET(parent)->parent);*/

  /*  req.width = width + parent->border_width;
  req.height = height + parent->border_width;
  gtk_widget_size_request(palette->parent, &req);*/
  /*gtk_widget_queue_resize (GTK_WIDGET(ipal));*/
  /*gtk_container_check_resize (GTK_WIDGET(parent)->parent);*/
  
  
  row = g_new (guchar, xn*cellsize*3);
  col = 0;
  for (i = 0; i < yn; i++)
    {
      for (j = 0; j < xn && col < ncol; j++, col++)
	{
	  for (k = 0; k < cellsize; k++)
	    for (b = 0; b < 3; b++)
	      row[(j * cellsize + k) * 3 + b] = gimage->cmap[col * 3 + b];
	}

      for (k = 0; k < cellsize; k++)
	{
	  for (l = j * cellsize; l < xn * cellsize; l++)
	    for (b = 0; b < 3; b++)
	      row[l * 3 + b] = ((((i * cellsize + k) & 0x4) ? (l) : (l + 0x4)) & 0x4) ?
		blend_light_check[0] : blend_dark_check[0];

	  gtk_preview_draw_row (ipal->palette, row, 0,
				i * cellsize + k, cellsize * xn);
	}
    }
  ipal_draw_cell (ipal, ipal->col_index);

  g_free (row);
  gtk_widget_draw (palette, NULL);
}

static void
ipal_draw_cell (GimpColormapDialog* ipal, gint col)
{
  guchar* row;
  gint cellsize, x, y, k;
  GdkRectangle rec;
  
  g_assert (ipal);
  g_assert (ipal->image);
  g_assert (col < ipal->image->num_cols);
  
  cellsize = ipal->cellsize;
  row = g_new(guchar, cellsize * 3);
  x = (col % ipal->xn) * cellsize;
  y = (col / ipal->xn) * cellsize;

  if(col == ipal->col_index){
    for(k = 0; k < cellsize; k++)
      row[k*3] = row[k*3+1] = row[k*3+2] = (k & 1) * 255;
    gtk_preview_draw_row (ipal->palette, row, x, y, cellsize);
    if(!(cellsize & 1))
      for(k = 0; k < cellsize; k++)
	row[k*3] = row[k*3+1] = row[k*3+2] = ((x+y+1) & 1) * 255;
    gtk_preview_draw_row (ipal->palette, row, x, y+cellsize-1, cellsize);
    row[0]=row[1]=row[2]=255;
    row[cellsize*3-3] = row[cellsize*3-2] = row[cellsize*3-1]
      = 255 * (cellsize & 1);
    for(k = 1; k < cellsize - 1; k++){
      row[k*3] = ipal->image->cmap[col * 3];
      row[k*3+1] = ipal->image->cmap[col * 3 + 1];
      row[k*3+2] = ipal->image->cmap[col * 3 + 2];
    }
    for(k = 1; k < cellsize - 1; k+=2)
      gtk_preview_draw_row (ipal->palette, row, x, y+k, cellsize);
    row[0] = row[1] = row[2] = 0;
    row[cellsize*3-3] = row[cellsize*3-2] = row[cellsize*3-1]
      = 255 * ((cellsize+1) & 1);
    for(k = 2; k < cellsize - 1; k+=2)
      gtk_preview_draw_row (ipal->palette, row, x, y+k, cellsize);
  } else {
    for (k = 0; k < cellsize; k++){
      row[k*3] = ipal->image->cmap[col * 3];
      row[k*3+1] = ipal->image->cmap[col * 3 + 1];
      row[k*3+2] = ipal->image->cmap[col * 3 + 2];
    }
    for (k = 0; k < cellsize; k++)
      gtk_preview_draw_row (ipal->palette, row, x, y+k, cellsize);
  }
  rec.x = x;
  rec.y = y;
  rec.width = rec.height = cellsize;
  gtk_widget_draw(GTK_WIDGET(ipal->palette), &rec);
}
    
static void
ipal_clear (GimpColormapDialog* ipal)
{
  int i, j;
  int offset;
  gint width, height;
  guchar* row = NULL;
  GtkWidget* palette;

  g_return_if_fail(ipal);

  palette = GTK_WIDGET(ipal->palette);

  /* Watch out for negative values (at least on Win32) */
  width = (int) (gint16) palette->allocation.width;
  height = (int) (gint16) palette->allocation.height;
  if (width > 0)
    row = g_new(guchar, width * 3);
  
  gtk_preview_size(ipal->palette, width, height);
  
  for (i = 0; i < height; i += 4)
    {
      offset = (i & 0x4) ? 0x4 : 0x0;

      for (j = 0; j < width; j++)
	{
	  row[j * 3 + 0] = row[j * 3 + 1] = row[j * 3 + 2] =
	    ((j + offset) & 0x4) ? blend_light_check[0] : blend_dark_check[0];
	}

      for (j = 0; j < 4 && i+j < height; j++)
	gtk_preview_draw_row (ipal->palette, row, 0, i + j, width);
    }
  if (width > 0)
    g_free (row);
  gtk_widget_draw (palette, NULL);
}


static void
ipal_update_entries(GimpColormapDialog* ipal)
{
  gchar* s;
  guchar* c;

  if(!ipal->image){
    gtk_widget_set_sensitive(GTK_WIDGET(ipal->index_entry), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(ipal->color_entry), FALSE);
    gtk_entry_set_text(ipal->index_entry, "");
    gtk_entry_set_text(ipal->color_entry, "");
  } else {
    s = g_strdup_printf("%d", ipal->col_index);
    c = &ipal->image->cmap[ipal->col_index * 3];
    gtk_entry_set_text(ipal->index_entry, s);
    g_free(s);
    s = g_strdup_printf("#%02x%02x%02x", c[0], c[1], c[2]);
    gtk_entry_set_text(ipal->color_entry, s);
    g_free(s);
    gtk_widget_set_sensitive(GTK_WIDGET(ipal->index_entry), TRUE);
    gtk_widget_set_sensitive(GTK_WIDGET(ipal->color_entry), TRUE);
  }
}

static void
ipal_set_index (GimpColormapDialog* ipal, gint i)
{
  if(i != ipal->col_index){
    gint old = ipal->col_index;
    ipal->col_index = i;
    ipal_draw_cell (ipal, old);
    ipal_draw_cell (ipal, i);
    ipal_update_entries (ipal);
  }
}


static void
index_entry_change_cb(GtkEntry* entry, GimpColormapDialog* ipal)
{
  gchar* s;
  gint i;

  g_return_if_fail(ipal);
  g_return_if_fail(ipal->image);

  s = gtk_entry_get_text(entry);
  
  if(sscanf(s, "%d", &i) && i>=0 && i<ipal->image->num_cols)
    ipal_set_index (ipal, i);

  ipal_update_entries (ipal);
}

static void
hex_entry_change_cb(GtkEntry* entry, GimpColormapDialog* ipal)
{
  gchar* s;
  gulong i;

  g_return_if_fail(ipal);
  g_return_if_fail(ipal->image);

  s = gtk_entry_get_text(entry);
  
  if(sscanf(s, "#%lx", &i)){
    guchar* c = &ipal->image->cmap[3 * ipal->col_index];
    c[0] = (i & 0xFF0000) >> 16;
    c[1] = (i & 0x00FF00) >> 8;
    c[2] = (i & 0x0000FF);
    gimp_image_colormap_changed (ipal->image, ipal->col_index);
  }
  
  ipal_update_entries (ipal);
}


static void
set_addrem_cb(GimpSet* set, GimpImage* image, GimpColormapDialog* ipal)
{
  ipal_update_image_list(ipal);
}	

static void
image_rename_cb(GimpImage* img, GimpColormapDialog* ipal)
{
  ipal_update_image_list(ipal);
}

static void
image_cmap_change_cb(GimpImage* img, gint ncol, GimpColormapDialog* ipal){

  if(img == ipal->image && gimp_image_base_type(img) == INDEXED){
    if(ncol<0){
      ipal_draw(ipal);
      gtk_container_queue_resize(GTK_CONTAINER(ipal));
    }else{
      ipal_draw_cell(ipal, ncol);
    }
    if(ncol == ipal->col_index)
      ipal_update_entries(ipal);
    gtk_widget_set_sensitive(ipal->add_item, (img->num_cols < 256));
  }else{
    ipal_update_image_list(ipal);
  }
  gdisplays_flush_now();
}
	       
static void
ipal_set_image (GimpColormapDialog* ipal, GimpImage* gimage)
{
  g_assert(ipal);

  if(ipal->image){
    ipal->image = gimage;
    if(!gimage){
      gtk_signal_handler_block(GTK_OBJECT(ipal->palette),
			       ipal->event_handler);
      if(GTK_WIDGET_MAPPED(GTK_WIDGET(ipal)))
	ipal_clear(ipal);
    }
  }

  if(gimage){
    if(!ipal->image)
      gtk_signal_handler_unblock(GTK_OBJECT(ipal->palette),
				 ipal->event_handler);
    g_return_if_fail(gimp_set_have(ipal->context, gimage));
    g_return_if_fail(gimage_base_type (gimage) == INDEXED);
    ipal->image = gimage;
    ipal_draw (ipal);
    gtk_container_queue_resize(GTK_CONTAINER(ipal));
  }else{
    if(ipal->color_notebook)
      color_notebook_hide(ipal->color_notebook);
  }
  
  ipal->col_index = 0;
  gtk_widget_set_sensitive(ipal->add_item,
			   (gimage && gimage->num_cols < 256));
  ipal_update_entries (ipal);
}

static void
ipal_add_callback (GtkWidget *w,
		   gpointer   client_data)
{
  GimpColormapDialog* ipal = client_data;
  g_return_if_fail (ipal);
  g_return_if_fail (ipal->image->num_cols < 256);
  memcpy(&ipal->image->cmap[ipal->image->num_cols * 3],
	 &ipal->image->cmap[ipal->col_index * 3],
	 3);
  ipal->image->num_cols++;
  gimp_image_colormap_changed (ipal->image, -1);
}

static void
ipal_edit_callback (GtkWidget *w,
		   gpointer   client_data)
{
  GimpColormapDialog* ipal = client_data;
  guchar r, g, b;

  g_return_if_fail (ipal);

  r = ipal->image->cmap[ipal->col_index*3];
  g = ipal->image->cmap[ipal->col_index*3+1];
  b = ipal->image->cmap[ipal->col_index*3+2];
  if (! ipal->color_notebook)
    {
      ipal->color_notebook
	= color_notebook_new (r, g, b,
			      ipal_select_callback, ipal, FALSE);
    }
  else
    {
      color_notebook_show (ipal->color_notebook);
      color_notebook_set_color (ipal->color_notebook, r, g, b, 1);
    }
}

static void
ipal_close_callback (GtkWidget *w,
				gpointer   client_data)
{
  GimpColormapDialog* ipal = client_data;
  g_assert(ipal);
  gtk_widget_hide (GTK_WIDGET(ipal));
}

static void
ipal_select_callback (int   r,
		      int   g,
		      int   b,
		      ColorNotebookState state,
		      void *client_data)
{
  GimpImage *gimage;
  GimpColormapDialog* ipal = client_data;
  
  g_return_if_fail (ipal);
  g_return_if_fail (ipal->image);
  g_return_if_fail (ipal->color_notebook);

  gimage = ipal->image;
  
  switch (state) {
  case COLOR_NOTEBOOK_UPDATE:
	  break;
  case COLOR_NOTEBOOK_OK:
	  gimage->cmap[ipal->col_index * 3 + 0] = r;
	  gimage->cmap[ipal->col_index * 3 + 1] = g;
	  gimage->cmap[ipal->col_index * 3 + 2] = b;
	  gimp_image_colormap_changed (gimage, ipal->col_index);
	  /* Fall through */
  case COLOR_NOTEBOOK_CANCEL:
	  color_notebook_hide (ipal->color_notebook);
  }
}

static gint
ipal_area_events (GtkWidget *widget,
		  GdkEvent * event,
		  GimpColormapDialog* ipal)
{
  GimpImage *gimage;
  GdkEventButton *bevent;
  guchar r, g, b;
  guint col;

  g_assert(ipal);
  gimage = ipal->image;
  g_assert (gimage);
  

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      if(bevent->button < 1 || bevent->button > 3)
	return FALSE;
      
      if(!(bevent->y < ipal->cellsize * ipal->yn
	   && bevent->x < ipal->cellsize * ipal->xn))
	return FALSE;

      col = ipal->xn * ((int)bevent->y / ipal->cellsize)
	+ ((int)bevent->x / ipal->cellsize);

      if(col >= ipal->image->num_cols)
	return FALSE;

      r = gimage->cmap[col * 3 + 0];
      g = gimage->cmap[col * 3 + 1];
      b = gimage->cmap[col * 3 + 2];
      ipal_set_index (ipal, col);

      if(bevent->button == 1)
	{
	  gimp_colormap_dialog_selected(ipal);
	}
      
      if (bevent->button == 3)
	{
	  ipal->col_index = col;
	  if (! ipal->color_notebook)
	    {
	      ipal->color_notebook
		= color_notebook_new (r, g, b,
				      ipal_select_callback, ipal, FALSE);
	    }
	  else
	    {
	      color_notebook_show (ipal->color_notebook);
	      color_notebook_set_color (ipal->color_notebook, r, g, b, 1);
	    }
	}
      break;

    default:
      break;
    }

  return FALSE;
}

static void
image_menu_callback (GtkWidget *w,
		     gpointer   client_data)
{
  GimpColormapDialog* ipal = gtk_object_get_data(GTK_OBJECT(w), "colormap_dialog");
  GimpImage* image = GIMP_IMAGE(client_data);
  g_assert(ipal);
  g_assert(image);
  ipal_set_image (ipal, image);
}

typedef struct{
  GimpImage** def;
  int* default_index;
  MenuItemCallback callback;
  GtkWidget* menu;
  int num_items;
  GimpImage* id;
  GimpColormapDialog* ipal;
}IMCBData;

static void
create_image_menu_cb (gpointer im, gpointer d)
{
  GimpImage* gimage = GIMP_IMAGE (im);
  IMCBData* data = (IMCBData*)d;
  char* image_name;
  char* menu_item_label;
  GtkWidget *menu_item;

  if (gimage_base_type(gimage) != INDEXED)
	  return;
  
  /*  make sure the default index gets set to _something_, if possible  */
  if (*data->default_index == -1)
    {
      data->id = gimage;
      *data->default_index = data->num_items;
    }

  if (gimage == *data->def)
    {
      data->id = *data->def;
      *data->default_index = data->num_items;
    }

  image_name = g_basename (gimage_filename (gimage));
  menu_item_label = g_strdup_printf ("%s-%d", image_name, 
                                     pdb_image_to_id (gimage));
  menu_item = gtk_menu_item_new_with_label (menu_item_label);
  gtk_object_set_data(GTK_OBJECT (menu_item), "colormap_dialog", data->ipal);
  gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
		      (GtkSignalFunc) data->callback,
		      gimage);
  gtk_container_add (GTK_CONTAINER (data->menu), menu_item);
  gtk_widget_show (menu_item);

  g_free (menu_item_label);
  data->num_items ++;  
}


static GtkWidget *
create_image_menu (GimpColormapDialog*    ipal,
		   GimpImage**       def,
		   int              *default_index,
		   MenuItemCallback  callback)
{
  IMCBData data;

  data.def = def;
  data.default_index = default_index;
  data.callback = callback;
  data.menu = gtk_menu_new ();
  data.num_items = 0;
  data.id = NULL;
  data.ipal = ipal;

  *default_index = -1;

  gimp_set_foreach (ipal->context, create_image_menu_cb, &data);

  if (!data.num_items)
    {
      GtkWidget* menu_item;
      menu_item = gtk_menu_item_new_with_label (_("none"));
      gtk_container_add (GTK_CONTAINER (data.menu), menu_item);
      gtk_widget_show (menu_item);
    }

  *def = data.id;

  return data.menu;
}
