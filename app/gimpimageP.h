#ifndef __GIMPIMAGEP_H__
#define __GIMPIMAGEP_H__

#include "gimpobjectP.h"
#include "gimpimage.h"

#include "tile_manager.h"
#include "temp_buf.h"
#include "channel.h"
#include "layer.h"


#define MAX_CHANNELS     4

struct _GimpImage
{
  GimpObject gobject;
  char *filename;		      /*  original filename            */
  int has_filename;                   /*  has a valid filename         */

  int width, height;		      /*  width and height attributes  */
  int base_type;                      /*  base gimp_image type             */

  unsigned char * cmap;               /*  colormap--for indexed        */
  int num_cols;                       /*  number of cols--for indexed  */

  int dirty;                          /*  dirty flag -- # of ops       */
  int undo_on;                        /*  Is undo enabled?             */

  int instance_count;                 /*  number of instances          */
  int ref_count;                      /*  number of references         */

  TileManager *shadow;                /*  shadow buffer tiles          */

                                      /*  Projection attributes  */
  int flat;                           /*  Is the gimp_image flat?          */
  int construct_flag;                 /*  flag for construction        */
  int proj_type;                      /*  type of the projection image */
  int proj_bytes;                     /*  bpp in projection image      */
  int proj_level;                     /*  projection level             */
  TileManager *projection;            /*  The projection--layers &     */
                                      /*  channels                     */

  GList *guides;                      /*  guides                       */

                                      /*  Layer/Channel attributes  */
  GSList *layers;                     /*  the list of layers           */
  GSList *channels;                   /*  the list of masks            */
  GSList *layer_stack;                /*  the layers in MRU order      */

  Layer * active_layer;               /*  ID of active layer           */
  Channel * active_channel;	      /*  ID of active channel         */
  Layer * floating_sel;               /*  ID of fs layer               */
  Channel * selection_mask;           /*  selection mask channel       */

  int visible [MAX_CHANNELS];         /*  visible channels             */
  int active  [MAX_CHANNELS];         /*  active channels              */

  int by_color_select;                /*  TRUE if there's an active    */
                                      /*  "by color" selection dialog  */

                                      /*  Undo apparatus  */
  GSList *undo_stack;                 /*  stack for undo operations    */
  GSList *redo_stack;                 /*  stack for redo operations    */
  int undo_bytes;                     /*  bytes in undo stack          */
  int undo_levels;                    /*  levels in undo stack         */
  int pushing_undo_group;             /*  undo group status flag       */

                                      /*  Composite preview  */
  TempBuf *comp_preview;              /*  the composite preview        */
  int comp_preview_valid[3];          /*  preview valid-1/channel      */
};

struct _GimpImageClass
{
  GimpObjectClass parent_class;
  void (*dirty) (GtkObject*);
  void (*repaint) (GtkObject*);
  void (*rename) (GtkObject*);
};
typedef struct _GimpImageClass GimpImageClass;

#define GIMP_IMAGE_CLASS(klass) \
GTK_CHECK_CLASS_CAST (klass, GIMP_TYPE_IMAGE, GimpImageClass)

#endif
