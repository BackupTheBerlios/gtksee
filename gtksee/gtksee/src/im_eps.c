/*
 * GTK See -- an image viewer based on GTK+
 * Copyright (C) 1998 Hotaru Lee <jkhotaru@mail.sti.com.cn> <hotaru@163.net>
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

/* 2003-12-08: This library takes codes from
 *             PS plug-in for the GIMP
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "im_eps.h"

#define DEFAULT_GS_PROG "gs"
#define STR_LENGTH      1024

/* Definition of default values */
#define RESOLUTION      100
#define WIDTH           826
#define HEIGHT          1170
#define COLOUR          6
#define TEXT_ALIASING   1
#define GRAPH_ALIASING  1

/* Declare some local functions */

gboolean get_bbox          (FILE *ifp,
                              gint *x0, gint *y0,
                              gint *x1, gint *y1);
FILE     *ps_open          (gchar *filename);
void     ps_close          (FILE *ifp);
gint     read_pnmraw_type  (FILE *ifp, gint *width, gint *height, gint *maxval);
gchar    *g_shell_quote    (const gchar *unquoted_string);

/* Global variables */
gint  llx, lly, urx, ury;
gint  offx, offy;

/* Begin the code ... */

gboolean
eps_get_header(gchar *filename, eps_info *info)
{
   FILE  *fd;
   gint  maybe_epsf, is_epsf;
   gint  x0, y0, x1, y1;
   gchar error = 1;

   llx   =
   lly   =
   urx   =
   ury   =
   offx  =
   offy  = 0;

   is_epsf     = 0;  /* Check if it is a EPS-file */
   maybe_epsf  = 0;

   if ((fd = fopen (filename, "rb")) == NULL)
   {
      return FALSE;
   } else
   {
      gchar    hdr[STR_LENGTH];
      gchar    *adobe, *epsf;
      gint     ds = 0;
      guchar   doseps[5] = { 0xc5, 0xd0, 0xd3, 0xc6, 0 };

      fread (hdr, 1, sizeof(hdr), fd);

      /* Check if the file is a PDF */
      if (hdr[0] == '%' && hdr[1] == 'P' && hdr[2] == 'D' && hdr[3] == 'F')
      {
         goto stop;
      }

      hdr[sizeof(hdr)-1] = '\0';

      if ((adobe = strstr (hdr, "PS-Adobe-")) == NULL)
      {
         goto stop;
      }

      if ((epsf = strstr (hdr, "EPSF-")) != NULL)
         ds = epsf - adobe;

      is_epsf = ((ds >= 11) && (ds <= 15));

      /* Illustrator uses negative values in BoundingBox without marking */
      /* files as EPSF. Try to handle that. */
      maybe_epsf = (strstr (hdr, "%%Creator: Adobe Illustrator(R) 8.0") != 0);

      /* Check DOS EPS binary file */
      if ((!is_epsf) && (strncmp (hdr, (char *)doseps, 4) == 0))
         is_epsf = 1;

      info->width = WIDTH;
      info->height= HEIGHT;
      urx         = WIDTH-1;
      ury         = HEIGHT-1;

      if (get_bbox (fd, &x0, &y0, &x1, &y1))
      {
         if (maybe_epsf && ((x0 < 0) || (y0 < 0)))
            is_epsf = 1;

         if (is_epsf)  /* Handle negative BoundingBox for EPSF */
         {
            offx  = -x0; x1 += offx; x0 += offx;
            offy  = -y0; y1 += offy; y0 += offy;
         }

         if ((x0 >= 0) && (y0 >= 0) && (x1 > x0) && (y1 > y0))
         {
            llx         = (int)((x0/72.0) * RESOLUTION + 0.0001);
            lly         = (int)((y0/72.0) * RESOLUTION + 0.0001);
            urx         = (int)((x1/72.0) * RESOLUTION + 0.5);
            ury         = (int)((y1/72.0) * RESOLUTION + 0.5);
            info->width = urx;
            info->height= ury;
         }
      }

      info->resolution     = RESOLUTION;
      info->pnm_type       = COLOUR;
      info->textalpha      = TEXT_ALIASING;
      info->graphicsalpha  = GRAPH_ALIASING;
      info->is_epsf        = is_epsf;

      error = 0;
   }

stop:
   fclose(fd);

   return (error ? FALSE : TRUE);
}

