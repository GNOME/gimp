#ifndef __LAYER_F_H__
#define __LAYER_F_H__

typedef enum  /*< chop=ADD_ >*/
{
  ADD_WHITE_MASK,
  ADD_BLACK_MASK,
  ADD_ALPHA_MASK
} AddMaskType;

typedef struct _GimpLayer      GimpLayer;
typedef struct _GimpLayerClass GimpLayerClass;
typedef struct _GimpLayerMask      GimpLayerMask;
typedef struct _GimpLayerMaskClass GimpLayerMaskClass;

typedef GimpLayer Layer;		/* convenience */
typedef GimpLayerMask LayerMask;	/* convenience */

typedef struct _layer_undo LayerUndo;

typedef struct _layer_mask_undo LayerMaskUndo;

typedef struct _fs_to_layer_undo FStoLayerUndo;

#endif
