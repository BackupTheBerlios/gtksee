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

/* 2003-11-10: This library takes codes from
 *             SUN plug-in for the GIMP
 */

#include <stdio.h>
#include <string.h>

#include "im_sun.h"
#include "common_tools.h"

#define RAS_MAGIC 0x59a66a95
#define RAS_TYPE_STD 1        /* Standard uncompressed format */
#define RAS_TYPE_RLE 2        /* Runlength compression format */

#define rle_getc(fp)       ((rlebuf.n > 0)                  \
                                 ? (rlebuf.n)--, rlebuf.val \
                                 : rle_fgetc (fp))

gboolean load_sun_d1 (FILE *, sun_info *, guchar *, SunLoadFunc);
gboolean load_sun_d24(FILE *, sun_info *, guchar *, SunLoadFunc);
gboolean load_sun_d8 (FILE *, sun_info *, guchar *, SunLoadFunc);
gboolean load_sun_d32(FILE *, sun_info *, guchar *, SunLoadFunc);

void rle_startread(FILE *);
gint rle_fread    (char *, int, int, FILE *);
gint rle_fgetc    (FILE *);

/* Buffer used */
static RLEBUF rlebuf;


/* Begin the code */

gboolean
sun_get_header (gchar *filename, sun_info *info)
{
   FILE     *zf;
   guchar   buf[sizeof(sun_info)];

   if ((zf = fopen (filename, "rb")) == NULL)
   {
      return FALSE;
   }

   if (fread(&buf[0], sizeof(sun_info), 1, zf) != 1)
   {
      fclose(zf);
      return FALSE;
   }

   fclose(zf);

   if ((info->magicnumber=buf2long(buf, 0)) != RAS_MAGIC)
   {
      return FALSE;
   }

   info->width          = buf2long(buf, 4);
   info->height         = buf2long(buf, 8);
   info->depth          = buf2long(buf, 12);
   info->length         = buf2long(buf, 16);
   info->type           = buf2long(buf, 20);
   info->colormaptype   = buf2long(buf, 24);
   info->colormaplength = buf2long(buf, 28);

   if ((info->type < 1) || (info->type > 5))
   {
      return FALSE;
   }

   return TRUE;
}

gboolean
sun_load (gchar *filename, SunLoadFunc func)
{
   gint32      image_ID;
   FILE        *ifp;
   sun_info    sunhdr;
   guchar      *suncolmap = NULL;
   guchar      buf[sizeof(sun_info)];
   gint        err;

   ifp = fopen (filename, "rb");
   if (!ifp)
   {
      return FALSE;
   }

   if (fread(&buf[0], sizeof(sun_info), 1, ifp) != 1)
   {
      fclose(ifp);
      return FALSE;
   }

   if ((sunhdr.magicnumber=buf2long(buf, 0)) != RAS_MAGIC)
   {
      return FALSE;
   }

   sunhdr.width          = buf2long(buf, 4);
   sunhdr.height         = buf2long(buf, 8);
   sunhdr.depth          = buf2long(buf, 12);
   sunhdr.length         = buf2long(buf, 16);
   sunhdr.type           = buf2long(buf, 20);
   sunhdr.colormaptype   = buf2long(buf, 24);
   sunhdr.colormaplength = buf2long(buf, 28);

   if ((sunhdr.type < 1) || (sunhdr.type > 5))
   {
      return FALSE;
   }

   err = 0;

   /* Is there a RGB colourmap ? */
   if ((sunhdr.colormaptype == 1) && (sunhdr.colormaplength > 0))
   {
      gint ncols;

      ncols = sunhdr.colormaplength / 3;
      if (ncols <= 0)
      {
         err = 1;
         goto data_short;
      }

      suncolmap = g_malloc (sizeof (guchar) * sunhdr.colormaplength);

      if (suncolmap == NULL)
      {
         err = 1;
         goto data_short;
      }

      if (fread (suncolmap, 3, ncols, ifp) != ncols)
      {
         err = 1;
         goto data_short;
      }
   } else if (sunhdr.colormaplength > 0)
   {
      fseek (ifp,
               (sizeof(sun_info)/sizeof (gulong)) * 4 + sunhdr.colormaplength,
               SEEK_SET);
   }

   switch (sunhdr.depth)
   {
      case 1:    /* bitmap */
         err = load_sun_d1 (ifp, &sunhdr, suncolmap, func);
         break;

      case 8:    /* 256 colours */
         err = load_sun_d8 (ifp, &sunhdr, suncolmap, func);
         break;

      case 24:   /* True color */
         err = load_sun_d24 (ifp, &sunhdr, suncolmap, func);
         break;

      case 32:   /* True color with extra byte */
         err = load_sun_d32 (ifp, &sunhdr, suncolmap, func);
         break;

      default:
         image_ID = -1;
         break;
   }

data_short:
   fclose (ifp);
   if (suncolmap != NULL)
   {
      g_free (suncolmap);
   }

   return (err ? FALSE : TRUE);
}


