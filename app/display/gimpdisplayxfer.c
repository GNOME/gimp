/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "display-types.h"

#include "gimpdisplayxfer.h"


#define NUM_PAGES 2

typedef struct _RTree     RTree;
typedef struct _RTreeNode RTreeNode;

struct _RTreeNode
{
  RTreeNode *children[2];
  RTreeNode *next;
  gint       x, y, w, h;
};

struct _RTree
{
  RTreeNode  root;
  RTreeNode *available;
};

struct _GimpDisplayXfer
{
  /* track subregions of render_surface for efficient uploads */
  RTree            rtree;
  cairo_surface_t *render_surface[NUM_PAGES];
  gint             page;
};


gint GIMP_DISPLAY_RENDER_BUF_WIDTH  = 256;
gint GIMP_DISPLAY_RENDER_BUF_HEIGHT = 256;


static RTreeNode *
rtree_node_create (RTree      *rtree,
                   RTreeNode **prev,
                   gint        x,
                   gint        y,
                   gint        w,
                   gint        h)
{
  RTreeNode *node;

  gimp_assert (x >= 0 && x+w <= rtree->root.w);
  gimp_assert (y >= 0 && y+h <= rtree->root.h);

  if (w <= 0 || h <= 0)
    return NULL;

  node = g_slice_alloc (sizeof (*node));

  node->children[0] = NULL;
  node->children[1] = NULL;
  node->x = x;
  node->y = y;
  node->w = w;
  node->h = h;

  node->next = *prev;
  *prev = node;

  return node;
}

static void
rtree_node_destroy (RTree     *rtree,
                    RTreeNode *node)
{
  gint i;

  for (i = 0; i < 2; i++)
    {
      if (node->children[i])
        rtree_node_destroy (rtree, node->children[i]);
    }

  g_slice_free (RTreeNode, node);
}

static RTreeNode *
rtree_node_insert (RTree      *rtree,
                   RTreeNode **prev,
                   RTreeNode  *node,
                   gint        w,
                   gint        h)
{
  *prev = node->next;

  if (((node->w - w) | (node->h - h)) > 1)
    {
      gint ww = node->w - w;
      gint hh = node->h - h;

      if (ww >= hh)
        {
          node->children[0] = rtree_node_create (rtree, prev,
                                                 node->x + w, node->y,
                                                 ww, node->h);
          node->children[1] = rtree_node_create (rtree, prev,
                                                 node->x, node->y + h,
                                                 w, hh);
        }
      else
        {
          node->children[0] = rtree_node_create (rtree, prev,
                                                 node->x, node->y + h,
                                                 node->w, hh);
          node->children[1] = rtree_node_create (rtree, prev,
                                                 node->x + w, node->y,
                                                 ww, h);
        }
    }

  return node;
}

static RTreeNode *
rtree_insert (RTree *rtree,
              gint   w,
              gint   h)
{
  RTreeNode *node, **prev;

  for (prev = &rtree->available; (node = *prev); prev = &node->next)
    if (node->w >= w && node->h >= h)
      return rtree_node_insert (rtree, prev, node, w, h);

  return NULL;
}

static void
rtree_init (RTree *rtree,
            gint   w,
            gint   h)
{
  rtree->root.x = 0;
  rtree->root.y = 0;
  rtree->root.w = w;
  rtree->root.h = h;
  rtree->root.children[0] = NULL;
  rtree->root.children[1] = NULL;
  rtree->root.next = NULL;
  rtree->available = &rtree->root;
}

static void
rtree_reset (RTree *rtree)
{
  gint i;

  for (i = 0; i < 2; i++)
    {
      if (rtree->root.children[i] == NULL)
        continue;

      rtree_node_destroy (rtree, rtree->root.children[i]);
      rtree->root.children[i] = NULL;
    }

  rtree->root.next = NULL;
  rtree->available = &rtree->root;
}

static void
xfer_destroy (void *data)
{
  GimpDisplayXfer *xfer = data;
  gint             i;

  for (i = 0; i < NUM_PAGES; i++)
    cairo_surface_destroy (xfer->render_surface[i]);

  rtree_reset (&xfer->rtree);
  g_free (xfer);
}

GimpDisplayXfer *
gimp_display_xfer_realize (GtkWidget *widget)
{
  GdkScreen       *screen;
  GimpDisplayXfer *xfer;
  const gchar     *env;

  env = g_getenv ("GIMP_DISPLAY_RENDER_BUF_SIZE");
  if (env)
    {
      gint width  = atoi (env);
      gint height = width;

      env = strchr (env, 'x');
      if (env)
        height = atoi (env + 1);

      if (width  > 0 && width  <= 8192 &&
          height > 0 && height <= 8192)
        {
          GIMP_DISPLAY_RENDER_BUF_WIDTH  = width;
          GIMP_DISPLAY_RENDER_BUF_HEIGHT = height;
        }
    }

  screen = gtk_widget_get_screen (widget);
  xfer = g_object_get_data (G_OBJECT (screen), "gimp-display-xfer");

  if (xfer == NULL)
    {
      cairo_t *cr;
      gint     w = GIMP_DISPLAY_RENDER_BUF_WIDTH;
      gint     h = GIMP_DISPLAY_RENDER_BUF_HEIGHT;
      int      n;

      xfer = g_new (GimpDisplayXfer, 1);
      rtree_init (&xfer->rtree, w, h);

      cr = gdk_cairo_create (gtk_widget_get_window (widget));
      for (n = 0; n < NUM_PAGES; n++)
        {
          xfer->render_surface[n] =
            cairo_surface_create_similar_image (cairo_get_target (cr),
                                                CAIRO_FORMAT_ARGB32, w, h);
          cairo_surface_mark_dirty (xfer->render_surface[n]);
        }
      cairo_destroy (cr);
      xfer->page = 0;

      g_object_set_data_full (G_OBJECT (screen),
                              "gimp-display-xfer",
                              xfer, xfer_destroy);
    }

  return xfer;
}

cairo_surface_t *
gimp_display_xfer_get_surface (GimpDisplayXfer *xfer,
                               gint             w,
                               gint             h,
                               gint            *src_x,
                               gint            *src_y)
{
  RTreeNode *node;

  gimp_assert (w <= GIMP_DISPLAY_RENDER_BUF_WIDTH &&
               h <= GIMP_DISPLAY_RENDER_BUF_HEIGHT);

  node = rtree_insert (&xfer->rtree, w, h);
  if (node == NULL)
    {
      xfer->page = (xfer->page + 1) % NUM_PAGES;
      cairo_surface_flush (xfer->render_surface[xfer->page]);
      rtree_reset (&xfer->rtree);
      cairo_surface_mark_dirty (xfer->render_surface[xfer->page]); /* XXX */
      node = rtree_insert (&xfer->rtree, w, h);
      gimp_assert (node != NULL);
    }

  *src_x = node->x;
  *src_y = node->y;

  return xfer->render_surface[xfer->page];
}
