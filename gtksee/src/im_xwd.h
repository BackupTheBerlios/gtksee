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

#ifndef __IM_XWD_H__
#define __IM_XWD_H__

#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

#include <gtk/gtk.h>

/* Some structures for mapping up to 32bit-pixel */
/* values which are kept in the XWD-Colormap */

#define MAPPERBITS 12
#define MAPPERMASK ((1 << MAPPERBITS)-1)

/* Values in the file are most significant byte first. */

typedef struct _xwd_file_header {
   gulong header_size;     /* Header size */

   gulong file_version;    /* File version */
   gulong pixmap_format;   /* ZPixmap or XYPixmap. Image type */
   gulong pixmap_depth;    /* Pixmap depth (number of planes) */
   gulong pixmap_width;    /* Image width */
   gulong pixmap_height;   /* Image height */
   gulong xoffset;         /* Bitmap x offset, normally 0 */
   gulong byte_order;      /* of image data: MSBFirst, LSBFirst */

   gulong bitmap_unit;
   gulong bitmap_bit_order;   /* bitmaps only: MSBFirst, LSBFirst */
   gulong bitmap_pad;
   gulong bits_per_pixel;     /* Bits per pixel */

   gulong bytes_per_line;     /* Number of bytes per scanline */
   gulong visual_class;       /* Class of colormap */
   gulong red_mask;           /* Z red mask */
   gulong green_mask;         /* Z green mask */
   gulong blue_mask;          /* Z blue mask */
   gulong bits_per_rgb;       /* Log2 of distinct color values */
   gulong colormap_entries;   /* Number of entries in colormap; not used? */
   gulong ncolors;            /* Number of XWDColor structures */
   gulong window_width;       /* Window width */
   gulong window_height;      /* Window height */
   gulong window_x;           /* Window upper left X coordinate */
   gulong window_y;           /* Window upper left Y coordinate */
   gulong window_bdrwidth;    /* Window border width */
} xwd_info;

/* Null-terminated window name follows the above structure. */

/* Next comes XWDColor structures, at offset XWDFileHeader.header_size in
 * the file.  XWDFileHeader.ncolors tells how many XWDColor structures
 * there are.
 */

typedef struct {
   gulong   pixel;         /* Colour index */
   gushort  red;           /* R  value     */
   gushort  green;         /* G  value     */
   gushort  blue;          /* B  value     */
   guchar   flags;
   guchar   pad;
} xwd_color;

typedef struct {
   gulong   pixel_val;
   guchar   red;
   guchar   green;
   guchar   blue;
} p_map;

typedef struct {
   gint     npixel;                       /* Number of pixel values in map */
   guchar   pixel_in_map[1 << MAPPERBITS];
   p_map    pmap[256];
} pixel_map;


typedef  gboolean (*XwdLoadFunc)    (guchar *buffer, gint width, gint left, gint scanline, gint components, gint pass, gint mode);

gboolean xwd_get_header             (gchar *filename, xwd_info *info);
gboolean xwd_load                   (gchar *filename, XwdLoadFunc func);

#ifdef __cplusplus
   }
#endif /* __cplusplus */

#endif

