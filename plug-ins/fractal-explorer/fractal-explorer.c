/**********************************************************************
    Fractal Explorer Plug-in (Version 2.00 Beta 2)
    Daniel Cotting (cotting@multimania.com)
 **********************************************************************
 **********************************************************************
    Official homepages: http://www.multimania.com/cotting
                        http://cotting.citeweb.net
 *********************************************************************/

/**********************************************************************
   GIMP - The GNU Image Manipulation Program
   Copyright (C) 1995 Spencer Kimball and Peter Mattis

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *********************************************************************/

/**********************************************************************
   Some code has been 'stolen' from:
        - Peter Kirchgessner (Pkirchg@aol.com)
        - Scott Draves (spot@cs.cmu.edu)
        - Andy Thomas (alt@picnic.demon.co.uk)
           .
           .
           .
 **********************************************************************
   "If you steal from one author it's plagiarism; if you steal from
   many it's research."  --Wilson Mizner
 *********************************************************************/


/**********************************************************************
 Include necessary files
 *********************************************************************/

#include "config.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib/gstdio.h>

#ifdef G_OS_WIN32
#include <libgimpbase/gimpwin32-io.h>
#endif

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "fractal-explorer.h"
#include "fractal-explorer-dialogs.h"

#include "libgimp/stdplugins-intl.h"


/**********************************************************************
  Global variables
 *********************************************************************/

gdouble              xmin = -2;
gdouble              xmax = 1;
gdouble              ymin = -1.5;
gdouble              ymax = 1.5;
gdouble              xbild;
gdouble              ybild;
gdouble              xdiff;
gdouble              ydiff;
gint                 sel_x;
gint                 sel_y;
gint                 preview_width;
gint                 preview_height;
gdouble             *gg;
gint                 line_no;
gchar               *filename;
clrmap               colormap;
vlumap               valuemap;
gchar               *fractalexplorer_path = NULL;

static gfloat        cx = -0.75;
static gfloat        cy = -0.2;
gint32               drawable_id;
static GList        *fractalexplorer_list = NULL;

explorer_interface_t wint =
{
    NULL,                       /* preview */
    NULL,                       /* wimage */
    FALSE                       /* run */
};                              /* wint */

explorer_vals_t wvals =
{
  0,
  -2.0,
  2.0,
  -1.5,
  1.5,
  50.0,
  -0.75,
  -0.2,
  0,
  1.0,
  1.0,
  1.0,
  1,
  1,
  0,
  0,
  0,
  0,
  1,
  256,
  0,
  0
};                              /* wvals */

fractalexplorerOBJ *current_obj   = NULL;
static GtkWidget   *delete_dialog = NULL;

static void query (void);
static void run   (const gchar      *name,
                   gint              nparams,
                   const GimpParam  *param,
                   gint             *nreturn_vals,
                   GimpParam       **return_vals);

static void explorer            (gint32 drawable_id);

/**********************************************************************
 Declare local functions
 *********************************************************************/

/* Functions for dialog widgets */

static void       delete_dialog_callback           (GtkWidget          *widget,
                                                    gboolean            value,
                                                    gpointer            data);
static gboolean   delete_fractal_callback          (GtkWidget          *widget,
                                                    gpointer            data);
static gint       fractalexplorer_list_pos         (fractalexplorerOBJ *feOBJ);
static gint       fractalexplorer_list_insert      (fractalexplorerOBJ *feOBJ);
static fractalexplorerOBJ *fractalexplorer_new     (void);
static void       fill_list_store                  (GtkListStore       *list_store);
static void       activate_fractal                 (fractalexplorerOBJ *sel_obj);
static void       activate_fractal_callback        (GtkTreeView        *view,
                                                    GtkTreePath        *path,
                                                    GtkTreeViewColumn  *col,
                                                    gpointer            data);
static gboolean   apply_fractal_callback           (GtkWidget          *widget,
                                                    gpointer            data);

static void       fractalexplorer_free             (fractalexplorerOBJ *feOBJ);
static void       fractalexplorer_free_everything  (fractalexplorerOBJ *feOBJ);
static void       fractalexplorer_list_free_all    (void);
static fractalexplorerOBJ * fractalexplorer_load   (const gchar *filename,
                                                    const gchar *name);

