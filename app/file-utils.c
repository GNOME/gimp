/* The GIMP -- an image manipulation program
 * Copyright (C) 1995, 1996, 1997 Spencer Kimball and Peter Mattis
 * Copyright (C) 1997 Josh MacDonald
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

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gtk/gtk.h>

#ifdef G_OS_WIN32
#include <direct.h>		/* For _mkdir() */
#define mkdir(path,mode) _mkdir(path)
#endif

#include "libgimpmath/gimpmath.h"

#include "apptypes.h"

#include "core/gimpimage.h"

#include "file-utils.h"
#include "plug_in.h"
#include "temp_buf.h"


static PlugInProcDef *
file_proc_find_by_name (GSList      *procs,
		        const gchar *filename,
		        gboolean     skip_magic)
{
  GSList *p;
  gchar  *ext = strrchr (filename, '.');

  if (ext)
    ext++;

  for (p = procs; p; p = g_slist_next (p))
    {
      PlugInProcDef *proc = p->data;
      GSList        *prefixes;

      if (skip_magic && proc->magics_list)
	continue;

      for (prefixes = proc->prefixes_list;
	   prefixes;
	   prefixes = g_slist_next (prefixes))
	{
	  if (strncmp (filename, prefixes->data, strlen (prefixes->data)) == 0)
	    return proc;
	}
     }

  for (p = procs; p; p = g_slist_next (p))
    {
      PlugInProcDef *proc = p->data;
      GSList        *extensions;

      for (extensions = proc->extensions_list;
	   ext && extensions;
	   extensions = g_slist_next (extensions))
	{
	  gchar *p1 = ext;
	  gchar *p2 = (gchar *) extensions->data;

          if (skip_magic && proc->magics_list)
	    continue;

	  while (*p1 && *p2)
	    {
	      if (tolower (*p1) != tolower (*p2))
		break;

	      p1++;
	      p2++;
	    }

	  if (!(*p1) && !(*p2))
	    return proc;
	}
    }

  return NULL;
}

PlugInProcDef *
file_proc_find (GSList      *procs,
		const gchar *filename)
{
  PlugInProcDef *file_proc;
  PlugInProcDef *size_matched_proc = NULL;
  GSList        *all_procs         = procs;
  FILE          *ifp               = NULL;
  gint           head_size         = -2;
  gint           size_match_count  = 0;
  gint           match_val;
  guchar         head[256];

  /* First, check magicless prefixes/suffixes */
  if ( (file_proc = file_proc_find_by_name (all_procs, filename, TRUE)) != NULL)
    return file_proc;

  /* Then look for magics */
  while (procs)
    {
      file_proc = procs->data;
      procs = procs->next;

      if (file_proc->magics_list)
        {
          if (head_size == -2)
            {
              head_size = 0;
              if ((ifp = fopen (filename, "rb")) != NULL)
                head_size = fread ((gchar *) head, 1, sizeof (head), ifp);
            }
          if (head_size >= 4)
            {
              match_val = file_check_magic_list (file_proc->magics_list,
                                                 head_size, head, ifp);
              if (match_val == 2)  /* size match ? */
                { /* Use it only if no other magic matches */
                  size_match_count++;
                  size_matched_proc = file_proc;
                }
              else if (match_val)
                {
                  fclose (ifp);
                  return (file_proc);
                }
            }
        }
    }
  if (ifp) fclose (ifp);
  if (size_match_count == 1)
    return (size_matched_proc);

  /* As a last ditch, try matching by name */
  return file_proc_find_by_name (all_procs, filename, FALSE);
}

static void
file_convert_string (gchar *instr,
		     gchar *outmem,
		     gint   maxmem,
		     gint  *nmem)
{
  /* Convert a string in C-notation to array of char */
  guchar *uin = (guchar *) instr;
  guchar *uout = (guchar *) outmem;
  guchar  tmp[5], *tmpptr;
  gint    k;

  while ((*uin != '\0') && ((((char *)uout) - outmem) < maxmem))
    {
      if (*uin != '\\')   /* Not an escaped character ? */
        {
          *(uout++) = *(uin++);
          continue;
        }
      if (*(++uin) == '\0')
        {
          *(uout++) = '\\';
          break;
        }
      switch (*uin)
        {
          case '0':  case '1':  case '2':  case '3': /* octal */
            for (tmpptr = tmp; (tmpptr-tmp) <= 3;)
              {
                *(tmpptr++) = *(uin++);
                if (   (*uin == '\0') || (!isdigit (*uin))
                    || (*uin == '8') || (*uin == '9'))
                  break;
              }
            *tmpptr = '\0';
            sscanf ((char *)tmp, "%o", &k);
            *(uout++) = k;
            break;

          case 'a': *(uout++) = '\a'; uin++; break;
          case 'b': *(uout++) = '\b'; uin++; break;
          case 't': *(uout++) = '\t'; uin++; break;
          case 'n': *(uout++) = '\n'; uin++; break;
          case 'v': *(uout++) = '\v'; uin++; break;
          case 'f': *(uout++) = '\f'; uin++; break;
          case 'r': *(uout++) = '\r'; uin++; break;

          default : *(uout++) = *(uin++); break;
        }
    }
  *nmem = ((gchar *) uout) - outmem;
}

