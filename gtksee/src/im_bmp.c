/*
 * GTK See -- a image viewer based on GTK+
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

/*
 * This library takes codes from:
 * BMP plug-in for the GIMP
 * And 16-bit(64k) BMP support was added
 *
 * 1998/11/24 by Hotaru Lee: fixed: funny spzeile value
 *
 * 2003-09-02: Little changes, added support for OS/2 1.x
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include "im_bmp.h"

#define BitSet(byte, bit)        (((byte) & (bit)) == (bit))
#define ReadOK(file,buffer,len)  (fread(buffer, len, 1, file) != 0)
#define ToL(buffer, off)         (buffer[(off)] | buffer[(off)+1]<<8 | buffer[(off)+2]<<16 | buffer[(off)+3]<<24)
#define ToS(buffer, off)         (buffer[(off)] | buffer[(off)+1]<<8);

gboolean BmpReadColorMap   (FILE *fd,
             guchar buffer[256][3],
             gint number,
             gint size,
             gint *grey);
gboolean BmpReadImage   (FILE *fd,
             gint        len,
             gint        height,
             guchar      cmap[256][3],
             gint        ncols,
             gint        bpp,
             gint        compression,
             gint        spzeile,
             gint        grey,
             BmpLoadFunc func);

gboolean
bmp_get_header(gchar *filename, bmp_info *info)
{
   FILE     *fd;
   guchar   buffer[50];
   gulong   bfSize, reserverd, bfOffs, biSize;

   if ((fd = fopen (filename, "rb")) == NULL)
      return FALSE;

   /* Read first 14-byte header + 4 bytes from bitmap header */
   if (!ReadOK (fd, buffer, 18)               ||
         (buffer[0] !='B' && buffer[1] !='M') ||   /* Windows BMP   */
         (buffer[0] !='B' && buffer[1] !='A'))     /* OS/2 BMP only */
   {
      fclose(fd);
      return FALSE;
   }

   bfSize   = ToL(buffer,  2);   /* Size of the file in bytes        */
   reserverd= ToL(buffer,  6);   /* WORD Reserved1 and Reserved2     */
   bfOffs   = ToL(buffer, 10);   /* Starting position of image data  */
   biSize   = ToL(buffer, 14);   /* Size of bitmap header in bytes   */
                                 /* Windows = 40   OS/2 = 12         */

   if ((biSize!=40) && (biSize!=12)) /* Windows 3.x  OS/2 1.x */
   {
      fclose(fd);
      return FALSE;
   }

   if (!ReadOK (fd, buffer, biSize-4))    /* Read bitmap header */
   {
      fclose(fd);
      return FALSE;
   }

   if (biSize==40)   /* Windows BMP */
   {
      info -> width     = ToL(buffer,  0);   /* Image width in pixels */
      info -> height    = ToL(buffer,  4);   /* Image height in pixels */
      info -> planes    = ToS(buffer,  8);   /* Number of color planes */
      info -> bitCnt    = ToS(buffer, 10);   /* Number of bits per pixel */
      info -> compr     = ToL(buffer, 12);   /* Compression methods used */
      info -> sizeIm    = ToL(buffer, 16);   /* Size of bitmap in bytes */
      info -> xPels     = ToL(buffer, 20);   /* Horizontal resolution in pixels per meter */
      info -> yPels     = ToL(buffer, 24);   /* Vertical resolution in pixels per meter */
      info -> clrUsed   = ToL(buffer, 28);   /* Number of colors in the image */
      info -> clrImp    = ToL(buffer, 32);   /* Minimum number of important colors */
   } else
   {        /* OS/2 BMP */
      info -> width     = ToS(buffer,  0);
      info -> height    = ToS(buffer,  2);
      info -> planes    = ToS(buffer,  4);
      info -> bitCnt    = ToS(buffer,  6);
      info -> compr     = 0;
   }

   fclose(fd);

   if (info -> bitCnt > 24)
      return FALSE;

   return TRUE;
}