static void       fractalexplorer_list_load_all    (const gchar *path);
static void       fractalexplorer_rescan_list      (GtkWidget *widget,
                                                    gpointer   data);

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

/**********************************************************************
 MAIN()
 *********************************************************************/

MAIN()

/**********************************************************************
 FUNCTION: query
 *********************************************************************/

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32, "run-mode", "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE, "image", "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable" },
    { GIMP_PDB_INT8, "fractaltype", "0: Mandelbrot; "
                                    "1: Julia; "
                                    "2: Barnsley 1; "
                                    "3: Barnsley 2; "
                                    "4: Barnsley 3; "
                                    "5: Spider; "
                                    "6: ManOWar; "
                                    "7: Lambda; "
                                    "8: Sierpinski" },
    { GIMP_PDB_FLOAT, "xmin", "xmin fractal image delimiter" },
    { GIMP_PDB_FLOAT, "xmax", "xmax fractal image delimiter" },
    { GIMP_PDB_FLOAT, "ymin", "ymin fractal image delimiter" },
    { GIMP_PDB_FLOAT, "ymax", "ymax fractal image delimiter" },
    { GIMP_PDB_FLOAT, "iter", "Iteration value" },
    { GIMP_PDB_FLOAT, "cx", "cx value ( only Julia)" },
    { GIMP_PDB_FLOAT, "cy", "cy value ( only Julia)" },
    { GIMP_PDB_INT8, "colormode", "0: Apply colormap as specified by the parameters below; "
                                  "1: Apply active gradient to final image" },
    { GIMP_PDB_FLOAT, "redstretch", "Red stretching factor" },
    { GIMP_PDB_FLOAT, "greenstretch", "Green stretching factor" },
    { GIMP_PDB_FLOAT, "bluestretch", "Blue stretching factor" },
    { GIMP_PDB_INT8, "redmode", "Red application mode (0:SIN;1:COS;2:NONE)" },
    { GIMP_PDB_INT8, "greenmode", "Green application mode (0:SIN;1:COS;2:NONE)" },
    { GIMP_PDB_INT8, "bluemode", "Blue application mode (0:SIN;1:COS;2:NONE)" },
    { GIMP_PDB_INT8, "redinvert", "Red inversion mode (1: enabled; 0: disabled)" },
    { GIMP_PDB_INT8, "greeninvert", "Green inversion mode (1: enabled; 0: disabled)" },
    { GIMP_PDB_INT8, "blueinvert", "Green inversion mode (1: enabled; 0: disabled)" },
    { GIMP_PDB_INT32, "ncolors", "Number of Colors for mapping (2<=ncolors<=8192)" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Render fractal art"),
                          "No help yet.",
                          "Daniel Cotting (cotting@multimania.com, www.multimania.com/cotting)",
                          "Daniel Cotting (cotting@multimania.com, www.multimania.com/cotting)",
                          "December, 1998",
                          N_("_Fractal Explorer..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Render/Fractals");
}

