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

#include "apptypes.h"

#include "appenv.h"
#include "gimage.h"
#include "gimpcontext.h"
#include "gimpimage.h"
#include "gimppalette.h"
#include "gimpui.h"
#include "gradient.h"
#include "gradient_header.h"
#include "gradient_select.h"
#include "palette.h"
#include "palette_import.h"
#include "pixel_region.h"
#include "temp_buf.h"

#include "pdb/procedural_db.h"

#include "libgimp/gimpintl.h"


/* New palette code... */

#define IMPORT_PREVIEW_WIDTH  80
#define IMPORT_PREVIEW_HEIGHT 80
#define MAX_IMAGE_COLORS      (10000*2)

typedef enum
{
  GRAD_IMPORT = 0,
  IMAGE_IMPORT = 1,
  INDEXED_IMPORT = 2
} ImportType;


typedef struct _ImportDialog ImportDialog;

struct _ImportDialog
{
  GtkWidget     *dialog;

  GtkWidget     *preview;
  GtkWidget     *entry;
  GtkWidget     *select_area;
  GtkWidget     *select;
  GtkWidget     *image_list;
  GtkWidget     *image_menu_item_image;
  GtkWidget     *image_menu_item_indexed;
  GtkWidget     *image_menu_item_gradient;
  GtkWidget     *optionmenu1_menu;
  GtkWidget     *type_option;
  GtkWidget     *threshold_scale;
  GtkWidget     *threshold_text;
  GtkAdjustment *threshold;
  GtkAdjustment *sample;
  ImportType     import_type;
  GimpImage     *gimage;
};


static ImportDialog *import_dialog = NULL;


/*****************************************************************************/
/*  palette import dialog functions  *****************************************/

/*  functions to create & update the import dialog's gradient selection  *****/

static void
palette_import_select_grad_callback (GtkWidget *widget,
				     gpointer   data)
{
  /*  Popup grad edit box ....  */
  gradient_dialog_create ();
}

static void
palette_import_fill_grad_preview (GtkWidget  *preview,
				  gradient_t *gradient)
{
  guchar   buffer[3*IMPORT_PREVIEW_WIDTH];
  gint     loop;
  guchar  *p = buffer;
  gdouble  dx, cur_x;
  GimpRGB  color;

  dx    = 1.0/ (IMPORT_PREVIEW_WIDTH - 1);
  cur_x = 0;

  for (loop = 0 ; loop < IMPORT_PREVIEW_WIDTH; loop++)
    {
      gradient_get_color_at (gradient, cur_x, &color);

      *p++ = (guchar) (color.r * 255.999);
      *p++ = (guchar) (color.g * 255.999);
      *p++ = (guchar) (color.b * 255.999);

      cur_x += dx;
    }

  for (loop = 0 ; loop < IMPORT_PREVIEW_HEIGHT; loop++)
    {
      gtk_preview_draw_row (GTK_PREVIEW (preview), buffer, 0, loop,
			    IMPORT_PREVIEW_WIDTH);
    }

  gtk_widget_draw (preview, NULL);
}

static void
palette_import_gradient_update (GimpContext *context,
				gradient_t  *gradient,
				gpointer     data)
{
  if (import_dialog && import_dialog->import_type == GRAD_IMPORT)
    {
      /* redraw gradient */
      palette_import_fill_grad_preview (import_dialog->preview, gradient);
      gtk_entry_set_text (GTK_ENTRY (import_dialog->entry), gradient->name);
    }
}

/*  functions to create & update the import dialog's image selection  ********/

static void
palette_import_gimlist_cb (gpointer im,
			   gpointer data)
{
  GSList **l;

  l = (GSList**) data;
  *l = g_slist_prepend (*l, im);
}

static void
palette_import_gimlist_indexed_cb (gpointer im,
				   gpointer data)
{
  GimpImage  *gimage = GIMP_IMAGE (im);
  GSList    **l;

  if (gimp_image_base_type (gimage) == INDEXED)
    {
      l = (GSList**) data;
      *l = g_slist_prepend (*l, im);
    }
}