static gint
file_check_single_magic (gchar  *offset,
                         gchar  *type,
                         gchar  *value,
                         gint    headsize,
                         guchar *file_head,
                         FILE   *ifp)

{
  /* Return values are 0: no match, 1: magic match, 2: size match */
  glong   offs;
  gulong  num_testval, num_operatorval;
  gulong  fileval;
  gint    numbytes, k, c = 0, found = 0;
  gchar  *num_operator_ptr, num_operator, num_test;
  guchar  mem_testval[256];

  /* Check offset */
  if (sscanf (offset, "%ld", &offs) != 1) return (0);
  if (offs < 0) return (0);

  /* Check type of test */
  num_operator_ptr = NULL;
  num_operator = '\0';
  num_test = '=';
  if (strncmp (type, "byte", 4) == 0)
    {
      numbytes = 1;
      num_operator_ptr = type+4;
    }
  else if (strncmp (type, "short", 5) == 0)
    {
      numbytes = 2;
      num_operator_ptr = type+5;
    }
  else if (strncmp (type, "long", 4) == 0)
    {
      numbytes = 4;
      num_operator_ptr = type+4;
    }
  else if (strncmp (type, "size", 4) == 0)
    {
      numbytes = 5;
    }
  else if (strcmp (type, "string") == 0)
    {
      numbytes = 0;
    }
  else return (0);

  /* Check numerical operator value if present */
  if (num_operator_ptr && (*num_operator_ptr == '&'))
    {
      if (isdigit (num_operator_ptr[1]))
        {
          if (num_operator_ptr[1] != '0')      /* decimal */
            sscanf (num_operator_ptr+1, "%ld", &num_operatorval);
          else if (num_operator_ptr[2] == 'x') /* hexadecimal */
            sscanf (num_operator_ptr+3, "%lx", &num_operatorval);
          else                                 /* octal */
            sscanf (num_operator_ptr+2, "%lo", &num_operatorval);
          num_operator = *num_operator_ptr;
        }
    }

  if (numbytes > 0)   /* Numerical test ? */
    {
      /* Check test value */
      if ((value[0] == '=') || (value[0] == '>') || (value[0] == '<'))
      {
        num_test = value[0];
        value++;
      }
      if (!isdigit (value[0])) return (0);

      /* 
       * to anybody reading this: is strtol's parsing behaviour (e.g. "0x" prefix)
       * broken on some systems or why do we do the base detection ourselves?
       * */
      if (value[0] != '0')      /* decimal */
        num_testval = strtol(value, NULL, 10);
      else if (value[1] == 'x') /* hexadecimal */
        num_testval = (unsigned long)strtoul(value+2, NULL, 16);
      else                      /* octal */
        num_testval = strtol(value+1, NULL, 8);

      fileval = 0;
      if (numbytes == 5)    /* Check for file size ? */
        {
	  struct stat buf;
	  
          if (fstat (fileno (ifp), &buf) < 0) return (0);
          fileval = buf.st_size;
        }
      else if (offs + numbytes <= headsize)  /* We have it in memory ? */
        {
          for (k = 0; k < numbytes; k++)
          fileval = (fileval << 8) | (long)file_head[offs+k];
        }
      else   /* Read it from file */
        {
          if (fseek (ifp, offs, SEEK_SET) < 0) return (0);
          for (k = 0; k < numbytes; k++)
            fileval = (fileval << 8) | (c = getc (ifp));
          if (c == EOF) return (0);
        }
      if (num_operator == '&')
        fileval &= num_operatorval;

      if (num_test == '<')
        found = (fileval < num_testval);
      else if (num_test == '>')
        found = (fileval > num_testval);
      else
        found = (fileval == num_testval);

      if (found && (numbytes == 5)) found = 2;
    }
  else if (numbytes == 0) /* String test */
    {
      file_convert_string ((char *)value, (char *)mem_testval,
                           sizeof (mem_testval), &numbytes);
      if (numbytes <= 0) return (0);

      if (offs + numbytes <= headsize)  /* We have it in memory ? */
        {
          found = (memcmp (mem_testval, file_head+offs, numbytes) == 0);
        }
      else   /* Read it from file */
        {
          if (fseek (ifp, offs, SEEK_SET) < 0) return (0);
          found = 1;
          for (k = 0; found && (k < numbytes); k++)
            {
              c = getc (ifp);
              found = (c != EOF) && (c == (int)mem_testval[k]);
            }
        }
    }

  return found;
}