/**********************************************************************
 FUNCTION: run
 *********************************************************************/

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[1];
  GimpRunMode        run_mode;
  gint               pwidth;
  gint               pheight;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  gint               sel_width;
  gint               sel_height;

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  *nreturn_vals = 1;
  *return_vals = values;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  run_mode    = param[0].data.d_int32;
  drawable_id = param[2].data.d_drawable;

  if (! gimp_drawable_mask_intersect (drawable_id,
                                      &sel_x, &sel_y,
                                      &sel_width, &sel_height))
    return;

  /* Calculate preview size */
  if (sel_width > sel_height)
    {
      pwidth  = MIN (sel_width, PREVIEW_SIZE);
      pheight = sel_height * pwidth / sel_width;
    }
  else
    {
      pheight = MIN (sel_height, PREVIEW_SIZE);
      pwidth  = sel_width * pheight / sel_height;
    }

  preview_width  = MAX (pwidth, 2);
  preview_height = MAX (pheight, 2);

  /* See how we will run */
  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /* Possibly retrieve data */
      gimp_get_data ("plug_in_fractalexplorer", &wvals);

      /* Get information from the dialog */
      if (!explorer_dialog ())
        return;

      break;

    case GIMP_RUN_NONINTERACTIVE:
      /* Make sure all the arguments are present */
      if (nparams != 22)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      else
        {
          wvals.fractaltype  = param[3].data.d_int8;
          wvals.xmin         = param[4].data.d_float;
          wvals.xmax         = param[5].data.d_float;
          wvals.ymin         = param[6].data.d_float;
          wvals.ymax         = param[7].data.d_float;
          wvals.iter         = param[8].data.d_float;
          wvals.cx           = param[9].data.d_float;
          wvals.cy           = param[10].data.d_float;
          wvals.colormode    = param[11].data.d_int8;
          wvals.redstretch   = param[12].data.d_float;
          wvals.greenstretch = param[13].data.d_float;
          wvals.bluestretch  = param[14].data.d_float;
          wvals.redmode      = param[15].data.d_int8;
          wvals.greenmode    = param[16].data.d_int8;
          wvals.bluemode     = param[17].data.d_int8;
          wvals.redinvert    = param[18].data.d_int8;
          wvals.greeninvert  = param[19].data.d_int8;
          wvals.blueinvert   = param[20].data.d_int8;
          wvals.ncolors      = CLAMP (param[21].data.d_int32, 2, MAXNCOLORS);
        }
      make_color_map();
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /* Possibly retrieve data */
      gimp_get_data ("plug_in_fractalexplorer", &wvals);
      make_color_map ();
      break;

    default:
      break;
    }

  xmin = wvals.xmin;
  xmax = wvals.xmax;
  ymin = wvals.ymin;
  ymax = wvals.ymax;
  cx = wvals.cx;
  cy = wvals.cy;

  if (status == GIMP_PDB_SUCCESS)
    {
      gimp_progress_init (_("Rendering fractal"));

      explorer (drawable_id);

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();

      /* Store data */
      if (run_mode == GIMP_RUN_INTERACTIVE)
        gimp_set_data ("plug_in_fractalexplorer",
                       &wvals, sizeof (explorer_vals_t));
    }

  values[0].data.d_status = status;
}

/**********************************************************************
 FUNCTION: explorer
 *********************************************************************/

