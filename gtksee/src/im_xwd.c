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

/* 2003-11-07: This library takes codes from
 *             XWD plug-in for the GIMP
 */

#include <stdio.h>
#include <string.h>
#include "common_tools.h"
#include "im_xwd.h"

/* Declare some local functions. */

gboolean load_xwd_f2_d24_b32(FILE *, xwd_info *, xwd_color *, XwdLoadFunc);
gboolean load_xwd_f2_d8_b8  (FILE *, xwd_info *, xwd_color *, XwdLoadFunc);
gboolean load_xwd_f2_d1_b1  (FILE *, xwd_info *, xwd_color *, XwdLoadFunc);
gchar    *load_xwd_f1_d24_b1(FILE *, xwd_info *, xwd_color *);

gint set_pixelmap      (gint, xwd_color *,pixel_map *);
gint get_pixelmap      (gulong, pixel_map *,guchar *, guchar *, guchar *);

/* Begin the code ... */

gboolean
xwd_get_header(gchar *filename, xwd_info *info)
{
   FILE     *zf;
   guchar   buf[sizeof(xwd_info)];

   if ((zf = fopen (filename, "rb")) == NULL)
   {
      return FALSE;
   }

   if (fread(&buf[0], sizeof(xwd_info), 1, zf) != 1)
   {
      fclose(zf);
      return FALSE;
   }

   fclose(zf);

   info->header_size = buf2long(buf, 0);

   if ((info->file_version = buf2long(buf, 4)) != 7)
   {
      return FALSE;
   }

   info->pixmap_format     = buf2long(buf, 8);
   info->pixmap_depth      = buf2long(buf, 12);
   info->pixmap_width      = buf2long(buf, 16);
   info->pixmap_height     = buf2long(buf, 20);
   info->xoffset           = buf2long(buf, 24);
   info->byte_order        = buf2long(buf, 28);

   info->bitmap_unit       = buf2long(buf, 32);
   info->bitmap_bit_order  = buf2long(buf, 36);
   info->bitmap_pad        = buf2long(buf, 40);
   info->bits_per_pixel    = buf2long(buf, 44);

   info->bytes_per_line    = buf2long(buf, 48);
   info->visual_class      = buf2long(buf, 52);
   info->red_mask          = buf2long(buf, 56);
   info->green_mask        = buf2long(buf, 60);
   info->blue_mask         = buf2long(buf, 64);
   info->bits_per_rgb      = buf2long(buf, 68);
   info->ncolors           = buf2long(buf, 72);

   if ((info->colormap_entries = buf2long(buf, 76)) > 256)
   {
      return FALSE;
   }

   info->window_width      = buf2long(buf, 80);
   info->window_height     = buf2long(buf, 84);
   info->window_x          = buf2long(buf, 88);
   info->window_y          = buf2long(buf, 92);
   info->window_bdrwidth   = buf2long(buf, 96);

   return TRUE;
}