static void
palette_import_update_image_preview (GimpImage *gimage)
{
  TempBuf *preview_buf;
  gchar   *src;
  gchar   *buf;
  gint     x, y, has_alpha;
  gint     sel_width, sel_height;
  gint     pwidth, pheight;

  import_dialog->gimage = gimage;

  /* Calculate preview size */

  sel_width = gimage->width;
  sel_height = gimage->height;

  if (sel_width > sel_height)
    {
      pwidth  = MIN (sel_width, IMPORT_PREVIEW_WIDTH);
      pheight = sel_height * pwidth / sel_width;
    }
  else
    {
      pheight = MIN (sel_height, IMPORT_PREVIEW_HEIGHT);
      pwidth  = sel_width * pheight / sel_height;
    }
  
  /* Min size is 2 */
  preview_buf = gimp_image_construct_composite_preview (gimage,
							MAX (pwidth, 2),
							MAX (pheight, 2));

  gtk_preview_size (GTK_PREVIEW (import_dialog->preview),
		    preview_buf->width,
		    preview_buf->height);

  buf = g_new (gchar,  IMPORT_PREVIEW_WIDTH * 3);
  src = (gchar *) temp_buf_data (preview_buf);
  has_alpha = (preview_buf->bytes == 2 || preview_buf->bytes == 4);
  for (y = 0; y <preview_buf->height ; y++)
    {
      if (preview_buf->bytes == (1+has_alpha))
	for (x = 0; x < preview_buf->width; x++)
	  {
	    buf[x*3+0] = src[x];
	    buf[x*3+1] = src[x];
	    buf[x*3+2] = src[x];
	  }
      else
	for (x = 0; x < preview_buf->width; x++)
	  {
	    gint stride = 3 + has_alpha;
	    buf[x*3+0] = src[x*stride+0];
	    buf[x*3+1] = src[x*stride+1];
	    buf[x*3+2] = src[x*stride+2];
	  }
      gtk_preview_draw_row (GTK_PREVIEW (import_dialog->preview),
			    (guchar *)buf, 0, y, preview_buf->width);
      src += preview_buf->width * preview_buf->bytes;
    }

  g_free (buf);
  temp_buf_free (preview_buf);

  gtk_widget_hide (import_dialog->preview);
  gtk_widget_draw (import_dialog->preview, NULL); 
  gtk_widget_show (import_dialog->preview);
}

static void
palette_import_image_sel_callback (GtkWidget *widget,
				   gpointer   data)
{
  GimpImage *gimage;
  gchar     *lab;

  gimage = GIMP_IMAGE (data);
  palette_import_update_image_preview (gimage);

  lab = g_strdup_printf ("%s-%d",
			 g_basename (gimp_image_filename (import_dialog->gimage)),
			 pdb_image_to_id (import_dialog->gimage));

  gtk_entry_set_text (GTK_ENTRY (import_dialog->entry), lab);

  g_free (lab);
}

static void
palette_import_image_menu_add (GimpImage *gimage)
{
  GtkWidget *menuitem;
  gchar     *lab;

  lab= g_strdup_printf ("%s-%d",
			g_basename (gimp_image_filename (gimage)),
			pdb_image_to_id (gimage));

  menuitem = gtk_menu_item_new_with_label (lab);

  g_free (lab);

  gtk_widget_show (menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (palette_import_image_sel_callback),
		      gimage);
  gtk_menu_append (GTK_MENU (import_dialog->optionmenu1_menu), menuitem);
}