static void
explorer (gint32 drawable_id)
{
  GeglBuffer *src_buffer;
  GeglBuffer *dest_buffer;
  const Babl *format;
  gint        width;
  gint        height;
  gint        bpp;
  gint        row;
  gint        x;
  gint        y;
  gint        w;
  gint        h;
  guchar     *src_row;
  guchar     *dest_row;

  /* Get the input area. This is the bounding box of the selection in
   *  the image (or the entire image if there is no selection). Only
   *  operating on the input area is simply an optimization. It doesn't
   *  need to be done for correct operation. (It simply makes it go
   *  faster, since fewer pixels need to be operated on).
   */
  if (! gimp_drawable_mask_intersect (drawable_id, &x, &y, &w, &h))
    return;

  /* Get the size of the input image. (This will/must be the same
   *  as the size of the output image.
   */
  width  = gimp_drawable_width  (drawable_id);
  height = gimp_drawable_height (drawable_id);

  if (gimp_drawable_has_alpha (drawable_id))
    format = babl_format ("R'G'B'A u8");
  else
    format = babl_format ("R'G'B' u8");

  bpp = babl_format_get_bytes_per_pixel (format);

  /*  allocate row buffers  */
  src_row  = g_new (guchar, bpp * w);
  dest_row = g_new (guchar, bpp * w);

  src_buffer  = gimp_drawable_get_buffer (drawable_id);
  dest_buffer = gimp_drawable_get_shadow_buffer (drawable_id);

  xbild = width;
  ybild = height;
  xdiff = (xmax - xmin) / xbild;
  ydiff = (ymax - ymin) / ybild;

  for (row = y; row < y + h; row++)
    {
      gegl_buffer_get (src_buffer, GEGL_RECTANGLE (x, row, w, 1), 1.0,
                       format, src_row,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      explorer_render_row (src_row,
                           dest_row,
                           row,
                           w,
                           bpp);

      gegl_buffer_set (dest_buffer, GEGL_RECTANGLE (x, row, w, 1), 0,
                       format, dest_row,
                       GEGL_AUTO_ROWSTRIDE);

      if ((row % 10) == 0)
        gimp_progress_update ((double) row / (double) h);
    }

  g_object_unref (src_buffer);
  g_object_unref (dest_buffer);

  g_free (src_row);
  g_free (dest_row);

  gimp_progress_update (1.0);

  /*  update the processed region  */
  gimp_drawable_merge_shadow (drawable_id, TRUE);
  gimp_drawable_update (drawable_id, x, y, w, h);
}

/**********************************************************************
 FUNCTION: explorer_render_row
 *********************************************************************/

void
explorer_render_row (const guchar *src_row,
                     guchar       *dest_row,
                     gint          row,
                     gint          row_width,
                     gint          bpp)
{
  gint    col;
  gdouble a;
  gdouble b;
  gdouble x;
  gdouble y;
  gdouble oldx;
  gdouble oldy;
  gdouble tempsqrx;
  gdouble tempsqry;
  gdouble tmpx = 0;
  gdouble tmpy = 0;
  gdouble foldxinitx;
  gdouble foldxinity;
  gdouble foldyinitx;
  gdouble foldyinity;
  gdouble xx = 0;
  gdouble adjust;
  gdouble cx;
  gdouble cy;
  gint    counter;
  gint    color;
  gint    iteration;
  gint    useloglog;
  gdouble log2;

  cx = wvals.cx;
  cy = wvals.cy;
  useloglog = wvals.useloglog;
  iteration = wvals.iter;
  log2 = log (2.0);

  for (col = 0; col < row_width; col++)
    {
      a = xmin + (double) col * xdiff;
      b = ymin + (double) row * ydiff;
      if (wvals.fractaltype != 0)
        {
          tmpx = x = a;
          tmpy = y = b;
        }
      else
        {
          x = 0;
          y = 0;
        }

      for (counter = 0; counter < iteration; counter++)
        {
          oldx=x;
          oldy=y;

          switch (wvals.fractaltype)
            {
            case TYPE_MANDELBROT:
              xx = x * x - y * y + a;
              y = 2.0 * x * y + b;
              break;

            case TYPE_JULIA:
              xx = x * x - y * y + cx;
              y = 2.0 * x * y + cy;
              break;

            case TYPE_BARNSLEY_1:
              foldxinitx = oldx * cx;
              foldyinity = oldy * cy;
              foldxinity = oldx * cy;
              foldyinitx = oldy * cx;
              /* orbit calculation */
              if (oldx >= 0)
                {
                  xx = (foldxinitx - cx - foldyinity);
                  y  = (foldyinitx - cy + foldxinity);
                }
              else
                {
                  xx = (foldxinitx + cx - foldyinity);
                  y  = (foldyinitx + cy + foldxinity);
                }
              break;

            case TYPE_BARNSLEY_2:
              foldxinitx = oldx * cx;
              foldyinity = oldy * cy;
              foldxinity = oldx * cy;
              foldyinitx = oldy * cx;
              /* orbit calculation */
              if (foldxinity + foldyinitx >= 0)
                {
                  xx = foldxinitx - cx - foldyinity;
                  y  = foldyinitx - cy + foldxinity;
                }
              else
                {
                  xx = foldxinitx + cx - foldyinity;
                  y  = foldyinitx + cy + foldxinity;
                }
              break;

            case TYPE_BARNSLEY_3:
              foldxinitx  = oldx * oldx;
              foldyinity  = oldy * oldy;
              foldxinity  = oldx * oldy;
              /* orbit calculation */
              if (oldx > 0)
                {
                  xx = foldxinitx - foldyinity - 1.0;
                  y  = foldxinity * 2;
                }
              else
                {
                  xx = foldxinitx - foldyinity -1.0 + cx * oldx;
                  y  = foldxinity * 2;
                  y += cy * oldx;
                }
              break;

            case TYPE_SPIDER:
              /* { c=z=pixel: z=z*z+c; c=c/2+z, |z|<=4 } */
              xx = x*x - y*y + tmpx + cx;
              y = 2 * oldx * oldy + tmpy +cy;
              tmpx = tmpx/2 + xx;
              tmpy = tmpy/2 + y;
              break;

            case TYPE_MAN_O_WAR:
              xx = x*x - y*y + tmpx + cx;
              y = 2.0 * x * y + tmpy + cy;
              tmpx = oldx;
              tmpy = oldy;
              break;

            case TYPE_LAMBDA:
              tempsqrx = x * x;
              tempsqry = y * y;
              tempsqrx = oldx - tempsqrx + tempsqry;
              tempsqry = -(oldy * oldx);
              tempsqry += tempsqry + oldy;
              xx = cx * tempsqrx - cy * tempsqry;
              y = cx * tempsqry + cy * tempsqrx;
              break;

            case TYPE_SIERPINSKI:
              xx = oldx + oldx;
              y = oldy + oldy;
              if (oldy > .5)
                y = y - 1;
              else if (oldx > .5)
                xx = xx - 1;
              break;

            default:
              break;
            }

          x = xx;

          if (((x * x) + (y * y)) >= 4.0)
            break;
        }

      if (useloglog)
        {
          gdouble modulus_square = (x * x) + (y * y);

          if (modulus_square > (G_E * G_E))
              adjust = log (log (modulus_square) / 2.0) / log2;
          else
              adjust = 0.0;
        }
      else
        {
          adjust = 0.0;
        }

      color = (int) (((counter - adjust) * (wvals.ncolors - 1)) / iteration);
      if (bpp >= 3)
        {
          dest_row[col * bpp + 0] = colormap[color].r;
          dest_row[col * bpp + 1] = colormap[color].g;
          dest_row[col * bpp + 2] = colormap[color].b;
        }
      else
          dest_row[col * bpp + 0] = valuemap[color];

      if (! ( bpp % 2))
        dest_row [col * bpp + bpp - 1] = 255;

    }
}

static void
delete_dialog_callback (GtkWidget *widget,
                        gboolean   delete,
                        gpointer   data)
{
  GtkWidget          *view = (GtkWidget *) data;
  GtkTreeSelection   *selection;
  GtkTreeModel       *model;
  GtkTreeIter         iter;
  gboolean            valid;
  fractalexplorerOBJ *sel_obj;

  if (delete)
    {
      /* Must update which object we are editing */
      /* Get the list and which item is selected */
      /* Only allow single selections */
      selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
      gtk_tree_selection_get_selected (selection, &model, &iter);

      gtk_tree_model_get (model, &iter, 1, &sel_obj, -1);

      /* Delete the current item + associated file */
      valid = gtk_list_store_remove (GTK_LIST_STORE(model), &iter);

      /* Try to select first item if last one was deleted */
      if (!valid)
        valid = gtk_tree_model_get_iter_first (model, &iter);

      /* Shadow copy for ordering info */
      fractalexplorer_list = g_list_remove (fractalexplorer_list, sel_obj);
      /*
        if(sel_obj == current_obj)
        {
        clear_undo();
        }
      */
      /* Free current obj */
      fractalexplorer_free_everything (sel_obj);

      /* Check whether there are items left */
      if (valid)
        {
          gtk_tree_selection_select_iter (selection, &iter);

          gtk_tree_model_get (model, &iter, 1, &current_obj, -1);
        }
    }

  delete_dialog = NULL;
}

static gboolean
delete_fractal_callback (GtkWidget *widget,
                         gpointer   data)
{
  gchar              *str;
  GtkWidget          *view = (GtkWidget *) data;
  GtkTreeSelection   *selection;
  GtkTreeModel       *model;
  GtkTreeIter         iter;
  fractalexplorerOBJ *sel_obj;

  if (delete_dialog)
    return FALSE;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gtk_tree_model_get (model, &iter, 1, &sel_obj, -1);

      str = g_strdup_printf (_("Are you sure you want to delete "
                               "\"%s\" from the list and from disk?"),
                             sel_obj->draw_name);

      delete_dialog = gimp_query_boolean_box (_("Delete Fractal"),
                                              gtk_widget_get_toplevel (view),
                                              gimp_standard_help_func, NULL,
                                              GIMP_ICON_DIALOG_QUESTION,
                                              str,
                                              _("_Delete"), _("_Cancel"),
                                              G_OBJECT (widget), "destroy",
                                              delete_dialog_callback,
                                              data);
      g_free (str);

      gtk_widget_show (delete_dialog);
    }

  return FALSE;
}