gboolean
xwd_load (gchar *filename, XwdLoadFunc func)
{
   FILE        *ifp;
   xwd_info    hdr;
   gint        y;
   xwd_color   *xwdcolmap = NULL;
   guchar      *data, *row;
   gboolean    result;

   if (xwd_get_header(filename, &hdr) == FALSE)
   {
      return FALSE;
   }

   if ((ifp = fopen (filename, "rb")) == NULL)
   {
      return FALSE;
   }

   if (hdr.file_version != 7)
   {
      fclose (ifp);
      return FALSE;
   }

   result = FALSE;

   /* Position to start of XWDColor structures */
   fseek (ifp, (long)hdr.header_size, SEEK_SET);

   if (hdr.colormap_entries > 0)
   {
      xwdcolmap = g_malloc (sizeof (xwd_color) * hdr.colormap_entries);
      if (xwdcolmap == NULL)
      {
         goto data_short;
      }

      for (y=0; y < hdr.colormap_entries; y++)
      {
         guchar   buf[8];

         if (fread(&buf[0], 4, 1, ifp) != 1)
         {
            goto data_short;
         }

         xwdcolmap[y].pixel  = buf2long(buf, 0);

         if (fread(&buf[0], 8, 1, ifp) != 1)
         {
            goto data_short;
         }
         xwdcolmap[y].red   = (gushort) buf2short(buf, 0);
         xwdcolmap[y].green = (gushort) buf2short(buf, 2);
         xwdcolmap[y].blue  = (gushort) buf2short(buf, 4);

         xwdcolmap[y].flags = (guchar) buf[6];
         xwdcolmap[y].pad   = (guchar) buf[7];
      }
   }

   switch (hdr.pixmap_format)
   {
      case 0:    /* Single plane bitmap */
         if ((hdr.pixmap_depth == 1) && (hdr.bits_per_pixel == 1))
         { /* Can be performed by format 2 loader */
            result = load_xwd_f2_d1_b1 (ifp, &hdr, xwdcolmap, func);
         }
         break;

      case 1:    /* Single plane pixmap */
         if ((hdr.pixmap_depth <= 24) && (hdr.bits_per_pixel == 1))
         {
            data = load_xwd_f1_d24_b1 (ifp, &hdr, xwdcolmap);
            if (data == NULL)
            {
               break;
            }

            for (y=0; y<hdr.pixmap_height; y++)
            {
               row = data + (y * hdr.pixmap_width * 3);
               if ((*func) (row, hdr.pixmap_width, 0, y, 3, -1, 0)) break;
            }
            g_free(data);
         }
         break;

      case 2:    /* Multiplane pixmaps */
         if ((hdr.pixmap_depth == 1) && (hdr.bits_per_pixel == 1))
         {
            result = load_xwd_f2_d1_b1 (ifp, &hdr, xwdcolmap, func);
            break;
         }
         else if ((hdr.pixmap_depth <= 8) && (hdr.bits_per_pixel == 8))
         {
            result = load_xwd_f2_d8_b8 (ifp, &hdr, xwdcolmap, func);
            break;
         }
         else if ((hdr.pixmap_depth <= 16) && (hdr.bits_per_pixel == 16))
         {
           /* Code not ready yet, because I can't get
            * any image with this format :(
            */
	    break;    
         }
         else if (   (hdr.pixmap_depth <= 24)
                  && ((hdr.bits_per_pixel == 24) || (hdr.bits_per_pixel == 32)))
         {
            result = load_xwd_f2_d24_b32 (ifp, &hdr, xwdcolmap, func);
         }
         break;
   }

data_short:
   fclose (ifp);
   if (xwdcolmap != NULL)
   {
      g_free ((char *)xwdcolmap);
   }

   return result;
}

/* Create a map for mapping up to 32 bit pixelvalues to RGB.
 * Returns number of colors kept in the map (up to 256)
 */
gint
set_pixelmap (int ncols, xwd_color *xwdcol, pixel_map *pixelmap)
{
   gint     i, j, k, maxcols;
   gulong   pixel_val;

   memset ((char *)pixelmap, 0, sizeof (pixel_map));
   maxcols = 0;

   for (j = 0; j < ncols; j++)      /* For each entry of the XWD colormap */
   {
      pixel_val = xwdcol[j].pixel;

      for (k = 0; k < maxcols; k++) /* Where to insert in list ? */
      {
         if (pixel_val <= pixelmap->pmap[k].pixel_val)
            break;
      }

      /* It was already in list ? */
      if (   ((k < maxcols) && (pixel_val == pixelmap->pmap[k].pixel_val))
          || (k >= 256))
         break;

      if (k < maxcols)   /* Must move entries to the back ? */
      {
         for (i = maxcols-1; i >= k; i--)
            memcpy ((char *)&(pixelmap->pmap[i+1]),(char *)&(pixelmap->pmap[i]), sizeof (p_map));
      }
      pixelmap->pmap[k].pixel_val   = pixel_val;
      pixelmap->pmap[k].red         = xwdcol[j].red   >> 8;
      pixelmap->pmap[k].green       = xwdcol[j].green >> 8;
      pixelmap->pmap[k].blue        = xwdcol[j].blue  >> 8;

      pixelmap->pixel_in_map[pixel_val & MAPPERMASK] = 1;
      maxcols++;
   }
   pixelmap->npixel = maxcols;

   return (pixelmap->npixel);
}