gint
file_check_magic_list (GSList *magics_list,
		       gint    headsize,
		       guchar *head,
		       FILE   *ifp)

{
  /* Return values are 0: no match, 1: magic match, 2: size match */
  gchar *offset;
  gchar *type;
  gchar *value;
  gint   and   = 0;
  gint   found = 0;
  gint   match_val;

  while (magics_list)
    {
      if ((offset = (gchar *)magics_list->data) == NULL) break;
      if ((magics_list = magics_list->next) == NULL) break;
      if ((type = (gchar *)magics_list->data) == NULL) break;
      if ((magics_list = magics_list->next) == NULL) break;
      if ((value = (gchar *)magics_list->data) == NULL) break;
      magics_list = magics_list->next;

      match_val = file_check_single_magic (offset, type, value,
                                           headsize, head, ifp);
      if (and)
	found = found && match_val;
      else
	found = match_val;

      and = (strchr (offset, '&') != NULL);

      if ((!and) && found)
	return match_val;
    }

  return 0;
}

TempBuf *
make_thumb_tempbuf (GimpImage *gimage)
{
  gint w, h;

  if (gimage->width<=80 && gimage->height<=60)
    {
      w = gimage->width;
      h = gimage->height;
    }
  else
    {
      /* Ratio molesting to fit within .xvpic thumbnail size limits */
      if (60 * gimage->width < 80 * gimage->height)
	{
	  h = 60;
	  w = (60 * gimage->width) / gimage->height;
	  if (w == 0)
	    w = 1;
	}
      else
	{
	  w = 80;
	  h = (80 * gimage->height) / gimage->width;
	  if (h == 0)
	    h = 1;
	}
    }

  /*printf("tn: %d x %d -> ", w, h);fflush(stdout);*/

  return gimp_viewable_get_preview (GIMP_VIEWABLE (gimage), w, h);
}

/* The readXVThumb function source may be re-used under
   the XFree86-style license. <adam@gimp.org> */
guchar *
readXVThumb (const gchar  *fnam,
	     gint         *w,
	     gint         *h,
	     gchar       **imginfo /* caller frees if != NULL */)
{
  FILE *fp;
  const gchar *P7_332 = "P7 332";
  gchar P7_buf[7];
  gchar linebuf[200];
  guchar *buf;
  gint twofivefive;
  void *ptr;

  *w = *h = 0;
  *imginfo = NULL;

  fp = fopen (fnam, "rb");
  if (!fp)
    return NULL;

  fread (P7_buf, 6, 1, fp);

  if (strncmp(P7_buf, P7_332, 6)!=0)
    {
      g_warning ("Thumbnail doesn't have the 'P7 332' header.");
      fclose (fp);
      return NULL;
    }

  /*newline*/
  fread (P7_buf, 1, 1, fp);

  do
    {
      ptr = fgets(linebuf, 199, fp);
      if ((strncmp(linebuf, "#IMGINFO:", 9) == 0) &&
	  (linebuf[9] != '\0') &&
	  (linebuf[9] != '\n'))
	{
	  if (linebuf[strlen(linebuf)-1] == '\n')
	    linebuf[strlen(linebuf)-1] = '\0';

	  if (linebuf[9] != '\0')
	    {
	      if (*imginfo)
		g_free(*imginfo);
	      *imginfo = g_strdup (&linebuf[9]);
	    }
	}
    }
  while (ptr && linebuf[0]=='#'); /* keep throwing away comment lines */

  if (!ptr)
    {
      /* g_warning("Thumbnail ended - not an image?"); */
      fclose (fp);
      return NULL;
    }

  sscanf(linebuf, "%d %d %d\n", w, h, &twofivefive);

  if (twofivefive!=255)
    {
      g_warning ("Thumbnail is of funky depth.");
      fclose (fp);
      return NULL;
    }

  if ((*w)<1 || (*h)<1 || (*w)>80 || (*h)>60)
    {
      g_warning ("Thumbnail size bad.  Corrupted?");
      fclose (fp);
      return NULL;
    }

  buf = g_malloc ((*w) * (*h));

  fread (buf, (*w) * (*h), 1, fp);

  fclose (fp);
  
  return buf;
}