static gint
fractalexplorer_list_pos (fractalexplorerOBJ *fractalexplorer)
{
  fractalexplorerOBJ *g;
  gint                n;
  GList              *tmp;

  n = 0;

  for (tmp = fractalexplorer_list; tmp; tmp = g_list_next (tmp))
    {
      g = tmp->data;

      if (strcmp (fractalexplorer->draw_name, g->draw_name) <= 0)
        break;

      n++;
    }
  return n;
}

static gint
fractalexplorer_list_insert (fractalexplorerOBJ *fractalexplorer)
{
  gint n = fractalexplorer_list_pos (fractalexplorer);

  /*
   *    Insert fractalexplorers in alphabetical order
   */

  fractalexplorer_list = g_list_insert (fractalexplorer_list,
                                        fractalexplorer, n);

  return n;
}

static fractalexplorerOBJ *
fractalexplorer_new (void)
{
  return g_new0 (fractalexplorerOBJ, 1);
}

static void
fill_list_store (GtkListStore *list_store)
{
  GList         *tmp;
  GtkTreeIter    iter;


  for (tmp = fractalexplorer_list; tmp; tmp = tmp->next)
    {
      fractalexplorerOBJ *g;
      g = tmp->data;

      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (list_store, &iter, 0, g->draw_name, 1, g, -1);
    }
}

