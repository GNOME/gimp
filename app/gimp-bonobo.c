/* The GIMP -- an image manipulation program
 * Copyright (C) 1995, 1996, 1997 Spencer Kimball and Peter Mattis
 * Copyright (C) 1999 Elliot Lee
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
 *
 * Bonobo support for the GIMP.
 *
 * Author: Elliot Lee <sopwith@redhat.com>
 */

#include "config.h"
#include "gimp-bonobo.h"
#include "gimpimage.h"
#include "gdisplay.h"
#include "fileops.h"

#include <stdlib.h>
#include <time.h>

typedef struct {
  GnomeEmbeddable *bonobo_object;

  GList *views;

  GimpImage *gimp_image;
  char *tmpfile;
} bonobo_object_data_t;

typedef struct {
  GDisplay *gdisplay;
  bonobo_object_data_t *parent;
} bonobo_view_t;

#define IO_CHUNK_SIZE 65536

static GnomeObject *
gimp_object_factory (GnomeEmbeddableFactory *this, void *data);

static GnomeView *
gimp_view_factory (GnomeEmbeddable *bonobo_object,
		   const GNOME_ViewFrame view_frame,
		   void *data);
static void
gimp_view_destroy (GnomeView *view, bonobo_view_t *view_data);

static int
gimp_stream_load_image (GnomePersistStream *ps, GNOME_Stream stream, void *data);

static int
gimp_stream_save_image (GnomePersistStream *ps, GNOME_Stream stream, void *data);

void
gimp_bonobo_init(void)
{
  gnome_embeddable_factory_new ("bonobo-object-factory:gimp-image",
				gimp_object_factory, NULL);
  srand(time(NULL));
}

static GnomeObject *
gimp_object_factory (GnomeEmbeddableFactory *this, void *data)
{
  GnomeEmbeddable *bonobo_object;
  GnomePersistStream *stream;
  bonobo_object_data_t *bonobo_object_data;

  bonobo_object_data = g_new0 (bonobo_object_data_t, 1);

  bonobo_object = gnome_embeddable_new (gimp_view_factory, bonobo_object_data);
  if (bonobo_object == NULL)
    goto out;

  stream = gnome_persist_stream_new (gimp_stream_load_image,
				     gimp_stream_save_image,
				     bonobo_object_data);
  if (stream == NULL)
    goto out;

  bonobo_object_data->bonobo_object = bonobo_object;

  gnome_object_add_interface (GNOME_OBJECT (bonobo_object),
			      GNOME_OBJECT (stream));
  return (GnomeObject *) bonobo_object;

 out:
  if (bonobo_object)
    gtk_object_unref (GTK_OBJECT (bonobo_object));
  if (bonobo_object_data)
    g_free (bonobo_object_data);
  return NULL;
}

static GnomeView *
gimp_view_factory (GnomeEmbeddable *bonobo_object,
		   const GNOME_ViewFrame view_frame,
		   void *data)
{
  bonobo_object_data_t *bonobo_object_data = data;
  bonobo_view_t *view_data;
  GnomeView *view;

  view_data = g_new0 (bonobo_view_t, 1);
  view_data->parent = bonobo_object_data;
  view_data->gdisplay = gdisplay_new (bonobo_object_data->gimp_image, 0x0101);

  view = gnome_view_new(view_data->gdisplay->shell);
  gtk_signal_connect (GTK_OBJECT (view), "destroy",
		      GTK_SIGNAL_FUNC(gimp_view_destroy), view_data);

  bonobo_object_data->views = g_list_prepend (bonobo_object_data->views,
					     view_data);

  return view;
}

static void
gimp_view_destroy (GnomeView *view, bonobo_view_t *view_data)
{
  view_data->parent->views = g_list_remove(view_data->parent->views, view_data);
  gdisplay_remove_and_delete(view_data->gdisplay);

  g_free(view_data);
}

/* Right now for file save/load we save the whole stream to a tmpfile, then edit that */
static int
gimp_stream_load_image (GnomePersistStream *ps, GNOME_Stream stream, void *data)
{
  GimpImage *new_image;
  char *new_tmpfile;
  GNOME_Stream_iobuf *iobuf = NULL;
  CORBA_long nread;
  CORBA_Environment ev;
  FILE *fh = NULL;
  GList *ltmp;

  bonobo_object_data_t *bonobo_object_data = data;

  /* Find a tmpfile to use */
  while((new_tmpfile = g_strdup_printf("%s/.gimp-temp-%x%x", g_get_home_dir(), rand(), rand()))
	&& !access(new_tmpfile, F_OK))
    g_free(new_tmpfile);

  CORBA_exception_init(&ev);
  fh = fopen(new_tmpfile, "w");
  if(!fh) goto out;

  nread = IO_CHUNK_SIZE;
  while(nread == IO_CHUNK_SIZE) {
    nread = GNOME_Stream_read(stream, IO_CHUNK_SIZE, &iobuf, &ev);
    if(ev._major != CORBA_NO_EXCEPTION) {
      iobuf = NULL;
      goto out;
    }

    fwrite(iobuf->_buffer, iobuf->_length, 1, fh);
    CORBA_free(iobuf);
  }
  fclose(fh); fh = NULL; iobuf = NULL;

  new_image = file_open_new_image(new_tmpfile, "Embedded Image");
  if(!new_image)
    goto out;

  for(ltmp = bonobo_object_data->views; ltmp; ltmp = g_list_next(ltmp)) {
    bonobo_view_t *view;

    view = (bonobo_view_t *)ltmp->data;

    gdisplay_reconnect(view->gdisplay, new_image);
  }

  if(bonobo_object_data->gimp_image)
    gtk_object_unref(GTK_OBJECT(bonobo_object_data->gimp_image));

  bonobo_object_data->gimp_image = new_image;
  gtk_object_ref(GTK_OBJECT(new_image));
  gtk_object_sink(GTK_OBJECT(new_image));
  g_free(bonobo_object_data->tmpfile);
  bonobo_object_data->tmpfile = new_tmpfile;

  CORBA_exception_free(&ev);

  return 0;

 out:
  if(fh)
    fclose(fh);

  CORBA_free(iobuf);

  if(new_tmpfile) {
    unlink(new_tmpfile);
    g_free(new_tmpfile);
  }

  CORBA_exception_free(&ev);

  return -1;
}

static int
gimp_stream_save_image (GnomePersistStream *ps, GNOME_Stream stream, void *data)
{
  GNOME_Stream_iobuf iobuf;
  char cbuf[IO_CHUNK_SIZE];
  CORBA_long nread;
  CORBA_Environment ev;
  FILE *fh = NULL;
  bonobo_object_data_t *bonobo_object_data = data;

  file_save(bonobo_object_data->gimp_image, bonobo_object_data->tmpfile, "Embedded Image", 0);

  CORBA_exception_init(&ev);
  fh = fopen(bonobo_object_data->tmpfile, "r");
  if(!fh) goto out;

  iobuf._buffer = cbuf;
  CORBA_sequence_set_release(&iobuf, CORBA_FALSE);
  while(1) {
    nread = fread(cbuf, 1, IO_CHUNK_SIZE, fh);
    if(nread <= 0)
      break;
    iobuf._length = nread;
    nread = GNOME_Stream_write(stream, &iobuf, &ev);
    if(ev._major != CORBA_NO_EXCEPTION)
      goto out;

    if(nread < IO_CHUNK_SIZE)
      break;
  }
  fclose(fh); fh = NULL;

  CORBA_exception_free(&ev);

  return 0;

 out:
  if(fh)
    fclose(fh);

  CORBA_exception_free(&ev);

  return -1;
}