gboolean
bmp_load(gchar *filename, BmpLoadFunc func)
{
   FILE     *fd;
   guchar   buffer[50], ColorMap[256][3];
   gulong   bfSize, reserverd, bfOffs, biSize;
   gint     ColormapSize, SpeicherZeile, Maps, Grey;
   bmp_info cinfo;

   if ((fd = fopen (filename, "rb")) == NULL)
      return FALSE;

   /* Read first 14-byte header + 4 bytes from bitmap header */
   if (!ReadOK (fd, buffer, 18)               ||
         (buffer[0] !='B' && buffer[1] !='M') ||   /* Windows BMP   */
         (buffer[0] !='B' && buffer[1] !='A'))     /* OS/2 BMP only */
   {
      fclose(fd);
      return FALSE;
   }

   bfSize   = ToL(buffer,  2);   /* Size of the file in bytes        */
   reserverd= ToL(buffer,  6);   /* WORD Reserved1 and Reserved2     */
   bfOffs   = ToL(buffer, 10);   /* Starting position of image data  */
   biSize   = ToL(buffer, 14);   /* Size of bitmap header in bytes   */
                                 /* Windows = 40 * OS/2 = 12         */

   if (biSize!=40 && biSize!=12) /* Windows 3.x  OS/2 1.x */
   {
      fclose(fd);
      return FALSE;
   }

   if (!ReadOK (fd, buffer, biSize-4))    /* Read bitmap header */
   {
      fclose(fd);
      return FALSE;
   }

   if (biSize==40)   /* Windows BMP */
   {
      cinfo.width     = ToL(buffer,  0);   /* Image width in pixels */
      cinfo.height    = ToL(buffer,  4);   /* Image height in pixels */
      cinfo.planes    = ToS(buffer,  8);   /* Number of color planes */
      cinfo.bitCnt    = ToS(buffer, 10);   /* Number of bits per pixel */
      cinfo.compr     = ToL(buffer, 12);   /* Compression methods used */
      cinfo.sizeIm    = ToL(buffer, 16);   /* Size of bitmap in bytes */
      cinfo.xPels     = ToL(buffer, 20);   /* Horizontal resolution in pixels per meter */
      cinfo.yPels     = ToL(buffer, 24);   /* Vertical resolution in pixels per meter */
      cinfo.clrUsed   = ToL(buffer, 28);   /* Number of colors in the image */
      cinfo.clrImp    = ToL(buffer, 32);   /* Minimum number of important colors */
   } else
   {
      cinfo.width     = ToS(buffer,  0);
      cinfo.height    = ToS(buffer,  2);
      cinfo.planes    = ToS(buffer,  4);
      cinfo.bitCnt    = ToS(buffer,  6);
      cinfo.compr     = 0;
   }


   if (cinfo.bitCnt > 24)
   {
      fclose(fd);
      return FALSE;
   }

   Maps = 4;

   ColormapSize = (bfOffs - biSize - 14) / Maps;
   if ((cinfo.clrUsed == 0) && (cinfo.bitCnt < 24))
      cinfo.clrUsed=ColormapSize;
   if (cinfo.bitCnt == 24 || cinfo.bitCnt == 16)
   {
      SpeicherZeile = ((bfSize - bfOffs) / cinfo.height);
   } else
   {
      SpeicherZeile = ((bfSize - bfOffs) / cinfo.height) * (8 / cinfo.bitCnt);
   }

   if (!BmpReadColorMap(fd, ColorMap, ColormapSize, Maps, &Grey))
   {
      fclose(fd);
      return FALSE;
   }

   if (!BmpReadImage(fd, cinfo.width, cinfo.height, ColorMap,
      cinfo.clrUsed, cinfo.bitCnt, cinfo.compr,
      SpeicherZeile, Grey, func))
   {
      fclose(fd);
      return FALSE;
   }

   fclose(fd);
   return TRUE;
}

gboolean BmpReadColorMap   (FILE *fd,
             guchar buffer[256][3],
             gint number,
             gint size,
             gint *grey)
{
   gint   i;
   guchar rgb[4];

   *grey=(number>2);
   for (i = 0; i < number ; i++)
   {
      if (!ReadOK (fd, rgb, size))
      {
         return FALSE;
      }
      if (size==4) {
         buffer[i][0] = rgb[2];
         buffer[i][1] = rgb[1];
         buffer[i][2] = rgb[0];
      } else {
         buffer[i][0] = rgb[1];
         buffer[i][1] = rgb[0];
         buffer[i][2] = rgb[2];
      }
      *grey=((*grey) && (rgb[0]==rgb[1]) && (rgb[1]==rgb[2]));
   }
   return TRUE;
}