static void
activate_fractal (fractalexplorerOBJ *sel_obj)
{
  current_obj = sel_obj;
  wvals = current_obj->opts;
  dialog_change_scale ();
  set_cmap_preview ();
  dialog_update_preview ();
}

static void
activate_fractal_callback (GtkTreeView       *view,
                           GtkTreePath       *path,
                           GtkTreeViewColumn *col,
                           gpointer           data)
{
  GtkTreeModel       *model;
  GtkTreeIter         iter;
  fractalexplorerOBJ *sel_obj;

  model = gtk_tree_view_get_model (view);

  if (gtk_tree_model_get_iter (model, &iter, path))
    {
      gtk_tree_model_get (model, &iter, 1, &sel_obj, -1);
      activate_fractal (sel_obj);
    }

}

static gboolean
apply_fractal_callback (GtkWidget *widget,
                        gpointer   data)
{
  GtkWidget          *view = (GtkWidget *) data;
  GtkTreeSelection   *selection;
  GtkTreeModel       *model;
  GtkTreeIter         iter;
  fractalexplorerOBJ *sel_obj;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gtk_tree_model_get (model, &iter, 1, &sel_obj, -1);
      activate_fractal (sel_obj);
    }

  return FALSE;
}

static void
fractalexplorer_free (fractalexplorerOBJ *fractalexplorer)
{
  g_assert (fractalexplorer != NULL);

  g_free (fractalexplorer->name);
  g_free (fractalexplorer->filename);
  g_free (fractalexplorer->draw_name);
  g_free (fractalexplorer);
}

static void
fractalexplorer_free_everything (fractalexplorerOBJ *fractalexplorer)
{
  g_assert (fractalexplorer != NULL);

  if (fractalexplorer->filename)
    g_remove (fractalexplorer->filename);

  fractalexplorer_free (fractalexplorer);
}

static void
fractalexplorer_list_free_all (void)
{
  g_list_free_full (fractalexplorer_list, (GDestroyNotify) fractalexplorer_free);
  fractalexplorer_list = NULL;
}

