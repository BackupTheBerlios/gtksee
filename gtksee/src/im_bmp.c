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

/*
 * This library takes codes from:
 * BMP plug-in for the GIMP
 * And 16-bit(64k) BMP support was added
 *
 * 1998/11/24 by Hotaru Lee: fixed: funny spzeile value
 *
 * 2003-09-02: Little changes, added support for OS/2 1.x
 *
 * 2003-11-02: Minors changes for support OS/2 2.2.x and 2.1.x files
 *             Work with 4, 8 and 24 bit RLE algorithm
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include "im_bmp.h"

#define ReadOK(file,buffer,len)  (fread(buffer, len, 1, file) != 0)
#define ToL(buffer, off)         (buffer[(off)] | buffer[(off)+1]<<8 | buffer[(off)+2]<<16 | buffer[(off)+3]<<24)
#define ToS(buffer, off)         (buffer[(off)] | buffer[(off)+1]<<8);

gboolean BmpReadColorMap   (FILE *fd,
             RGB2 buffer[256],
             gint number,
             gint size);

gboolean BmpReadImage   (FILE *fd,
            gint        width,
            gint        height,
            RGB2        cmap[256],
            gint        ncols,
            gint        bpp,
            gint        compression,
            gint        spzeile,
            BmpLoadFunc func);

gboolean
bmp_get_header(gchar *filename, bmp_info *info)
{
   FILE     *fd;
   guchar   buffer[64];
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

   if ((biSize!=40) && (biSize!=12) && (biSize!=64)) /* Windows 3.x  OS/2 1.x 2.x.x */
   {
      fclose(fd);
      return FALSE;
   }

   if (!ReadOK (fd, buffer, biSize-4))    /* Read bitmap header */
   {
      fclose(fd);
      return FALSE;
   }

   fclose(fd);

   if (biSize==40 || biSize==64)   /* Windows BMP, OS/2 2.2.x*/
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
   {        /* OS/2 1.x */
      info -> width     = ToS(buffer,  0);
      info -> height    = ToS(buffer,  2);
      info -> planes    = ToS(buffer,  4);
      info -> bitCnt    = ToS(buffer,  6);
      info -> compr     = 0;
   }

   if (info -> bitCnt > 24 || info -> compr == 3)
      return FALSE;

   return TRUE;
}

gboolean
bmp_load(gchar *filename, BmpLoadFunc func)
{
   FILE     *fd;
   guchar   buffer[64];
   RGB2     ColorMap[256];
   gulong   bfSize, reserverd, bfOffs, biSize;
   gint     ColormapSize, SpeicherZeile, Maps;
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

   if (biSize!=40 && biSize!=12 && biSize!=64) /* Windows 3.x  OS/2 1.x */
   {
      fclose(fd);
      return FALSE;
   }

   if (!ReadOK (fd, buffer, biSize-4))    /* Read bitmap header */
   {
      fclose(fd);
      return FALSE;
   }

   Maps = 3;

   switch (biSize)
   {
      case 12:    /* OS/2 1.x or 2.1.x*/
         cinfo.width    = ToS(buffer,  0);
         cinfo.height   = ToS(buffer,  2);
         cinfo.planes   = ToS(buffer,  4);
         cinfo.bitCnt   = ToS(buffer,  6);
         cinfo.compr    = 0;
         cinfo.clrUsed  = 0;
         break;

      case 40:    /* Windows BMP */
      case 64:    /* OS/2 2.2.x  */
         Maps ++;
         cinfo.width    = ToL(buffer,  0);   /* Image width in pixels */
         cinfo.height   = ToL(buffer,  4);   /* Image height in pixels */
         cinfo.planes   = ToS(buffer,  8);   /* Number of color planes */
         cinfo.bitCnt   = ToS(buffer, 10);   /* Number of bits per pixel */
         cinfo.compr    = ToL(buffer, 12);   /* Compression methods used */
         cinfo.sizeIm   = ToL(buffer, 16);   /* Size of bitmap in bytes */
         cinfo.xPels    = ToL(buffer, 20);   /* Horizontal resolution in pixels per meter */
         cinfo.yPels    = ToL(buffer, 24);   /* Vertical resolution in pixels per meter */
         cinfo.clrUsed  = ToL(buffer, 28);   /* Number of colors in the image */
         cinfo.clrImp   = ToL(buffer, 32);   /* Minimum number of important colors */
         break;

      default:
         fclose(fd);
         return FALSE;
         break;
   }

   if (cinfo.bitCnt > 24 || cinfo.compr == 3)
      return FALSE;

   /* Calculate the number of color palette entries */
   ColormapSize = (bfOffs - biSize - 14) / Maps;

   if (cinfo.clrUsed == 0)
   {
      if (cinfo.bitCnt <= 8)
      {
         cinfo.clrUsed =
         ColormapSize  = 1 << cinfo.bitCnt;
      } else if (cinfo.bitCnt < 24)
      {
         cinfo.clrUsed = ColormapSize;
      }
   }

   if (ColormapSize != 0)
   {
      if (!BmpReadColorMap(fd, ColorMap, ColormapSize, Maps))
      {
         fclose(fd);
         return FALSE;
      }
   }

   switch (cinfo.bitCnt)
   {
      case 16:
      case 24:
         SpeicherZeile = (((cinfo.width * cinfo.bitCnt - 1) >> 5 ) << 2) + 4;
         break;
      default:
         SpeicherZeile = ((((cinfo.width * cinfo.bitCnt - 1) >> 5 ) << 2) + 4) * 8 / cinfo.bitCnt;
   }

   if (!BmpReadImage(fd, cinfo.width, cinfo.height, ColorMap,
                     cinfo.clrUsed, cinfo.bitCnt, cinfo.compr,
                     SpeicherZeile, func))
   {
      fclose(fd);
      return FALSE;
   }

   fclose(fd);
   return TRUE;
}

