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
#ifndef __GIMPIMAGEP_H__
#define __GIMPIMAGEP_H__

#include "gimpobjectP.h"
#include "gimpimage.h"

#include "tile_manager.h"
#include "temp_buf.h"
#include "channel.h"
#include "layer.h"
#include "parasitelistF.h"
#include "pathsP.h"
#include "undo_types.h"


#define MAX_CHANNELS     4

struct _GimpImage
{
  GimpObject gobject;

  gchar         *filename;	      /*  original filename            */
  gboolean       has_filename;        /*  has a valid filename         */
  PlugInProcDef *save_proc;           /*  last PDB save proc used      */

  gint               width, height;   /*  width and height attributes  */
  gdouble            xresolution;     /*  image x-res, in dpi          */
  gdouble            yresolution;     /*  image y-res, in dpi          */
  GimpUnit           unit;            /*  image unit                   */
  GimpImageBaseType  base_type;       /*  base gimp_image type         */

  guchar *cmap;                       /*  colormap--for indexed        */
  int     num_cols;                   /*  number of cols--for indexed  */

  gint      dirty;                    /*  dirty flag -- # of ops       */
  gboolean  undo_on;                  /*  Is undo enabled?             */

  gint instance_count;                /*  number of instances          */
  gint disp_count;                    /*  number of displays           */

  Tattoo tattoo_state;                /*  the next unique tattoo to use*/

  TileManager *shadow;                /*  shadow buffer tiles          */

                                      /*  Projection attributes  */
  gint           construct_flag;      /*  flag for construction        */
  GimpImageType  proj_type;           /*  type of the projection image */
  gint           proj_bytes;          /*  bpp in projection image      */
  gint           proj_level;          /*  projection level             */
  TileManager   *projection;          /*  The projection--layers &     */
                                      /*  channels                     */

  GList *guides;                      /*  guides                       */

                                      /*  Layer/Channel attributes  */
  GSList *layers;                     /*  the list of layers           */
  GSList *channels;                   /*  the list of masks            */
  GSList *layer_stack;                /*  the layers in MRU order      */

  Layer   *active_layer;              /*  ID of active layer           */
  Channel *active_channel;	      /*  ID of active channel         */
  Layer   *floating_sel;              /*  ID of fs layer               */
  Channel *selection_mask;            /*  selection mask channel       */

  ParasiteList *parasites;            /*  Plug-in parasite data        */

  PathList *paths;                    /*  Paths data for this image    */

  gint visible [MAX_CHANNELS];        /*  visible channels             */
  gint active  [MAX_CHANNELS];        /*  active channels              */

  gboolean  by_color_select;           /*  TRUE if there's an active    */
                                      /*  "by color" selection dialog  */

  gboolean  qmask_state;              /*  TRUE if qmask is on          */
  gdouble   qmask_opacity;            /*  opacity of the qmask channel */
  guchar    qmask_color[3];           /*  rgb triplet of the color     */

                                      /*  Undo apparatus  */
  GSList *undo_stack;                 /*  stack for undo operations    */
  GSList *redo_stack;                 /*  stack for redo operations    */
  gint    undo_bytes;                 /*  bytes in undo stack          */
  gint    undo_levels;                /*  levels in undo stack         */
  gint    group_count;		      /*  nested undo groups           */
  UndoType pushing_undo_group;        /*  undo group status flag       */
  GtkWidget *undo_history;	      /*  history viewer, or NULL      */

                                      /*  Composite preview  */
  TempBuf *comp_preview;              /*  the composite preview        */
  gint comp_preview_valid[3];         /*  preview valid-1/channel      */
};

struct _GimpImageClass
{
  GimpObjectClass parent_class;
  void (*dirty)   (GtkObject*);
  void (*repaint) (GtkObject*);
  void (*rename)  (GtkObject*);
};
typedef struct _GimpImageClass GimpImageClass;

#define GIMP_IMAGE_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, GIMP_TYPE_IMAGE, GimpImageClass)

#endif /* __GIMPIMAGEP_H__ */
