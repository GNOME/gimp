#ifndef __XCF_H__
#define __XCF_H__


#include <stdio.h>


typedef struct _XcfInfo  XcfInfo;

struct _XcfInfo
{
  FILE *fp;
  guint cp;
  char *filename;
  int active_layer;
  int active_channel;
  int floating_sel_drawable;
  guint floating_sel_offset;
  int swap_num;
  int *ref_count;
  int compression;
  int file_version;
};


void xcf_init (void);


#endif /* __XCF_H__ */