static fractalexplorerOBJ *
fractalexplorer_load (const gchar *filename,
                      const gchar *name)
{
  fractalexplorerOBJ * fractalexplorer;
  FILE * fp;
  gchar load_buf[MAX_LOAD_LINE];

  g_assert (filename != NULL);

  fp = g_fopen (filename, "rt");
  if (!fp)
    {
      g_message (_("Could not open '%s' for reading: %s"),
                 gimp_filename_to_utf8 (filename), g_strerror (errno));
      return NULL;
    }

  fractalexplorer = fractalexplorer_new ();

  fractalexplorer->name = g_strdup (name);
  fractalexplorer->draw_name = g_strdup (name);
  fractalexplorer->filename = g_strdup (filename);

  /* HEADER
   * draw_name
   * version
   * obj_list
   */

  get_line (load_buf, MAX_LOAD_LINE, fp, 1);

  if (strncmp (fractalexplorer_HEADER, load_buf, strlen (load_buf)))
    {
      g_message (_("File '%s' is not a FractalExplorer file"),
                 gimp_filename_to_utf8 (filename));
      fclose (fp);
      fractalexplorer_free (fractalexplorer);

      return NULL;
    }

  if (load_options (fractalexplorer, fp))
    {
      g_message (_("File '%s' is corrupt.\nLine %d Option section incorrect"),
                 gimp_filename_to_utf8 (filename), line_no);
      fclose (fp);
      fractalexplorer_free (fractalexplorer);

      return NULL;
    }

  fclose (fp);

  fractalexplorer->obj_status = fractalexplorer_OK;

  return fractalexplorer;
}

static void
fractalexplorer_list_load_all (const gchar *explorer_path)
{
  GList *path;
  GList *list;

  /*  Make sure to clear any existing fractalexplorers  */
  current_obj = NULL;
  fractalexplorer_list_free_all ();

  path = gimp_config_path_expand_to_files (explorer_path, NULL);

  for (list = path; list; list = g_list_next (list))
    {
      GFileEnumerator *enumerator;

      enumerator = g_file_enumerate_children (list->data,
                                              G_FILE_ATTRIBUTE_STANDARD_NAME ","
                                              G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN ","
                                              G_FILE_ATTRIBUTE_STANDARD_TYPE,
                                              G_FILE_QUERY_INFO_NONE,
                                              NULL, NULL);

      if (enumerator)
        {
          GFileInfo *info;

          while ((info = g_file_enumerator_next_file (enumerator, NULL, NULL)))
            {
              GFileType file_type = g_file_info_get_file_type (info);

              if (file_type == G_FILE_TYPE_REGULAR &&
                  ! g_file_info_get_is_hidden (info))
                {
                  fractalexplorerOBJ *fractalexplorer;
                  GFile              *child;
                  gchar              *filename;
                  gchar              *basename;

                  child = g_file_enumerator_get_child (enumerator, info);

                  filename = g_file_get_path (child);
                  basename = g_file_get_basename (child);

                  fractalexplorer = fractalexplorer_load (filename,
                                                          basename);

                  g_free (filename);
                  g_free (basename);

                  if (fractalexplorer)
                    fractalexplorer_list_insert (fractalexplorer);

                  g_object_unref (child);
                }

              g_object_unref (info);
            }

          g_object_unref (enumerator);
        }
    }

  g_list_free_full (path, (GDestroyNotify) g_object_unref);

  if (!fractalexplorer_list)
    {
      fractalexplorerOBJ *fractalexplorer;

      /* lets have at least one! */
      fractalexplorer = fractalexplorer_new ();
      fractalexplorer->draw_name = g_strdup (_("My first fractal"));
      fractalexplorer_list_insert (fractalexplorer);
    }
  current_obj = fractalexplorer_list->data;  /* set to first entry */
}