/* Search a pixel value in the pixel map.
 * Returns 0 if the pixelval was not found in map.
 * Returns 1 if found.
 */

gint
get_pixelmap (gulong pixelval, pixel_map *pixelmap,
               guchar *red, guchar *green, guchar *blue)
{
   p_map    *low, *high, *middle;

   if (pixelmap->npixel == 0) return (0);

   if (!(pixelmap->pixel_in_map[pixelval & MAPPERMASK])) return (0);

   low   = &(pixelmap->pmap[0]);
   high  = &(pixelmap->pmap[pixelmap->npixel-1]);

   /* Do a binary search on the array */
   while (low < high)
   {
      middle = low + ((high - low)/2);
      if (pixelval <= middle->pixel_val)
      {
         high = middle;
      } else
      {
         low = middle+1;
      }
   }

   if (pixelval == low->pixel_val)
   {
      *red     = low->red;
      *green   = low->green;
      *blue    = low->blue;
      return (1);
   }

   return (0);
}

/* Load XWD with pixmap_format 2, pixmap_depth up to 24, bits_per_pixel 24/32 */
gboolean
load_xwd_f2_d24_b32 (FILE *ifp, xwd_info *hdr, xwd_color *xwdcolmap, XwdLoadFunc func)
{
   guchar    *dest, lsbyte_first;
   //gint      width, height,
   gint      linepad, i, j, c0, c1, c2, c3;
   gint      scan_lines;
   gulong    pixelval;
   gint      red, green, blue, ncols;
   gint      maxred, maxgreen, maxblue;
   gulong    redmask, greenmask, bluemask;
   guint     redshift, greenshift, blueshift;
   guchar    redmap[256],greenmap[256],bluemap[256];
   guchar    *data;
   pixel_map pixelmap;
   gint      err = 0;

   redmask  = (hdr->red_mask  == 0) ? 0xff0000 : hdr->red_mask;
   greenmask= (hdr->green_mask== 0) ? 0xff0000 : hdr->green_mask;
   bluemask = (hdr->blue_mask == 0) ? 0xff0000 : hdr->blue_mask;

   /* How to shift RGB to be right aligned ?
    * (We rely on the the mask bits are grouped and not mixed)
    */
   redshift  =
   greenshift=
   blueshift = 0;

   while (((1 << redshift  ) & redmask  ) == 0) redshift++;
   while (((1 << greenshift) & greenmask) == 0) greenshift++;
   while (((1 << blueshift ) & bluemask ) == 0) blueshift++;

   /* The bits_per_rgb may not be correct. Use redmask instead */
   maxred = 0;
   while (redmask >> (redshift + maxred)) maxred++;
   maxred = (1 << maxred) - 1;

   maxgreen = 0;
   while (greenmask >> (greenshift + maxgreen)) maxgreen++;
   maxgreen = (1 << maxgreen) - 1;

   maxblue = 0;
   while (bluemask >> (blueshift + maxblue)) maxblue++;
   maxblue = (1 << maxblue) - 1;

   /* Set map-arrays for red, green, blue */
   for (red = 0; red <= maxred; red++)
      redmap[red] = (red * 255) / maxred;
   for (green = 0; green <= maxgreen; green++)
      greenmap[green] = (green * 255) / maxgreen;
   for (blue = 0; blue <= maxblue; blue++)
      bluemap[blue] = (blue * 255) / maxblue;

   ncols = (hdr->ncolors < hdr->colormap_entries) ? hdr->ncolors : hdr->colormap_entries;

   ncols = set_pixelmap (ncols, xwdcolmap, &pixelmap) + 1;

   /* What do we have to consume after a line has finished ? */
   linepad = hdr->bytes_per_line -
               ((hdr->pixmap_width * hdr->bits_per_pixel) >> 3);
   if (linepad < 0)
   {
      linepad = 0;
   }

   lsbyte_first = (hdr->byte_order == 0);

   if (hdr->bits_per_pixel == 32)
   {
      for (i = 0; i < hdr->pixmap_height; i++)
      {
         data = g_malloc (hdr->pixmap_width * 3);
         dest = data;

         for (j = 0; j < hdr->pixmap_width; j++)
         {
            c0 = getc (ifp);
            c1 = getc (ifp);
            c2 = getc (ifp);

            if ((c3 = getc (ifp)) < 0)
            {
               err = 1;
               break;
            }
            if (lsbyte_first)
               pixelval = c0 | (c1 << 8) | (c2 << 16) | (c3 << 24);
            else
               pixelval = (c0 << 24) | (c1 << 16) | (c2 << 8) | c3;

            if (get_pixelmap (pixelval, &pixelmap, dest, dest+1, dest+2))
            {
               dest += 3;
            } else
            {
               *dest++ = redmap[(pixelval & redmask) >> redshift];
               *dest++ = greenmap[(pixelval & greenmask) >> greenshift];
               *dest++ = bluemap[(pixelval & bluemask) >> blueshift];
            }
         }
         scan_lines++;

         if (err) break;
         if ((*func) (data, hdr->pixmap_width, 0, i, 3, -1, 0)) break;

         for (j = 0; j < linepad; j++)
            getc (ifp);

      }
   } else    /* 24 bits per pixel */
   {
      for (i = 0; i < hdr->pixmap_height; i++)
      {
         data = g_malloc (hdr->pixmap_width * 3);
         dest = data;

         for (j = 0; j < hdr->pixmap_width; j++)
         {
            c0 = fgetc (ifp);
            c1 = fgetc (ifp);
            if ((c2 = getc (ifp)) < 0)
            {
               g_free(data);
               err = 1;
               break;
            }

            if (lsbyte_first)
               pixelval = c0 | (c1 << 8) | (c2 << 16);
            else
               pixelval = (c0 << 16) | (c1 << 8) | c2;

            if (get_pixelmap (pixelval, &pixelmap, dest, dest+1, dest+2))
            {
               dest += 3;
            } else
            {
               *dest++ = redmap[(pixelval & redmask) >> redshift];
               *dest++ = greenmap[(pixelval & greenmask) >> greenshift];
               *dest++ = bluemap[(pixelval & bluemask) >> blueshift];
            }
         }

         if (err) break;
         if ((*func) (data, hdr->pixmap_width, 0, i, 3, -1, 0)) break;

         for (j = 0; j < linepad; j++)
         {
            fgetc (ifp);
         }
      }
   }

   g_free(data);

   return (err ? FALSE : TRUE);
}