gboolean BmpReadImage   (FILE *fd,
             gint        len,
             gint        height,
             guchar      cmap[256][3],
             gint        ncols,
             gint        bpp,
             gint        compression,
             gint        spzeile,
             gint        grey,
             BmpLoadFunc func)
{
   guchar   v, wieviel, buf[16],
            *dest, *temp;
   gint     xpos=0, ypos=0,
            i, j, pix, egal;

   ypos = height-1; /* Bitmaps begin in the lower left corner */

   dest = g_malloc(sizeof(guchar) * len * 3);

   if (bpp==24)
   {
      while (ReadOK(fd,buf,3))
      {
         temp = dest + (xpos++ * 3);

         *(temp++) = buf[2];
         *(temp++) = buf[1];
         *temp     = buf[0];

         if (xpos == len)
         {
            if (spzeile > len*3)
            {
               fseek(fd, spzeile - (len*3), SEEK_CUR);
            }
            if ((*func) (dest, len, 0, ypos, 3, -1, 0))
               break;

            ypos--;
            xpos=0;
         }

         if (ypos < 0)
            break;
      }
   } else
   if (bpp==16)
   {
      while (ReadOK(fd,buf,2))
      {
         temp = dest + (xpos++ * 3);

         *(temp++) = (buf[1]<<1) & 0xf8;
         *(temp++) = ((buf[1]>>5)& 0x04) | ((buf[1]<<6)&0xc0) | ((buf[0]>>2)&0x38);
         *temp     = (buf[0]<<3) & 0xf8;

         if (xpos == len)
         {
            if ((*func) (dest, len, 0, ypos, 3, -1, 0))
               break;
            ypos--;
            xpos=0;
         }
         if (ypos < 0)
            break;
      }
   } else
   {
      switch(compression)
      {
         case 0:     /* uncompressed */
            while (ReadOK(fd, &v, 1))
            {
               for (i=1; (i<=(8/bpp)) && (xpos<len); i++,xpos++)
               {
                  temp = dest + (xpos * 3);
                  /* look at my bitmask !! */
                  pix = ( v & ( ((1<<bpp)-1) << (8-(i*bpp)) ) ) >> (8-(i*bpp));
                  *(temp++) = cmap[pix][0];
                  *(temp++) = cmap[pix][1];
                  *temp = cmap[pix][2];
               }
               if (xpos == len)
               {
                  if (spzeile>len)
                  {
                     fseek(fd, (spzeile-len)/(8/bpp), SEEK_CUR);
                  }
                  if ((*func) (dest, len, 0, ypos, 3, -1, 0))
                     break;

                  ypos--;
                  xpos=0;
               }

               if (ypos < 0)
                  break;
            }
            break;

         default: /* Compressed images */
            while (TRUE)
            {
               egal = ReadOK(fd, buf, 2);

               /* Count + Color - record */
               if ((guchar) buf[0]!=0)
               {
                  for (j=0;((guchar) j < (guchar) buf[0]) && (xpos<len);)
                  {
                     for (i=1;((i<=(8/bpp)) && (xpos<len) && ((guchar) j < (guchar) buf[0]));i++,xpos++,j++)
                     {
                        temp      = dest + (xpos * 3);
                        pix       = (buf[1] & ( ((1<<bpp)-1) << (8-(i*bpp)) ) ) >> (8-(i*bpp));
                        *(temp++) = cmap[pix][0];
                        *(temp++) = cmap[pix][1];
                        *temp     = cmap[pix][2];
                     }
                  }
               }

               /* uncompressed record */
               if (((guchar) buf[0]==0) && ((guchar) buf[1]>2))
               {
                  wieviel=buf[1];
                  for (j=0;j<wieviel;j+=(8/bpp))
                  {
                     egal = ReadOK(fd, &v, 1);
                     i=1;
                     while ((i<=(8/bpp)) && (xpos<len))
                     {
                        temp = dest + (xpos * 3);
                        pix = (v & ( ((1<<bpp)-1) << (8-(i*bpp)) ) ) >> (8-(i*bpp));
                        *(temp++) = cmap[pix][0];
                        *(temp++) = cmap[pix][1];
                        *temp = cmap[pix][2];
                        i++;
                        xpos++;
                     }
                  }
                  if ( (wieviel / (8/bpp)) % 2)
                     egal = ReadOK(fd, &v, 1);
               }

               /* Zeilenende */
               if (((guchar) buf[0]==0) && ((guchar) buf[1]==0))
               {
                  if ((*func) (dest, len, 0, ypos, 3, -1, 0)) break;
                  ypos--;
                  xpos=0;
               }

               /* Bitmapende */
               if (((guchar) buf[0]==0) && ((guchar) buf[1]==1))
               {
                  break;
               }

               /* Deltarecord */
               if (((guchar) buf[0]==0) && ((guchar) buf[1]==2))
               {
                  xpos+=(guchar) buf[2];
                  ypos+=(guchar) buf[3];
               }
            }
            break;
      }
   }

   g_free(dest);
   return TRUE;
}