gboolean
eps_load (gchar *filename, EpsLoadFunc func)
{
   guchar   *dest;
   gboolean retval;
   FILE     *ifp;
   gint     width, height, maxval;
   gint     image_width, image_height;
   gint     skip_left, skip_bottom, i;

   ifp = ps_open (filename);
   if (!ifp)
   {
      return FALSE;
   }

   /* Load PNM image generated from PostScript file */
   if (read_pnmraw_type (ifp, &width, &height, &maxval) != 6)
      return FALSE;

   if ((width == urx+1) && (height == ury+1))  /* gs respected BoundingBox ? */
   {
      skip_left      = llx;
      skip_bottom    = lly;
      image_width    = width - skip_left;
      image_height   = height - skip_bottom;
   } else
   {
      skip_left   =
      skip_bottom = 0;
      image_width = width;
      image_height= height;
   }

   /* Read a PPM raw rgb file */
   dest = g_malloc(sizeof(guchar) * image_width * 3);
   for (i = 0; i< image_height; i++)
   {
      if (!fread(dest, image_width * 3, 1, ifp)) break;
      if ((*func) (dest, image_width, 0, i, 3, -1, 0)) break;
   }
   g_free(dest);

   ps_close (ifp);

   return (retval ? TRUE : FALSE);
}

/* A function like fgets, but treats single CR-character as line break. */
/* As a line break the newline-character is returned. */
static char *
psfgets (char *s, int size, FILE *stream)
{
   int   c;
   char  *sptr = s;

   if (size <= 0) return NULL;

   if (size == 1)
   {
      *s = '\0';
      return NULL;
   }
   c = getc (stream);
   if (c == EOF) return NULL;

   for (;;)
   {
      /* At this point we have space in sptr for at least two characters */
      if (c == '\n')    /* Got end of line (UNIX line end) ? */
      {
         *(sptr++) = '\n';
         break;
      } else if (c == '\r')  /* Got a carriage return. Check next charcater */
      {
         c = getc (stream);
         if ((c == EOF) || (c == '\n')) /* EOF or DOS line end ? */
         {
            *(sptr++) = '\n';  /* Return UNIX line end */
            break;
         } else  /* Single carriage return. Return UNIX line end. */
         {
            ungetc (c, stream);  /* Save the extra character */
            *(sptr++) = '\n';
            break;
         }
      } else   /* no line end character */
      {
         *(sptr++) = (char)c;
         size--;
      }
      if (size == 1) break;  /* Only space for the nul-character ? */
      c = getc (stream);
      if (c == EOF) break;
   }
   *sptr = '\0';
   return s;
}


/* Get the BoundingBox of a PostScript file. On success, 0 is returned. */
/* On failure, -1 is returned. */
gboolean
get_bbox (FILE *ifp, gint *x0, gint *y0, gint *x1, gint *y1)
{
   gchar line[STR_LENGTH], *src;
   gint  retval = 0;

   fseek(ifp, 0L, SEEK_SET);

   for (;;)
   {
      if (psfgets (line, sizeof (line)-1, ifp) == NULL)
         break;

      if ((line[0] != '%') || (line[1] != '%'))
         continue;

      src = &(line[2]);
      while ((*src == ' ') || (*src == '\t'))
         src++;

      if (strncmp (src, "BoundingBox", 11) != 0)
         continue;

      src += 11;

      while ((*src == ' ') || (*src == '\t') || (*src == ':'))
         src++;

      if (strncmp (src, "(atend)", 7) == 0)
         continue;

      if (sscanf (src, "%d%d%d%d", x0, y0, x1, y1) == 4)
         retval = 1;

      break;
   }

   return (retval ? TRUE : FALSE);
}