/* Load XWD with pixmap_format 2, pixmap_depth 8, bits_per_pixel 8 */
gboolean
load_xwd_f2_d8_b8 (FILE *ifp,
                     xwd_info *hdr, xwd_color *xwdcolmap,
                     XwdLoadFunc func)
{
   gint     linepad;
   gint     i, j, ncols;
   gint     greyscale;
   guchar   *dest, *data;
   gint     err = 0;
   gint     valor, z;

  /* This could also be a greyscale image. Check it */
   greyscale = 0;

   if ((hdr->ncolors == 256) && (hdr->colormap_entries == 256))
   {
      for (j = 0; j < 256; j++)
      {
         if (   (xwdcolmap[j].pixel       != j)
             || ((xwdcolmap[j].red   >> 8)!= j)
             || ((xwdcolmap[j].green >> 8)!= j)
             || ((xwdcolmap[j].blue  >> 8)!= j))
       break;
      }
      greyscale = (j == 256);
   }

   data = g_malloc (hdr->pixmap_width * 3);

   ncols = hdr->colormap_entries;

   if (!greyscale && (hdr->ncolors < ncols))
   {
      ncols = hdr->ncolors;
   }

   linepad = hdr->bytes_per_line - hdr->pixmap_width;
   if (linepad < 0) linepad = 0;

   for (i = 0; i < hdr->pixmap_height; i++)
   {
      dest  = data;
      z     = 0;

      for (j=0; j < hdr->pixmap_width; j++)
      {
         valor = fgetc(ifp);
         if (xwdcolmap == NULL)
         {
            dest[z++] = valor;
            dest[z++] = valor;
            dest[z++] = valor;
         } else
         {
            dest[z++] = xwdcolmap[valor].red   >> 8;
            dest[z++] = xwdcolmap[valor].green >> 8;
            dest[z++] = xwdcolmap[valor].blue  >> 8;
         }
      }

      for (j = 0; j < linepad; j++)
      {
         fgetc (ifp);
      }

      if (err) break;
      if ((*func) (data, hdr->pixmap_width, 0, i, 3, -1, 0)) break;
   }

   g_free(data);
   return (err ? FALSE : TRUE);
}

