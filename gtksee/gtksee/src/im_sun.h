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

#ifndef __IM_SUN_H__
#define __IM_SUN_H__

#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

#include <gtk/gtk.h>

typedef struct _SunRaster
{
   gulong magicnumber;      /* Magic number (0x59a66a95) */
   gulong width;            /* Width of image in pixels */
   gulong height;           /* Height of image in pixels */
   gulong depth;            /* Number of bits per pixel */
   gulong length;           /* Size of image data in bytes */
   gulong type;             /* Type of raster file */
   gulong colormaptype;     /* Type of color map */
   gulong colormaplength;   /* Size of the color map in bytes */
} sun_info;

typedef struct _rle_buf
{
   gint val;   /* The value that is to be repeated */
   gint n;     /* How many times it is repeated */
}  RLEBUF;

typedef  gboolean (*SunLoadFunc)    (guchar *buffer, gint width, gint left, gint scanline, gint components, gint pass, gint mode);

gboolean sun_get_header             (gchar *filename, sun_info *info);
gboolean sun_load                   (gchar *filename, SunLoadFunc func);

#ifdef __cplusplus
   }
#endif /* __cplusplus */

#endif