/* Last Param gives us control over what goes in the menu on a delete oper */
static void
palette_import_image_menu_activate (gint        redo,
				    ImportType  type,
				    GimpImage  *del_image)
{
  GSList    *list      = NULL;
  gint       num_images;
  GimpImage *last_img  = NULL;
  GimpImage *first_img = NULL;
  gint       act_num   = -1;
  gint       count     = 0;
  gchar     *lab;

  if (! import_dialog)
    return;

  if (import_dialog->import_type == type && !redo)
    return;

  /* Destroy existing widget if necessary */
  if (import_dialog->image_list)
    {
      if (redo) /* Preserve settings in this case */
        last_img = import_dialog->gimage;
      gtk_widget_hide (import_dialog->image_list);
      gtk_widget_destroy (import_dialog->image_list);
      import_dialog->image_list = NULL;
    }

  import_dialog->import_type= type;

  /* Get list of images */
  if (import_dialog->import_type == INDEXED_IMPORT)
    {
      gimage_foreach (palette_import_gimlist_indexed_cb, &list);
    }
  else
    {
      gimage_foreach (palette_import_gimlist_cb, &list);
    }

  num_images = g_slist_length (list);
      
  if (num_images)
    {
      GtkWidget *optionmenu1;
      GtkWidget *optionmenu1_menu;
      gint       i;

      import_dialog->image_list = optionmenu1 = gtk_option_menu_new ();
      gtk_widget_set_usize (optionmenu1, IMPORT_PREVIEW_WIDTH, -1);
      import_dialog->optionmenu1_menu = optionmenu1_menu = gtk_menu_new ();

      for (i = 0; i < num_images; i++, list = g_slist_next (list))
        {
          if (GIMP_IMAGE (list->data) != del_image)
            {
	      if (first_img == NULL)
	        first_img = GIMP_IMAGE (list->data);
	      palette_import_image_menu_add (GIMP_IMAGE (list->data));
	      if (last_img == GIMP_IMAGE (list->data))
	        act_num = count;
	      else
	        count++;
	    }
	}

      gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu1),
				optionmenu1_menu);
      gtk_widget_hide (import_dialog->select);
      gtk_box_pack_start (GTK_BOX (import_dialog->select_area),
            		  optionmenu1, FALSE, FALSE, 0);

      if(last_img != NULL && last_img != del_image)
        palette_import_update_image_preview (last_img);
      else if (first_img != NULL)
	palette_import_update_image_preview (first_img);

      gtk_widget_show (optionmenu1);

      /* reset to last one */
      if (redo && act_num >= 0)
        {
           gchar *lab;

	   lab = g_strdup_printf ("%s-%d",
				  g_basename (gimp_image_filename (import_dialog->gimage)),
				  pdb_image_to_id (import_dialog->gimage));

           gtk_option_menu_set_history (GTK_OPTION_MENU (optionmenu1), act_num);
           gtk_entry_set_text (GTK_ENTRY (import_dialog->entry), lab);

	   g_free (lab);
	}
    }
  g_slist_free (list);

  lab = g_strdup_printf ("%s-%d",
			 g_basename (gimp_image_filename (import_dialog->gimage)),
			 pdb_image_to_id (import_dialog->gimage));

  gtk_entry_set_text (GTK_ENTRY (import_dialog->entry), lab);

  g_free (lab);
}

/*  the import source menu item callbacks  ***********************************/

static void
palette_import_grad_callback (GtkWidget *widget,
			      gpointer   data)
{
  if (import_dialog)
    {
      gradient_t *gradient;

      gradient = gimp_context_get_gradient (gimp_context_get_user ());

      import_dialog->import_type = GRAD_IMPORT;
      if (import_dialog->image_list)
	{
	  gtk_widget_hide (import_dialog->image_list);
	  gtk_widget_destroy (import_dialog->image_list);
	  import_dialog->image_list = NULL;
	}
      gtk_widget_show (import_dialog->select);
      palette_import_fill_grad_preview (import_dialog->preview, gradient);

      gtk_entry_set_text (GTK_ENTRY (import_dialog->entry), gradient->name);
      gtk_widget_set_sensitive (import_dialog->threshold_scale, FALSE);
      gtk_widget_set_sensitive (import_dialog->threshold_text, FALSE);
    }
}

static void
palette_import_image_callback (GtkWidget *widget,
			       gpointer   data)
{
  palette_import_image_menu_activate (FALSE, IMAGE_IMPORT, NULL);
  gtk_widget_set_sensitive (import_dialog->threshold_scale, TRUE);
  gtk_widget_set_sensitive (import_dialog->threshold_text, TRUE);
}

