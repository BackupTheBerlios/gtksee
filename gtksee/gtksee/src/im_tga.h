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

#ifndef __IM_TGA_H__
#define __IM_TGA_H__

#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

#include <gtk/gtk.h>

/* Header structure definition. */
typedef struct {
   guchar   IDLength;     /* Size of Image Identifier field */
   guchar   ColorMapType; /* Color map type */
   guchar   ImageType;    /* Image type code */
   guint    CMapStart;    /* Color map origin */
   guint    CMapLength;   /* Color map length */
   guchar   CMapDepth;    /* Size of color map entries */
   guint    XOffset;      /* X origin of image */
   guint    YOffset;      /* Y origin of image */
   guint    Width;        /* Width of image */
   guint    Height;       /* Height of image */
   guchar   PixelDepth;   /* Image pixel size (8,16,24,32) */
   guchar   AttBits;      /* 4 bits, number of attribute bits per pixel */
   guchar   Rsrvd;        /* 1 bit, reserved */
   guchar   OrgBit;       /* 1 bit, origin: 0=lower left, 1=upper left */
   guchar   IntrLve;      /* 2 bits, interleaving flag */
   gboolean RLE;          /* Run length encoded */
} tga_info;

typedef  gboolean (*TgaLoadFunc)    (guchar *buffer, gint width, gint left, gint scanline, gint components, gint pass, gint mode);

gboolean tga_get_header             (gchar *filename, tga_info *info);
gboolean tga_load                   (gchar *filename, TgaLoadFunc func);

#ifdef __cplusplus
   }
#endif /* __cplusplus */

#endif