/* Load SUN-raster-file with depth 1 */
gboolean
load_sun_d1 (FILE *ifp, sun_info *sunhdr, guchar *suncolmap, SunLoadFunc func)
{
   gint     pix8, val;
   gint     width, height, linepad;
   gint     i, j, k;
   guchar   *dest, *data;
   gint     err = 0,
            rle = (sunhdr->type == RAS_TYPE_RLE);

   width = sunhdr->width;
   height= sunhdr->height;

   data     = g_malloc (width * 3);
   linepad  = (((sunhdr->width+7)/8) % 2); /* Check for 16bit align */

   if (rle)
   {
      rle_startread (ifp);
   }

   for (i = 0; i < height; i++)
   {
      j     = width;
      dest  = data;

      while (j >= 8)
      {
         pix8 = rle ? rle_getc (ifp) : getc (ifp);
         if (pix8 < 0)
         {
            err = 1;
            break;
         }

         for (k = 7; k >= 0; k--)
         {
            val = ((pix8 & (1 << k)) != 0) ? 0x00 : 0xff;
            *(dest++) = val;
            *(dest++) = val;
            *(dest++) = val;
         }
         j -= 8;
      }

      if (j > 0)
      {
         pix8 = rle ? rle_getc (ifp) : getc (ifp);
         if (pix8 < 0)
         {
            err = 1;
            break;
         }

         for (k = 7; k > (7 -j); k--)
         {
            val = ((pix8 & (1 << k)) != 0) ? 0x00 : 0xff;
            *(dest++) = val;
            *(dest++) = val;
            *(dest++) = val;
         }
      }

      if (linepad)
      {
         if ((err = ((rle ? rle_getc (ifp) : (getc (ifp)) < 0))))
            break;
      }

      if ((*func) (data, width, 0, i, 3, -1, 0)) break;
   }

   g_free (data);

   return (err ? FALSE : TRUE);
}


/* Load SUN-raster-file with depth 8 */

gboolean
load_sun_d8 (FILE *ifp, sun_info *sunhdr, guchar *suncolmap, SunLoadFunc func)
{
   gint   width, height, linepad, i, j, k;
   gint   greyscale, ncols;
   guchar *data, *buf;
   gint   err = 0,
          rle = (sunhdr->type == RAS_TYPE_RLE);

   width = sunhdr->width;
   height= sunhdr->height;

   /* This could also be a greyscale image. Check it */
   ncols = sunhdr->colormaplength / 3;

   greyscale = 1;  /* Also greyscale if no colourmap present */

   if ((ncols > 0) && (suncolmap != NULL))
   {
      for (j = 0; j < ncols; j++)
      {
         if (   (suncolmap[j]        != j)
             || (suncolmap[j+ncols]  != j)
             || (suncolmap[j+2*ncols]!= j))
         {
            greyscale = 0;
            break;
         }
      }
   }

   data = g_malloc (width * 3);
   buf  = g_malloc (width);

   linepad = (sunhdr->width % 2);

   if (rle)
   {
      rle_startread (ifp);  /* Initialize RLE-buffer */
   }

   for (i = 0; i < height; i++)
   {
      err  = (  (rle)
              ? (rle_fread ((char *)buf, 1, width, ifp)) != width
              : (fread ((char *)buf, 1, width, ifp))     != width);

      if (linepad)
      {
         err |= ((rle ? rle_getc (ifp) : getc (ifp)) < 0);
      }

      if (err)
      {
         break;
      }

      k = 0;
      for (j=0; j < width; j++)
      {
         if (greyscale == 1)
         {
            data[k++] = buf[j];
            data[k++] = buf[j];
            data[k++] = buf[j];
         } else
         {
            data[k++] = suncolmap[buf[j]];
            data[k++] = suncolmap[buf[j]+ncols];
            data[k++] = suncolmap[buf[j]+2*ncols];
         }
      }

      if ((*func) (data, width, 0, i, 3, -1, 0)) break;
   }

   g_free (data);
   g_free (buf);

   return (err ? FALSE : TRUE);
}