static void
palette_import_indexed_callback (GtkWidget *widget,
				 gpointer   data)
{
  palette_import_image_menu_activate (FALSE, INDEXED_IMPORT, NULL);
  gtk_widget_set_sensitive (import_dialog->threshold_scale, FALSE);
  gtk_widget_set_sensitive (import_dialog->threshold_text, FALSE);
}

/*  functions & callbacks to keep the import dialog uptodate  ****************/

static gint
palette_import_image_count (ImportType type)
{
  GSList *list       = NULL;
  gint    num_images = 0;

  if (type == INDEXED_IMPORT)
    {
      gimage_foreach (palette_import_gimlist_indexed_cb, &list);
    }
  else
    {
      gimage_foreach (palette_import_gimlist_cb, &list);
    }

  num_images = g_slist_length (list);

  g_slist_free (list);

  return num_images;
}

static void
palette_import_image_new (GimpSet   *set,
			  GimpImage *gimage,
			  gpointer   data)
{
  if (! import_dialog)
    return;

  if (! GTK_WIDGET_IS_SENSITIVE (import_dialog->image_menu_item_image))
    {
      gtk_widget_set_sensitive (import_dialog->image_menu_item_image, TRUE);
      return;
    }

  if (! GTK_WIDGET_IS_SENSITIVE (import_dialog->image_menu_item_indexed) &&
      gimp_image_base_type(gimage) == INDEXED)
    {
      gtk_widget_set_sensitive (import_dialog->image_menu_item_indexed, TRUE);
      return;
    }

  /* Now fill in the names if image menu shown */
  if (import_dialog->import_type == IMAGE_IMPORT ||
      import_dialog->import_type == INDEXED_IMPORT)
    {
      palette_import_image_menu_activate (TRUE, import_dialog->import_type,
					  NULL);
    }
}

static void
palette_import_image_destroyed (GimpSet   *set,
				GimpImage *gimage,
				gpointer   data)
{
  if (! import_dialog)
    return;

  if (palette_import_image_count (import_dialog->import_type) <= 1)
    {
      /* Back to gradient type */
      gtk_option_menu_set_history (GTK_OPTION_MENU (import_dialog->type_option), 0);
      palette_import_grad_callback (NULL, NULL);
      if (import_dialog->image_menu_item_image)
	gtk_widget_set_sensitive (import_dialog->image_menu_item_image, FALSE);
      return;
    }

  if (import_dialog->import_type == IMAGE_IMPORT ||
      import_dialog->import_type == INDEXED_IMPORT)
    {
      palette_import_image_menu_activate (TRUE, import_dialog->import_type,
					  gimage);
    }
}

void
palette_import_image_renamed (GimpImage* gimage)
{
  /* Now fill in the names if image menu shown */
  if (import_dialog &&
      (import_dialog->import_type == IMAGE_IMPORT ||
       import_dialog->import_type == INDEXED_IMPORT))
    {
      palette_import_image_menu_activate (TRUE, import_dialog->import_type,
					  NULL);
    }
}

/*  create a palette from a gradient  ****************************************/

static void
palette_import_create_from_grad (gchar *name)
{
  GimpPalette *palette;
  gradient_t  *gradient;

  gradient = gimp_context_get_gradient (gimp_context_get_user ());

  if (gradient)
    {
      /* Add names to entry */
      gdouble dx, cur_x;
      GimpRGB color;
      gint    sample_sz;
      gint    loop;

      palette = gimp_palette_new (name);

      sample_sz = (gint) import_dialog->sample->value;  

      dx    = 1.0 / (sample_sz - 1);
      cur_x = 0;
      
      for (loop = 0; loop < sample_sz; loop++)
	{
	  gradient_get_color_at (gradient, cur_x, &color);

	  cur_x += dx;
	  gimp_palette_add_entry (palette, NULL, &color);
	}

      palettes_list_insert (palette);
    }
}