/* Load XWD with pixmap_format 2, pixmap_depth 1, bits_per_pixel 1 */
gboolean
load_xwd_f2_d1_b1 (FILE *ifp, xwd_info *hdr, xwd_color *xwdcolmap, XwdLoadFunc func)
{
   gint     i, j, k;
   gint     pix8;
   guchar   *scanline, *src;
   guchar   *data, *dest;
   gint     err = 0;

   data     = g_malloc (hdr->pixmap_height * hdr->pixmap_width);

   scanline = (guchar *)g_malloc (hdr->bytes_per_line + 8);
   if (scanline == NULL)
      return FALSE;

   for (i = 0; i < hdr->pixmap_height; i++)
   {
      dest = data;

      if (fread (scanline, hdr->bytes_per_line, 1, ifp) != 1)
      {
         err = 1;
         break;
      }

      /* Need to check byte order ? */
      if (hdr->bitmap_bit_order != hdr->byte_order)
      {
         guchar   buf[4];

         src = scanline;
         switch (hdr->bitmap_unit)
         {
            case 16:
               j = hdr->bytes_per_line;
               while (j > 0)
               {
                  buf[0] = src[0];
                  buf[1] = src[1];
                  *(src++) = buf[1];
                  *(src++) = buf[0];
                  j -= 2;
               }
               break;

            case 32:
               j = hdr->bytes_per_line;
               while (j > 0)
               {
                  buf[0] = src[0];
                  buf[1] = src[1];
                  buf[2] = src[2];
                  buf[3] = src[3];
                  *(src++) = buf[3];
                  *(src++) = buf[2];
                  *(src++) = buf[1];
                  *(src++) = buf[0];
                  j -= 4;
               }
               break;
         }
      }

      src   = scanline;
      j     = hdr->pixmap_width;

      while (j >= 8)
      {
         pix8   = *(src++);

         if (!hdr->bitmap_bit_order)
         {
            for (k = 0; k < 8; k++)
               *(dest++) = ((pix8 & (1 << k)) != 0) ? 0xff : 0x0;
         } else
         {
            for (k = 7; k >= 0; k--)
               *(dest++) = ((pix8 & (1 << k)) != 0) ? 0xff : 0x0;
         }
         j     -= 8;
      }

      if (j > 0)
      {
         pix8 = *(src++);

         if (!hdr->bitmap_bit_order)
         {
            for (k = 0; k < j; k++)
               *(dest++) = ((pix8 & (1 << k)) != 0) ? 0xff : 0x0;
         } else
         {
            for (k = 7; k > (7 -j); k--)
               *(dest++) = ((pix8 & (1 << k)) != 0) ? 0xff : 0x0;
         }
      }

      if (err) break;
      if ((*func) (data, hdr->pixmap_width, 0, i, 1, -1, 0)) break;
   }

   g_free (scanline);
   g_free (data);

   return (err ? FALSE : TRUE);
}