/* Open the PostScript file. On failure, NULL is returned. */
/* The filepointer returned will give a PNM-file generated */
/* by the PostScript-interpreter. */
FILE *
ps_open (gchar *filename)
{
   gchar    *cmd, *gs, *gs_opts, *quoted_fn;
   FILE     *fd_popen;
   gint     width, height, resolution;
   gchar    TextAlphaBits[64], GraphicsAlphaBits[64], geometry[32];
   gchar    offset[32];
   gint     is_epsf;
   eps_info loadopt;

   if (eps_get_header(filename, &loadopt) == FALSE)
      return NULL;

   resolution  = loadopt.resolution;
   width       = loadopt.width;
   height      = loadopt.height;
   is_epsf     = loadopt.is_epsf;

   gs = getenv ("GS_PROG");

   if (gs == NULL)
      gs = DEFAULT_GS_PROG;

   quoted_fn = g_shell_quote (filename);

   gs_opts = getenv ("GS_OPTIONS");
   if (gs_opts == NULL)
      gs_opts = "-dSAFER";
   else
      gs_opts = "";  /* Ghostscript will add these options */

   TextAlphaBits[0]     =
   GraphicsAlphaBits[0] =
   geometry[0]          =
   offset[0]            = '\0';

   /* Offset command for gs to get image part with negative x/y-coord. */
   if ((offx != 0) || (offy != 0))
      sprintf (offset, "-c %d %d translate ", offx, offy);

   /* Antialiasing not available for PBM-device */
   if ((loadopt.pnm_type != 4) && (loadopt.textalpha != 1))
      sprintf (TextAlphaBits, "-dTextAlphaBits=%d", (int)loadopt.textalpha);

   if ((loadopt.pnm_type != 4) && (loadopt.graphicsalpha != 1))
      sprintf (GraphicsAlphaBits, "-dGraphicsAlphaBits=%d", (int)loadopt.graphicsalpha);

   cmd = g_strdup_printf ("%s -sDEVICE=ppmraw -r%d -g%dx%d %s %s -q "
                           "-dNOPAUSE %s -sOutputFile=- %s -f %s %s -c quit",
                           gs, resolution, width, height,
                           TextAlphaBits, GraphicsAlphaBits,
                           gs_opts, offset, quoted_fn,
                           (is_epsf ? "-c showpage" : ""));

   fd_popen = popen (cmd, "r");

   g_free (cmd);
   g_free (quoted_fn);

   return (fd_popen);
}

/* Close the PNM-File of the PostScript interpreter */
void
ps_close (FILE *ifp)
{
   gchar buf[STR_LENGTH];

   while (fgets(buf, STR_LENGTH-1, ifp) != NULL)
      ;

   pclose (ifp);  /* Finish reading from pipe. */
}

/* Read the header of a raw PNM-file and
 * return type (4-6) or 0 on failure */
gint
read_pnmraw_type (FILE *ifp, gint *width, gint *height, gint *maxval)
{
   gint  frst, scnd, thrd, pnmtype;
   gchar line[STR_LENGTH];

   /* GhostScript may write some informational messages infront of
    * the header. We are just looking at a Px\n in the input stream.
    */
   frst = getc (ifp);
   scnd = getc (ifp);
   thrd = getc (ifp);

   for (;;)
   {
      if (thrd == EOF)
         return 0;

      if ((thrd == '\n') && (frst == 'P') && (scnd >= '1') && (scnd <= '6'))
         break;

      frst = scnd;
      scnd = thrd;
      thrd = getc (ifp);
   }
   pnmtype = scnd - '0';

   /* We dont use the ASCII-versions */
   if ((pnmtype >= 1) && (pnmtype <= 3))
      return 0;

   /* Read width/height */
   for (;;)
   {
      if (fgets (line, sizeof (line)-1, ifp) == NULL)
         return 0;
      if (line[0] != '#')
         break;
   }
   if (sscanf (line, "%d%d", width, height) != 2)
      return 0;

   *maxval = 255;

   if (pnmtype != 4)  /* Read maxval */
   {
      for (;;)
      {
         if (fgets (line, sizeof (line)-1, ifp) == NULL)
            return 0;
         if (line[0] != '#')
            break;
      }
      if (sscanf (line, "%d", maxval) != 1)
         return FALSE;
   }
   return (pnmtype);
}

/* Misc utility from glib2 */
gchar *
g_shell_quote (const gchar *unquoted_string)
{
   /* We always use single quotes, because the algorithm is cheesier.
    * We could use double if we felt like it, that might be more
    * human-readable.
    */

   const gchar *p;
   GString *dest;
   gchar *ret;

   g_return_val_if_fail (unquoted_string != NULL, NULL);

   dest = g_string_new ("'");

   p = unquoted_string;

   /* could speed this up a lot by appending chunks of
    * text at a time.
    */
   while (*p)
   {
      /* Replace literal ' with a close ', a \', and a open ' */
      if (*p == '\'')
         g_string_append (dest, "'\\''");
      else
         g_string_append_c (dest, *p);

      ++p;
   }

   /* close the quote */
   g_string_append_c (dest, '\'');

   ret = dest->str;
   g_string_free (dest, FALSE);

   return ret;
}