/*  create a palette from a non-indexed image  *******************************/

typedef struct _ImgColors ImgColors;

struct _ImgColors
{
  guint  count;
  guint  r_adj;
  guint  g_adj;
  guint  b_adj;
  guchar r;
  guchar g;
  guchar b;
};

static gint count_color_entries = 0;

static GHashTable *
palette_import_store_colors (GHashTable *h_array, 
			     guchar     *colors,
			     guchar     *colors_real, 
			     gint        sample_sz)
{
  gpointer   found_color = NULL;
  ImgColors *new_color;
  guint      key_colors = colors[0] * 256 * 256 + colors[1] * 256 + colors[2];

  if (h_array == NULL)
    {
      h_array = g_hash_table_new (g_direct_hash, g_direct_equal);
      count_color_entries = 0;
    }
  else
    {
      found_color = g_hash_table_lookup (h_array, (gpointer) key_colors);
    }

  if (found_color == NULL)
    {
      if (count_color_entries > MAX_IMAGE_COLORS)
	{
	  /* Don't add any more new ones */
	  return h_array;
	}

      count_color_entries++;

      new_color = g_new (ImgColors, 1);

      new_color->count = 1;
      new_color->r_adj = 0;
      new_color->g_adj = 0;
      new_color->b_adj = 0;
      new_color->r = colors[0];
      new_color->g = colors[1];
      new_color->b = colors[2];

      g_hash_table_insert (h_array, (gpointer) key_colors, new_color);
    }
  else
    {
      new_color = (ImgColors *) found_color;
      if(new_color->count < (G_MAXINT - 1))
	new_color->count++;
      
      /* Now do the adjustments ...*/
      new_color->r_adj += (colors_real[0] - colors[0]);
      new_color->g_adj += (colors_real[1] - colors[1]);
      new_color->b_adj += (colors_real[2] - colors[2]);

      /* Boundary conditions */
      if(new_color->r_adj > (G_MAXINT - 255))
	new_color->r_adj /= new_color->count;

      if(new_color->g_adj > (G_MAXINT - 255))
	new_color->g_adj /= new_color->count;

      if(new_color->b_adj > (G_MAXINT - 255))
	new_color->b_adj /= new_color->count;
    }

  return h_array;
}

static void
palette_import_create_sorted_list (gpointer key,
				   gpointer value,
				   gpointer user_data)
{
  GSList    **sorted_list = (GSList**) user_data;
  ImgColors  *color_tab   = (ImgColors *) value;

  *sorted_list = g_slist_prepend (*sorted_list, color_tab);
}

static gint
palette_import_sort_colors (gconstpointer a,
			    gconstpointer b)
{
  ImgColors *s1 = (ImgColors *) a;
  ImgColors *s2 = (ImgColors *) b;

  if(s1->count > s2->count)
    return -1;
  if(s1->count < s2->count)
    return 1;

  return 0;
}

static void
palette_import_create_image_palette (gpointer data,
				     gpointer user_data)
{
  GimpPalette *palette;
  ImgColors   *color_tab;
  gint         sample_sz;
  gchar       *lab;
  GimpRGB      color;

  palette   = (GimpPalette *) user_data;
  color_tab = (ImgColors *) data;

  sample_sz = (gint) import_dialog->sample->value;  

  if (palette->n_colors >= sample_sz)
    return;

  lab = g_strdup_printf ("%s (occurs %u)", _("Untitled"), color_tab->count);

  /* Adjust the colors to the mean of the the sample */
  gimp_rgba_set_uchar
    (&color,
     (guchar) color_tab->r + (color_tab->r_adj / color_tab->count), 
     (guchar) color_tab->g + (color_tab->g_adj / color_tab->count), 
     (guchar) color_tab->b + (color_tab->b_adj / color_tab->count),
     255);

  gimp_palette_add_entry (palette, lab, &color);

  g_free (lab);
}

static gboolean
palette_import_color_print_remove (gpointer key,
				   gpointer value,
				   gpointer user_data)
{
  g_free (value);

  return TRUE;
}