/* Load XWD with pixmap_format 1, pixmap_depth up to 24, bits_per_pixel 1 */
gchar *
load_xwd_f1_d24_b1 (FILE *ifp, xwd_info *hdr, xwd_color *xwdcolmap)
{
   guchar      *dest, outmask, inmask, do_reverse;
   gint        width, height, linepad, i, j, plane, fromright;
   gint        tile_start, tile_end;
   gint        indexed, bytes_per_pixel;
   gint        maxred, maxgreen, maxblue;
   gint        red, green, blue, ncols, standard_rgb;
   glong       data_offset, plane_offset, tile_offset;
   gulong      redmask, greenmask, bluemask;
   guint       redshift, greenshift, blueshift;
   gulong      g;
   guchar      redmap[256], greenmap[256], bluemap[256];
   guchar      bit_reverse[256];
   guchar      *xwddata, *xwdin, *data;
   gulong      pixelval;
   pixel_map   pixel_map;
   gint        err = 0;

   xwddata = g_malloc (hdr->bytes_per_line);
   if (xwddata == NULL)
      return FALSE;

   width          = hdr->pixmap_width;
   height         = hdr->pixmap_height;
   indexed        = (hdr->pixmap_depth <= 8);
   bytes_per_pixel= (indexed ? 1 : 3);

   data = g_malloc (height * width * bytes_per_pixel);

   linepad =   hdr->bytes_per_line - (hdr->pixmap_width+7)/8;
   if (linepad < 0) linepad = 0;

   for (j = 0; j < 256; j++)   /* Create an array for reversing bits */
   {
      inmask = 0;
      for (i = 0; i < 8; i++)
      {
         inmask <<= 1;
         if (j & (1 << i)) inmask |= 1;
      }
      bit_reverse[j] = inmask;
   }

   redmask   = hdr->red_mask;
   greenmask = hdr->green_mask;
   bluemask  = hdr->blue_mask;

   if (redmask  == 0) redmask  = 0xff0000;
   if (greenmask== 0) greenmask= 0x00ff00;
   if (bluemask == 0) bluemask = 0x0000ff;

   standard_rgb =    (redmask  == 0xff0000)
                  && (greenmask== 0x00ff00)
                  && (bluemask == 0x0000ff);
   redshift = greenshift = blueshift = 0;

   if (!standard_rgb)   /* Do we need to re-map the pixel-values ? */
   {
      /* How to shift RGB to be right aligned ? */
      /* (We rely on the the mask bits are grouped and not mixed) */

      while (((1 << redshift)  & redmask  ) == 0) redshift++;
      while (((1 << greenshift)& greenmask) == 0) greenshift++;
      while (((1 << blueshift) & bluemask ) == 0) blueshift++;

      /* The bits_per_rgb may not be correct. Use redmask instead */

      maxred = 0;
      while (redmask >> (redshift + maxred)) maxred++;
      maxred = (1 << maxred) - 1;

      maxgreen = 0;
      while (greenmask >> (greenshift + maxgreen)) maxgreen++;
      maxgreen = (1 << maxgreen) - 1;

      maxblue = 0;
      while (bluemask >> (blueshift + maxblue)) maxblue++;
      maxblue = (1 << maxblue) - 1;

      /* Set map-arrays for red, green, blue */
      for (red = 0; red <= maxred; red++)
         redmap[red] = (red * 255) / maxred;
      for (green = 0; green <= maxgreen; green++)
         greenmap[green] = (green * 255) / maxgreen;
      for (blue = 0; blue <= maxblue; blue++)
         bluemap[blue] = (blue * 255) / maxblue;
   }

   ncols = hdr->colormap_entries;
   if (hdr->ncolors < ncols) ncols = hdr->ncolors;

   if (!indexed)
   {
     ncols = set_pixelmap (ncols, xwdcolmap, &pixel_map);
   }

   do_reverse = !hdr->bitmap_bit_order;

   /* This is where the image data starts within the file */
   data_offset = ftell (ifp);

   for (tile_start = 0; tile_start < height; tile_start += height)
   {
      memset (data, 0, width * height * bytes_per_pixel);

      tile_end = tile_start + height - 1;
      if (tile_end >= height) tile_end = height - 1;

      for (plane = 0; plane < hdr->pixmap_depth; plane++)
      {
         dest = data;    /* Position to start of tile within the plane */
         plane_offset = data_offset + plane*height*hdr->bytes_per_line;
         tile_offset = plane_offset + tile_start*hdr->bytes_per_line;
         fseek (ifp, tile_offset, SEEK_SET);

         /* Place the last plane at the least significant bit */

         if (indexed)   /* Only 1 byte per pixel */
         {
            fromright = hdr->pixmap_depth-1-plane;
            outmask = (1 << fromright);
         } else           /* 3 bytes per pixel */
         {
            fromright = hdr->pixmap_depth-1-plane;
            dest += 2 - fromright/8;
            outmask = (1 << (fromright % 8));
         }

         for (i = tile_start; i <= tile_end; i++)
         {
            if (fread (xwddata,hdr->bytes_per_line,1,ifp) != 1)
            {
               err = 1;
               break;
            }
            xwdin = xwddata;

            /* Handle bitmap unit */
            if (hdr->bitmap_unit == 16)
            {
               if (hdr->bitmap_bit_order != hdr->byte_order)
               {
                  j = hdr->bytes_per_line/2;
                  while (j--)
                  {
                     inmask = xwdin[0]; xwdin[0] = xwdin[1]; xwdin[1] = inmask;
                     xwdin += 2;
                  }
                  xwdin = xwddata;
               }
            } else if (hdr->bitmap_unit == 32)
            {
               if (hdr->bitmap_bit_order != hdr->byte_order)
               {
                  j = hdr->bytes_per_line/4;
                  while (j--)
                  {
                     inmask = xwdin[0]; xwdin[0] = xwdin[3]; xwdin[3] = inmask;
                     inmask = xwdin[1]; xwdin[1] = xwdin[2]; xwdin[2] = inmask;
                     xwdin += 4;
                  }
                  xwdin = xwddata;
               }
            }

            g = inmask = 0;
            for (j = 0; j < width; j++)
            {
               if (!inmask)
               {
                  g = *(xwdin++);
                  if (do_reverse)
                     g = bit_reverse[g];
                  inmask = 0x80;
               }

               if (g & inmask)
                  *dest |= outmask;
               dest += bytes_per_pixel;

               inmask >>= 1;
            }
         }
      }

      /* For indexed images, the mapping to colors is done
       * by the color table. Otherwise we must do the mapping
       * by ourself.
       */
      if (!indexed)
      {
         dest = data;
         for (i = tile_start; i <= tile_end; i++)
         {
            for (j = 0; j < width; j++)
            {
               pixelval = (*dest << 16) | (*(dest+1) << 8) | *(dest+2);
               if (   get_pixelmap (pixelval, &pixel_map, dest, dest+1, dest+2)
                   || standard_rgb)
               {
                  dest += 3;
               } else   /* We have to map RGB to 0,...,255 */
               {
                  *(dest++) = redmap[(pixelval & redmask) >> redshift];
                  *(dest++) = greenmap[(pixelval & greenmask) >> greenshift];
                  *(dest++) = bluemap[(pixelval & bluemask) >> blueshift];
               }
            }
         }
      }
   }

   g_free (xwddata);

   return (err ? NULL : data);
}
