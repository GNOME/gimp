/**************************************************
 * file: megawidget/megawidget.h
 *
 * Copyright (c) 1997 Eric L. Hernes (erich@rrnet.com)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software withough specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id$
 */

#ifndef __MEGAWIDGET_H__
#define __MEGAWIDGET_H__

struct mwPreview
{
  gint     width;
  gint     height;
  gint     bpp;
  gdouble  scale;
  guchar  *bits;
};

#ifndef PREVIEW_SIZE
#define PREVIEW_SIZE 100
#endif

extern gint do_preview;

typedef void mw_preview_t (GtkWidget *pvw);

GtkWidget        * mw_preview_new          (GtkWidget        *parent,
					    struct mwPreview *mwp,
					    mw_preview_t     *fcn);

struct mwPreview * mw_preview_build        (GDrawable        *drw);
struct mwPreview * mw_preview_build_virgin (GDrawable        *drw);

#endif /* __MEGAWIDGET_H__ */