gboolean BmpReadColorMap   (FILE *fd,
             RGB2    buffer[256],
             gint    number,
             gint    size)
{
   gint   i;
   guchar rgb[4];

   for (i = 0; i < number ; i++)
   {
      if (!ReadOK (fd, rgb, size))
      {
         return FALSE;
      }
      buffer[i].blue = rgb[0];
      buffer[i].green= rgb[1];
      buffer[i].red  = rgb[2];
   }

   return TRUE;
}

gboolean BmpReadImage   (FILE *fd,
             gint        width,
             gint        height,
             RGB2        cmap[256],
             gint        ncols,
             gint        bpp,
             gint        compression,
             gint        spzeile,
             BmpLoadFunc func)
{
   guchar   v, wieviel, buf[16], *dest, *temp;
   gint     xpos=0, ypos=0, i, j, pix, egal;

   ypos = height-1; /* Bitmaps begin in the lower left corner */

   dest = g_malloc(sizeof(guchar) * width * 3 * bpp);

   switch (bpp)
   {
      case 24:
         if (compression)     /* 24-bit RLE */
         {
            while (TRUE && (ypos >= 0))
            {
               egal = ReadOK(fd, buf, 2);

               /* Count + Color - record */
               if ((guchar) buf[0] != 0)
               {
                  egal = ReadOK(fd, &buf[2], 2);
                  for (j=0;((guchar) j < (guchar) buf[0]) && (xpos<width);)
                  {
                     temp      = dest + (xpos * 3);
                     *(temp++) = buf[3];
                     *(temp++) = buf[2];
                     *temp     = buf[1];
                     xpos ++;
                     j    ++;
                  }
               }

               /* uncompressed record */
               if (((guchar) buf[0] == 0) && ((guchar) buf[1] > 2))
               {
                  for (j=0;((guchar) j < (guchar) buf[1]) && (xpos<width);)
                  {
                     egal = ReadOK(fd, &buf[2], 3);
                     temp      = dest + (xpos * 3);
                     *(temp++) = buf[4];
                     *(temp++) = buf[3];
                     *temp     = buf[2];
                     xpos++;
                     j ++;
                  }
                  if ((guchar) buf[1] & 1)
                  {
                     egal = ReadOK(fd, &v, 1);
                  }
               }

               /* Line end */
               if (((guchar) buf[0]==0) && ((guchar) buf[1]==0))
               {
                  if ((*func) (dest, width, 0, ypos, 3, -1, 0)) break;
                  ypos--;
                  xpos=0;
               }

               /* Deltarecord */
               if (((guchar) buf[0]==0) && ((guchar) buf[1]==2))
               {
                  egal = ReadOK(fd, &buf, 2);
                  xpos += (guchar) buf[0];
                  ypos += (guchar) buf[1];
               }

               /* Bitmap end */
               if (((guchar) buf[0]==0) && ((guchar) buf[1]==1))
               {
                  break;
               }
            }
         } else   /* uncompressed image */
         {
            while (ReadOK(fd, buf, 3) && (ypos >= 0))
            {
               temp = dest + (xpos++ * 3);

               *(temp++) = buf[2];
               *(temp++) = buf[1];
               *temp     = buf[0];

               if (xpos == width)
               {
                  if (spzeile > width*3)
                  {
                     fseek(fd, spzeile - (width*3), SEEK_CUR);
                  }
                  if ((*func) (dest, width, 0, ypos, 3, -1, 0)) break;
                  ypos--;
                  xpos=0;
               }
            }
         }
         break;

      case 16:
         while (ReadOK(fd, buf, 2) && (ypos >= 0))
         {
            temp = dest + (xpos++ * 3);

            *(temp++) = (buf[1]<<1) & 0xf8;
            *(temp++) = ((buf[1]>>5)& 0x04) |
                        ((buf[1]<<6)&0xc0)  |
                        ((buf[0]>>2)&0x38);
            *temp     = (buf[0]<<3) & 0xf8;

            if (xpos == width)
            {
               if ((*func) (dest, width, 0, ypos, 3, -1, 0))
                  break;
               ypos--;
               xpos=0;
            }
         }
         break;

      case 8:
      case 4:
      case 1:
         switch(compression)
         {
            case 0:     /* uncompressed image */
               while (ReadOK(fd, &v, 1) && (ypos >= 0))
               {
                  for (i=1; (i<=(8/bpp)) && (xpos<width); i++,xpos++)
                  {
                     temp = dest + (xpos * 3);
                     /* look at my bitmask !! */
                     pix = (v & (((1<<bpp)-1) << (8-(i*bpp)))) >> (8-(i*bpp));
                     *(temp++)= cmap[pix].red  ;
                     *(temp++)= cmap[pix].green;
                     *temp    = cmap[pix].blue ;
                  }
                  if (xpos == width)
                  {
                     if (spzeile>width)
                     {
                        fseek(fd, (spzeile-width)/(8/bpp), SEEK_CUR);
                     }
                     if ((*func) (dest, width, 0, ypos, 3, -1, 0))
                        break;

                     ypos--;
                     xpos=0;
                  }
               }
               break;

            default: /* compressed image with 4-bit or 8-bit RLE */
               while (TRUE)
               {
                  egal = ReadOK(fd, buf, 2);

                  /* Count + Color - record */
                  if ((guchar) buf[0]!=0)
                  {
                     for (j=0;((guchar) j < (guchar) buf[0]) && (xpos<width);)
                     {
                        for (i=1; (   (i<=(8/bpp))
                                   && (xpos<width)
                                   && ((guchar) j < (guchar) buf[0]))
                                ; i++, xpos++, j++)
                        {
                           temp      = dest + (xpos * 3);
                           pix       = (buf[1] & (((1<<bpp)-1) << (8-(i*bpp)))) >> (8-(i*bpp));
                           *(temp++) = cmap[pix].red  ;
                           *(temp++) = cmap[pix].green;
                           *temp     = cmap[pix].blue ;
                        }
                     }
                  }

                  /* uncompressed record */
                  if (((guchar) buf[0]==0) && ((guchar) buf[1]>2))
                  {
                     wieviel=buf[1];
                     for (j=0; j<wieviel; j+=(8/bpp))
                     {
                        egal = ReadOK(fd, &v, 1);
                        i=1;
                        while ((i<=(8/bpp)) && (xpos<width))
                        {
                           temp = dest + (xpos * 3);
                           pix = (v & ( ((1<<bpp)-1) << (8-(i*bpp)) ) ) >> (8-(i*bpp));
                           *(temp++) = cmap[pix].red  ;
                           *(temp++) = cmap[pix].green;
                           *temp     = cmap[pix].blue ;
                           i++;
                           xpos++;
                        }
                     }

                     if ((wieviel % 2) && (bpp==4))
                        wieviel++;

                     if ((wieviel / (8/bpp)) % 2)
                        egal = ReadOK(fd, &v, 1);
                  }

                  /* Bitmap end */
                  if (((guchar) buf[0]==0) && ((guchar) buf[1]==1))
                  {
                     break;
                  }

                  /* Deltarecord */
                  if (((guchar) buf[0]==0) && ((guchar) buf[1]==2))
                  {
                     egal = ReadOK(fd, &buf, 2);
                     xpos += (guchar) buf[0];
                     ypos += (guchar) buf[1];
                  }

                  /* Line end */
                  if (((guchar) buf[0]==0) && ((guchar) buf[1]==0))
                  {
                     if ((*func) (dest, width, 0, ypos, 3, -1, 0)) break;
                     ypos--;
                     xpos=0;
                  }
               }
               break;
         }
         break;

      default:
         break;
   }

   g_free(dest);
   return TRUE;
}