static void
palette_import_image_make_palette (GHashTable *h_array,
				   guchar     *name)
{
  GimpPalette *palette;
  GSList      *sorted_list = NULL;

  g_hash_table_foreach (h_array, palette_import_create_sorted_list,
			&sorted_list);
  sorted_list = g_slist_sort (sorted_list, palette_import_sort_colors);

  palette = gimp_palette_new (name);

  g_slist_foreach (sorted_list, palette_import_create_image_palette, palette);

  /*  Free up used memory
   *  Note the same structure is on both the hash list and the sorted
   *  list. So only delete it once.
   */
  g_hash_table_freeze (h_array);
  g_hash_table_foreach_remove (h_array,
			       palette_import_color_print_remove, NULL);
  g_hash_table_thaw (h_array);
  g_hash_table_destroy (h_array);
  g_slist_free (sorted_list);

  palettes_list_insert (palette);
}

static void
palette_import_create_from_image (GImage *gimage,
				  gchar  *pname)
{
  PixelRegion  imagePR;
  guchar      *image_data;
  guchar      *idata;
  guchar       rgb[MAX_CHANNELS];
  guchar       rgb_real[MAX_CHANNELS];
  gint         has_alpha, indexed;
  gint         width, height;
  gint         bytes, alpha;
  gint         i, j;
  void        *pr;
  gint         d_type;
  GHashTable  *store_array = NULL;
  gint         sample_sz;
  gint         threshold = 1;

  sample_sz = (gint) import_dialog->sample->value;  

  if (gimage == NULL)
    return;

  /*  Get the image information  */
  bytes = gimp_image_composite_bytes (gimage);
  d_type = gimp_image_composite_type (gimage);
  has_alpha = (d_type == RGBA_GIMAGE ||
	       d_type == GRAYA_GIMAGE ||
	       d_type == INDEXEDA_GIMAGE);
  indexed = d_type == INDEXEDA_GIMAGE || d_type == INDEXED_GIMAGE;
  width = gimage->width;
  height = gimage->height;
  pixel_region_init (&imagePR, gimp_image_composite (gimage), 0, 0,
		     width, height, FALSE);

  alpha = bytes - 1;

  threshold = (gint) import_dialog->threshold->value;

  if(threshold < 1)
    threshold = 1;

  /*  iterate over the entire image  */
  for (pr = pixel_regions_register (1, &imagePR);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      image_data = imagePR.data;

      for (i = 0; i < imagePR.h; i++)
	{
	  idata = image_data;

	  for (j = 0; j < imagePR.w; j++)
	    {
	      /*  Get the rgb values for the color  */
	      gimp_image_get_color (gimage, d_type, rgb, idata);
	      memcpy (rgb_real, rgb, MAX_CHANNELS); /* Structure copy */

	      rgb[0] = (rgb[0] / threshold) * threshold;
	      rgb[1] = (rgb[1] / threshold) * threshold;
	      rgb[2] = (rgb[2] / threshold) * threshold;

	      store_array =
		palette_import_store_colors (store_array, rgb, rgb_real,
					     sample_sz);

	      idata += bytes;
	    }

	  image_data += imagePR.rowstride;
	}
    }

  /*  Make palette from the store_array  */
  palette_import_image_make_palette (store_array, pname);
}

/*  create a palette from an indexed image  **********************************/

static void
palette_import_create_from_indexed (GImage *gimage,
				    gchar  *pname)
{
  GimpPalette *palette;
  gint         samples;
  gint         count;
  GimpRGB      color;

  samples = (gint) import_dialog->sample->value;  

  if (gimage == NULL)
    return;

  if (gimp_image_base_type (gimage) != INDEXED)
    return;

  palette = gimp_palette_new (pname);

  for (count= 0; count < samples && count < gimage->num_cols; ++count)
    {
      gimp_rgba_set_uchar (&color,
			   gimage->cmap[count*3],
			   gimage->cmap[count*3+1],
			   gimage->cmap[count*3+2],
			   255);

      gimp_palette_add_entry (palette, NULL, &color);
    }

  palettes_list_insert (palette);
}