/* Load SUN-raster-file with depth 24 */
gboolean
load_sun_d24 (FILE *ifp, sun_info *sunhdr, guchar *suncolmap, SunLoadFunc func)
{
   guchar   *dest, blue;
   guchar   *data;
   gint     width, height, linepad;
   gint     i, j;
   int      err = 0,
            rle = (sunhdr->type == RAS_TYPE_RLE);

   width = sunhdr->width;
   height= sunhdr->height;

   data     = g_malloc (width * 3);
   linepad  = ((sunhdr->width * 3) % 2);

   if (rle)
      rle_startread (ifp);  /* Initialize RLE-buffer */

   for (i = 0; i < height; i++)
   {
      dest = data;

      err |= ((rle ? rle_fread ((char *)dest, 3, width, ifp)
               : fread ((char *)dest, 3, width, ifp)) != width);

      if (linepad)
      {
         err |= ((rle ? rle_getc (ifp) : getc (ifp)) < 0);
      }

      if (err)
      {
         break;
      }

      if (sunhdr->type != 3)  /* We have BGR format. Correct it */
      {
         dest = data;
         for (j = 0; j < width; j++)
         {
            blue      = *dest;
            *dest     = *(dest+2);
            *(dest+2) = blue;
            dest     += 3;
         }
      }

      if ((*func) (data, width, 0, i, 3, -1, 0)) break;

   }

   g_free (data);

   return (err ? FALSE : TRUE);
}

/* Load SUN-raster-file with depth 32 */

gboolean
load_sun_d32 (FILE *ifp, sun_info *sunhdr, guchar *suncolmap, SunLoadFunc func)
{
   guchar   *dest, blue;
   guchar   *data;
   gint     width, height;
   gint     i, j;
   gint     err = 0, cerr,
            rle = (sunhdr->type == RAS_TYPE_RLE);

   width = sunhdr->width;
   height= sunhdr->height;

  /* initialize */

   cerr = 0;

   data = g_malloc (width * 3);

   if (rle)
   {
      rle_startread (ifp);  /* Initialize RLE-buffer */
   }

   dest = data;

   for (i = 0; i < height; i++)
   {
      if (rle)
      {
         for (j = 0; j < width; j++)
         {
            rle_getc (ifp);   /* Skip unused byte */
            *(dest++) = rle_getc (ifp);
            *(dest++) = rle_getc (ifp);
            *(dest++) = (cerr = (rle_getc (ifp)));
         }
      } else
      {
         for (j = 0; j < width; j++)
         {
            getc (ifp);   /* Skip unused byte */
            *(dest++) = getc (ifp);
            *(dest++) = getc (ifp);
            *(dest++) = (cerr = (getc (ifp)));
         }
      }

      err |= (cerr < 0);

      if (sunhdr->type != 3) /* BGR format ? Correct it */
      {
         for (j = 0; j < width; j++)
         {
            dest -= 3;
            blue = *dest;
            *dest = *(dest+2);
            *(dest+2) = blue;
         }
         dest += width*3;
      }

   }

   g_free (data);

   return (err ? FALSE : TRUE);
}

/* Start reading Runlength Encoded Data */
void
rle_startread (FILE *ifp)
{
   rlebuf.val=        /* Clear RLE-buffer */
   rlebuf.n  = 0;
}


/* Read uncompressed elements from RLE-stream */
gint
rle_fread (gchar *ptr, gint sz, gint nelem, FILE *ifp)
{
   int elem_read, cnt, val, err = 0;

   for (elem_read = 0; elem_read < nelem; elem_read++)
   {
      for (cnt = 0; cnt < sz; cnt++)
      {
         val = rle_getc (ifp);
         if (val < 0)
         {
            err = 1;
            break;
         }
         *(ptr++) = (char)val;
      }
      if (err) break;
   }

   return (elem_read);
}


/* Get one byte of uncompressed data from RLE-stream */
gint
rle_fgetc (FILE *ifp)
{
   int flag, runcnt, runval;

   if (rlebuf.n > 0)    /* Something in the buffer ? */
   {
      (rlebuf.n)--;
      return (rlebuf.val);
   }

   /* Nothing in the buffer. We have to read something */
   if ((flag = getc (ifp)) < 0)
      return (-1);

   if (flag != 0x0080)
      return (flag);    /* Single byte run ? */

   if ((runcnt = getc (ifp)) < 0)
      return (-1);

   if (runcnt == 0)
      return (0x0080);     /* Single 0x80 ? */

   /* The run */
   if ((runval = getc (ifp)) < 0)
      return (-1);

   rlebuf.n    = runcnt;
   rlebuf.val  = runval;

   return (runval);
}