GtkWidget *
add_objects_list (void)
{
  GtkWidget         *table;
  GtkWidget         *scrolled_win;
  GtkTreeViewColumn *col;
  GtkCellRenderer   *renderer;
  GtkWidget         *view;
  GtkTreeSelection  *selection;
  GtkListStore      *list_store;
  GtkWidget         *button;

  table = gtk_table_new (3, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 12);
  gtk_widget_show (table);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win),
                                       GTK_SHADOW_IN);
  gtk_table_attach (GTK_TABLE (table), scrolled_win, 0, 3, 0, 1,
                    GTK_FILL|GTK_EXPAND , GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show (scrolled_win);

  view = gtk_tree_view_new ();
  col = gtk_tree_view_column_new ();
  gtk_tree_view_append_column (GTK_TREE_VIEW (view), col);

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (col, renderer, TRUE);
  gtk_tree_view_column_add_attribute (col, renderer, "text", 0);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), FALSE);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);
  g_signal_connect (view, "row-activated",
                    G_CALLBACK (activate_fractal_callback),
                    NULL);
  gtk_container_add (GTK_CONTAINER (scrolled_win), view);
  gtk_widget_show (view);

  fractalexplorer_list_load_all (fractalexplorer_path);
  list_store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_POINTER);
  fill_list_store (list_store);
  gtk_tree_view_set_model (GTK_TREE_VIEW (view), GTK_TREE_MODEL (list_store));
  g_object_unref (list_store); /* destroy model automatically with view */

  /* Put buttons in */
  button = gtk_button_new_with_mnemonic (_("_Refresh"));
  gtk_table_attach (GTK_TABLE (table), button, 0, 1, 1, 2,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (button);

  gimp_help_set_help_data (button,
                           _("Select folder and rescan collection"), NULL);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (fractalexplorer_rescan_list),
                    view);

  button = gtk_button_new_with_mnemonic (_("_Apply"));
  gtk_table_attach (GTK_TABLE (table), button, 1, 2, 1, 2,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (button);

  gimp_help_set_help_data (button,
                           _("Apply currently selected fractal"), NULL);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (apply_fractal_callback),
                    view);

  button = gtk_button_new_with_mnemonic (_("_Delete"));
  gtk_table_attach (GTK_TABLE (table), button, 2, 3, 1, 2,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (button);

  gimp_help_set_help_data (button,
                           _("Delete currently selected fractal"), NULL);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (delete_fractal_callback),
                    view);

  return table;
}

static void
fractalexplorer_rescan_list (GtkWidget *widget,
                             gpointer   data)
{
  static GtkWidget *dlg  = NULL;
  GtkWidget        *view = data;
  GtkWidget        *patheditor;

  if (dlg)
    {
      gtk_window_present (GTK_WINDOW (dlg));
      return;
    }

  dlg = gimp_dialog_new (_("Rescan for Fractals"), PLUG_IN_ROLE,
                         gtk_widget_get_toplevel (view),
                         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                         gimp_standard_help_func, PLUG_IN_PROC,

                         _("_Cancel"), GTK_RESPONSE_CANCEL,
                         _("_OK"),     GTK_RESPONSE_OK,

                         NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dlg),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect (dlg, "destroy",
                    G_CALLBACK (gtk_widget_destroyed),
                    &dlg);

  patheditor = gimp_path_editor_new (_("Add FractalExplorer Path"),
                                     fractalexplorer_path);
  gtk_container_set_border_width (GTK_CONTAINER (patheditor), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dlg))),
                      patheditor, TRUE, TRUE, 0);
  gtk_widget_show (patheditor);

  if (gimp_dialog_run (GIMP_DIALOG (dlg)) == GTK_RESPONSE_OK)
    {
      g_free (fractalexplorer_path);
      fractalexplorer_path =
        gimp_path_editor_get_path (GIMP_PATH_EDITOR (patheditor));

      if (fractalexplorer_path)
        {
          GtkTreeModel     *model;
          GtkTreeSelection *selection;
          GtkTreePath      *path;
          GtkTreeIter       iter;

          fractalexplorer_list_load_all (fractalexplorer_path);

          model = gtk_tree_view_get_model (GTK_TREE_VIEW (view));
          gtk_list_store_clear (GTK_LIST_STORE (model));
          fill_list_store (GTK_LIST_STORE (model));

          /* select active fractal, otherwise first fractal */
          selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
          if (gtk_tree_model_get_iter_first (model, &iter))
            {
              gtk_tree_selection_select_iter (selection, &iter);
              path = gtk_tree_model_get_path (model, &iter);
              current_obj = fractalexplorer_list->data;
              gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (view), path, NULL,
                                            FALSE, 0.0, 0.0);
              gtk_tree_path_free (path);
            }
        }
    }

  gtk_widget_destroy (dlg);
}