/*  the palette import action area callbacks  ********************************/

static void
palette_import_close_callback (GtkWidget *widget,
			       gpointer   data)
{
  gtk_widget_destroy (import_dialog->dialog);
  g_free (import_dialog);
  import_dialog = NULL;
}

static void
palette_import_import_callback (GtkWidget *widget,
				gpointer   data)
{
  gchar *pname;

  if (! import_dialog)
    return;

  pname = gtk_entry_get_text (GTK_ENTRY (import_dialog->entry));
  if (!pname || !strlen (pname))
    pname = g_strdup ("tmp");
  else
    pname = g_strdup (pname);

  switch (import_dialog->import_type)
    {
    case GRAD_IMPORT:
      palette_import_create_from_grad (pname);
      break;
    case IMAGE_IMPORT:
      palette_import_create_from_image (import_dialog->gimage, pname);
      break;
    case INDEXED_IMPORT:
      palette_import_create_from_indexed (import_dialog->gimage, pname);
      break;
    default:
      break;
    }

  palette_import_close_callback (NULL, NULL);
}

/*  the palette import dialog constructor  ***********************************/

static ImportDialog *
palette_import_dialog_new (void)
{
  GtkWidget *dialog;
  GtkWidget *hbox;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *spinbutton;
  GtkWidget *button;
  GtkWidget *entry;
  GtkWidget *optionmenu;
  GtkWidget *optionmenu_menu;
  GtkWidget *menuitem;
  GtkWidget *image;
  GtkWidget *hscale;

  import_dialog = g_new (ImportDialog, 1);
  import_dialog->image_list = NULL;
  import_dialog->gimage     = NULL;

  import_dialog->dialog = dialog =
    gimp_dialog_new (_("Import Palette"), "import_palette",
		     gimp_standard_help_func,
		     "dialogs/palette_editor/import_palette.html",
		     GTK_WIN_POS_NONE,
		     FALSE, TRUE, FALSE,

		     _("Import"), palette_import_import_callback,
		     import_dialog, NULL, NULL, FALSE, FALSE,
		     _("Close"), palette_import_close_callback,
		     import_dialog, NULL, NULL, TRUE, TRUE,

		     NULL);

  /*  The main hbox  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hbox);
  gtk_widget_show (hbox);

  /*  The "Import" frame  */
  frame = gtk_frame_new (_("Import"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  table = gtk_table_new (4, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*  The source's name  */
  label = gtk_label_new (_("Name:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND, 0, 0);
  gtk_widget_show (label);

  entry = import_dialog->entry = gtk_entry_new ();
  gtk_table_attach_defaults (GTK_TABLE (table), entry, 1, 2, 0, 1);
  {
    gradient_t* gradient;

    gradient = gimp_context_get_gradient (gimp_context_get_current ());
    gtk_entry_set_text (GTK_ENTRY (entry),
			gradient ? gradient->name : _("new_import"));
  }
  gtk_widget_show (entry);

  /*  The source type  */
  label = gtk_label_new (_("Source:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND, 0, 0);
  gtk_widget_show (label);

  optionmenu = import_dialog->type_option = gtk_option_menu_new ();
  optionmenu_menu = gtk_menu_new ();
  gtk_table_attach_defaults (GTK_TABLE (table), optionmenu, 1, 2, 1, 2);
  menuitem = import_dialog->image_menu_item_gradient = 
    gtk_menu_item_new_with_label (_("Gradient"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (palette_import_grad_callback),
		      NULL);
  gtk_menu_append (GTK_MENU (optionmenu_menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = import_dialog->image_menu_item_image =
    gtk_menu_item_new_with_label (_("Image"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (palette_import_image_callback),
		      import_dialog);
  gtk_menu_append (GTK_MENU (optionmenu_menu), menuitem);
  gtk_widget_show (menuitem);
  gtk_widget_set_sensitive (menuitem,
			    palette_import_image_count (IMAGE_IMPORT) > 0);

  menuitem = import_dialog->image_menu_item_indexed =
    gtk_menu_item_new_with_label (_("Indexed Palette"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
                      GTK_SIGNAL_FUNC (palette_import_indexed_callback),
                      import_dialog);
  gtk_menu_append (GTK_MENU (optionmenu_menu), menuitem);
  gtk_widget_show (menuitem);
  gtk_widget_set_sensitive (menuitem,
			    palette_import_image_count (INDEXED_IMPORT) > 0);

  gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), optionmenu_menu);
  gtk_widget_show (optionmenu);

  /*  The sample size  */
  label = gtk_label_new (_("Sample Size:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND, 0, 0);
  gtk_widget_show (label);

  import_dialog->sample =
    GTK_ADJUSTMENT(gtk_adjustment_new (256, 2, 10000, 1, 10, 10));
  spinbutton = gtk_spin_button_new (import_dialog->sample, 1, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), spinbutton, 1, 2, 2, 3);
  gtk_widget_show (spinbutton);

  /*  The interval  */
  label = import_dialog->threshold_text = gtk_label_new (_("Interval:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_set_sensitive(label, FALSE);
  gtk_widget_show (label);

  import_dialog->threshold = 
    GTK_ADJUSTMENT (gtk_adjustment_new (1, 1, 128, 1, 1, 1));
  hscale = import_dialog->threshold_scale = 
    gtk_hscale_new (import_dialog->threshold);
  gtk_scale_set_value_pos (GTK_SCALE (hscale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (hscale), 0);
  gtk_table_attach_defaults (GTK_TABLE (table), hscale, 1, 2, 3, 4);
  gtk_widget_set_sensitive (hscale, FALSE);
  gtk_widget_show (hscale);

  /*  The preview frame  */
  frame = gtk_frame_new (_("Preview"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = import_dialog->select_area = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  image = import_dialog->preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_set_dither (GTK_PREVIEW (image), GDK_RGB_DITHER_MAX);
  gtk_preview_size (GTK_PREVIEW (image),
		    IMPORT_PREVIEW_WIDTH, IMPORT_PREVIEW_HEIGHT);
  gtk_widget_set_usize (image, IMPORT_PREVIEW_WIDTH, IMPORT_PREVIEW_HEIGHT);
  gtk_box_pack_start (GTK_BOX (vbox), image, FALSE, FALSE, 0);
  gtk_widget_show (image);

  button = import_dialog->select = gtk_button_new_with_label (_("Select"));
  GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", 
		      GTK_SIGNAL_FUNC (palette_import_select_grad_callback),
		      image);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /*  Fill with the selected gradient  */
  palette_import_fill_grad_preview
    (image, gimp_context_get_gradient (gimp_context_get_user ()));
  import_dialog->import_type = GRAD_IMPORT;
  gtk_signal_connect (GTK_OBJECT (gimp_context_get_user ()), "gradient_changed",
		      GTK_SIGNAL_FUNC (palette_import_gradient_update),
		      NULL);

  /*  keep the dialog up-to-date  */
  gtk_signal_connect (GTK_OBJECT (image_context), "add",
		      GTK_SIGNAL_FUNC (palette_import_image_new),
		      NULL);
  gtk_signal_connect (GTK_OBJECT (image_context), "remove",
		      GTK_SIGNAL_FUNC (palette_import_image_destroyed),
		      NULL);

  return import_dialog;
}

void
palette_import_dialog_show (void)
{
  if (! import_dialog)
    palette_import_dialog_new ();

  if (! GTK_WIDGET_VISIBLE (import_dialog->dialog))
    {
      gtk_widget_show (import_dialog->dialog);
    }
  else if (import_dialog->dialog->window)
    {
      gdk_window_raise (import_dialog->dialog->window);
    }
}

void
palette_import_dialog_destroy (void)
{
  if (import_dialog)
    {
      gtk_widget_destroy (import_dialog->dialog);
      g_free (import_dialog);
      import_dialog = NULL;
    }
}
