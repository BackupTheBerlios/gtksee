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

/* 2003-12-08: Code for RLE taken from
 *             SGI plug-in for the GIMP
 */

#include <stdio.h>
#include <stdlib.h>

#include "common_tools.h"
#include "im_sgi.h"

#define MAGIC  474
#define ReadOK(file,buffer,len)  (fread(buffer, len, 1, file) != 0)

gint rle_char (FILE *, guchar *, gint );
gint rle_short(FILE *, guchar *, gint );

gboolean
sgi_get_header(gchar *filename, sgi_info *info)
{
   FILE     *zf;
   gchar    buf[14];

   if ((zf = fopen (filename, "rb")) == NULL)
   {
      return FALSE;
   }

   if (!ReadOK (zf, buf, 12) || buf2short(buf, 0) != MAGIC)
   {
      fclose(zf);
      return FALSE;
   }

   info->storage  = buf[2];
   info->bpc      = buf[3];
   info->dimension= buf2short(buf, 4);
   info->xsize    = buf2short(buf, 6);
   info->ysize    = buf2short(buf, 8);
   info->zsize    = buf2short(buf, 10);

   fclose(zf);

   return TRUE;
}

gboolean
sgi_load (gchar *filename, SgiLoadFunc func)
{
   gint     i, line, size;    /* Looping var */
   guchar   *channelrow[4]={NULL, NULL, NULL, NULL};
   guchar   *buffer;
   glong    jump;
   FILE     *zf;
   sgi_info sgihdr;
   gulong   *start = NULL; /* Array for offset */

   if (sgi_get_header(filename, &sgihdr) == FALSE)
   {
      return FALSE;
   }

   if ((zf = fopen (filename, "rb")) == NULL)
   {
      return FALSE;
   }

   fseek(zf, 512, SEEK_SET);

   if (sgihdr.storage) /* Is compressed ? */
   {
      gchar c[sizeof(gulong)];
      gint  j;

      size  = sgihdr.ysize * sgihdr.zsize * sizeof(gulong);
      start = (gulong *) g_malloc(size);

      for (j=0; j<sgihdr.zsize; j++)
      {
         for (i=0; i<sgihdr.ysize; i++)
         {
            ReadOK(zf, c, sizeof(gulong));
            start[i + j*sgihdr.ysize] = buf2long(c, 0);
         }
      }

      fseek(zf, size, SEEK_CUR);
   }

   size  = sgihdr.xsize * sgihdr.bpc;
   jump  = sgihdr.xsize * sgihdr.ysize;
   buffer= (guchar *) g_malloc(size * 3);

   for (i=0; i<sgihdr.zsize; i++)
   {
      channelrow[i] = (guchar *) g_malloc(size);
   }

   for (i=sgihdr.ysize-1, line=0; i>=0; i--, line ++)
   {
      gint     j, k;
      guchar   *data;

      data = buffer;

      switch (sgihdr.storage)
      {
         case 0 :    /* Uncompressed */
            for (j=0; j<sgihdr.zsize; j++)
            {
               fseek(zf, (512 + jump * j + sgihdr.xsize * i), SEEK_SET);
               fread(channelrow[j], size, 1, zf);    /* Read data */
            }
            break;

         case 1 :    /* RLE */
            for (j=0; j<sgihdr.zsize; j++)
            {
               fseek(zf, start[i + j*sgihdr.ysize], SEEK_SET);

               if (sgihdr.bpc == 1)
               {
                  if (rle_char(zf, channelrow[j], sgihdr.xsize) == -1) break;
               } else
               {
                  if (rle_short(zf, channelrow[j], sgihdr.xsize) == -1) break;
               }
            }
            break;
      }

      if (sgihdr.zsize == 1)
      {
         channelrow[1] =
         channelrow[2] = channelrow[0];
      }

      for (k=0; k<sgihdr.xsize; k++)
      {
         if (sgihdr.bpc == 1)
         {
            *data++ = channelrow[0][k];
            *data++ = channelrow[1][k];
            *data++ = channelrow[2][k];
         } else
         {
            *data++ = buf2short(&channelrow[0][k], 0);
            *data++ = buf2short(&channelrow[1][k], 0);
            *data++ = buf2short(&channelrow[2][k], 0);
         }
      }

      if ((*func) (buffer, sgihdr.xsize, 0, line, 3, -1, 0)) break;
   }

   fclose(zf);
   g_free (buffer);

   for (i=0; i<sgihdr.zsize; i++)
   {
      g_free(channelrow[i]);
   }
   if (start != NULL)
   {
      g_free(start);
   }

   return TRUE;
}

gint
rle_char(FILE *fp, guchar *row, gint xsize)
{
   gint  i, ch, count, length;

   length = 0;

   while (xsize > 0)
   {
      if ((ch = getc(fp)) == EOF)
         return (-1);
      length ++;

      count = ch & 127;
      if (count == 0)
         break;

      if (ch & 128)
      {
         for (i = 0; i < count; i ++, row ++, xsize --, length ++)
            *row = getc(fp);
      } else
      {
         ch = getc(fp);
         length ++;
         for (i = 0; i < count; i ++, row ++, xsize --)
            *row = ch;
      };
   };

   return (xsize > 0 ? -1 : length);
}

gint
rle_short(FILE *fp, guchar *row, gint xsize)
{
   gint     i, ch, count, length;
   guchar   c[2];

   length = 0;

   while (xsize > 0)
   {
      ReadOK(fp, c, 2);
      if ((ch = buf2short(c, 0)) == EOF)
         return (-1);
      length ++;

      count = ch & 127;
      if (count == 0)
         break;

      if (ch & 128)
      {
         for (i = 0; i < count; i ++, row ++, xsize --, length ++)
         {
            ReadOK(fp, c, 2);
            *row = buf2short(c, 0);
         }
      } else
      {
         ReadOK(fp, c, 2);
         ch = buf2short(c, 0);
         length ++;

         for (i = 0; i < count; i ++, row ++, xsize --)
            *row = ch;
      };
   };

   return (xsize > 0 ? -1 : length * 2);
}