gboolean
file_save_thumbnail (GimpImage   *gimage,
		     const gchar *full_source_filename,
		     TempBuf     *tempbuf)
{
  gint               i,j;
  gint               w,h;
  guchar            *tbd;
  gchar             *pathname;
  gchar             *filename;
  gchar             *xvpathname;
  gchar             *thumbnailname;
  GimpImageBaseType  basetype;
  FILE              *fp;
  struct stat        statbuf;

  if (stat (full_source_filename, &statbuf) != 0)
    {
      return FALSE;
    }

  pathname = g_dirname (full_source_filename);
  filename = g_basename (full_source_filename); /* Don't free! */

  xvpathname = g_strconcat (pathname, G_DIR_SEPARATOR_S, ".xvpics",
			    NULL);

  thumbnailname = g_strconcat (xvpathname, G_DIR_SEPARATOR_S,
			       filename,
			       NULL);

  tbd = temp_buf_data (tempbuf);

  w = tempbuf->width;
  h = tempbuf->height;
  /*printf("tn: %d x %d\n", w, h);fflush(stdout);*/

  mkdir (xvpathname, 0755);

  fp = fopen (thumbnailname, "wb");
  g_free (pathname);
  g_free (xvpathname);
  g_free (thumbnailname);

  if (fp)
    {
      basetype = gimp_image_base_type (gimage);

      fprintf (fp,
	       "P7 332\n#IMGINFO:%dx%d %s (%d %s)\n"
	       "#END_OF_COMMENTS\n%d %d 255\n",
	       gimage->width, gimage->height,
	       (basetype == RGB) ? "RGB" :
	       (basetype == GRAY) ? "Greyscale" :
	       (basetype == INDEXED) ? "Indexed" :
	       "(UNKNOWN COLOUR TYPE)",
	       (int)statbuf.st_size,
	       (statbuf.st_size == 1) ? "byte" : "bytes",
	       w, h);

      switch (basetype)
	{
	case INDEXED:
	case RGB:
	  for (i=0; i<h; i++)
	    {
	      /* Do a cheap unidirectional error-spread to look better */
	      gint rerr=0, gerr=0, berr=0, a;

	      for (j=0; j<w; j++)
		{
		  gint32 r,g,b;

		  if (128 & *(tbd + 3))
		    {
		      r = *(tbd++) + rerr;
		      g = *(tbd++) + gerr;
		      b = *(tbd++) + berr;
		      tbd++;
		    }
		  else
		    {
		      a = (( (i^j) & 4 ) << 5) | 64; /* cute. */
		      r = a + rerr;
		      g = a + gerr;
		      b = a + berr;
		      tbd += 4;
		    }

		  r = CLAMP0255 (r);
		  g = CLAMP0255 (g);
		  b = CLAMP0255 (b);

		  fputc(((r>>5)<<5) | ((g>>5)<<2) | (b>>6), fp);

		  rerr = r - ( (r>>5) * 255 ) / 7;
		  gerr = g - ( (g>>5) * 255 ) / 7;
		  berr = b - ( (b>>6) * 255 ) / 3;
		}
	    }
	  break;

	case GRAY:
	  for (i=0; i<h; i++)
	    {
	      /* Do a cheap unidirectional error-spread to look better */
	      gint b3err=0, b2err=0, v, a;

	      for (j=0; j<w; j++)
		{
		  gint32 b3, b2;

		  v = *(tbd++);
		  a = *(tbd++);

		  if (!(128 & a))
		    v = (( (i^j) & 4 ) << 5) | 64;

		  b2 = v + b2err;
		  b3 = v + b3err;

		  b2 = CLAMP0255 (b2);
		  b3 = CLAMP0255 (b3);

		  fputc(((b3>>5)<<5) | ((b3>>5)<<2) | (b2>>6), fp);

		  b2err = b2 - ( (b2>>6) * 255 ) / 3;
		  b3err = b3 - ( (b3>>5) * 255 ) / 7;
		}
	    }
	  break;

	default:
	  g_warning("UNKNOWN GIMAGE TYPE IN THUMBNAIL SAVE");
	  break;
	}

      fclose (fp);
    }
  else /* Error writing thumbnail */
    {
      return FALSE;
    }

  return TRUE;
}
