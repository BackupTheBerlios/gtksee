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

#ifndef __GTYPES_H__
#define __GTYPES_H__

#define THUMBNAIL_WIDTH 80
#define THUMBNAIL_HEIGHT 60
#define SLIDESHOW_DELAY 4000

#define min(a, b) (((a)<(b))?(a):(b))
#define max(a, b) (((a)>(b))?(a):(b))

#ifdef __cplusplus
        extern "C" {
#endif /* __cplusplus */

#include <time.h>
#include <gtk/gtk.h>

typedef enum
{
   JPG,
   PNG,
   GIF,
   XCF,
   XPM,
   BMP,
   ICO,
   PCX,
   TIF,
   PNM,
   PSD,
   XBM,
   TGA,
   XWD,
   SUN,
   EPS,   
   SGI,
   UNKNOWN,
   MAX_IMAGE_TYPES
} ImageType;

typedef enum
{
   NORMAL_MODE       = 0,
   DISSOLVE_MODE     = 1,
   MULTIPLY_MODE     = 3,
   SCREEN_MODE       = 4,
   OVERLAY_MODE      = 5,
   DIFFERENCE_MODE   = 6,
   ADDITION_MODE     = 7,
   SUBTRACT_MODE     = 8,
   DARKEN_ONLY_MODE  = 9,
   LIGHTEN_ONLY_MODE = 10,
   HUE_MODE          = 11,
   SATURATION_MODE   = 12,
   COLOR_MODE        = 13,
   VALUE_MODE        = 14
} LayerMode;

typedef enum
{
   IMAGE_LIST_DETAILS      = 0,
   IMAGE_LIST_THUMBNAILS   = 1,
   IMAGE_LIST_SMALL_ICONS  = 2
} ImageListType;

typedef enum
{
   IMAGE_SORT_ASCEND_BY_NAME        = 0,
   IMAGE_SORT_DESCEND_BY_NAME       = 1,
   IMAGE_SORT_ASCEND_BY_SIZE        = 2,
   IMAGE_SORT_DESCEND_BY_SIZE       = 3,
   IMAGE_SORT_ASCEND_BY_PROPERTY    = 4,
   IMAGE_SORT_DESCEND_BY_PROPERTY   = 5,
   IMAGE_SORT_ASCEND_BY_DATE        = 6,
   IMAGE_SORT_DESCEND_BY_DATE       = 7,
   IMAGE_SORT_ASCEND_BY_TYPE        = 8,
   IMAGE_SORT_DESCEND_BY_TYPE       = 9,
} ImageSortType;

typedef struct _ImageCache ImageCache;
struct _ImageCache
{
   guchar *buffer;
   guint16 buffer_width;
   guint16 buffer_height;
};

typedef struct _ImageInfo ImageInfo;
struct _ImageInfo
{
   ImageType   type;
   gint        serial;
   gchar       name[256];
   glong       size;
   time_t      time;
   gint        width;
   gint        height;
   gchar       *desc;
   gboolean    has_desc;
   GdkPixmap   *type_pixmap;
   GdkBitmap   *type_mask;

   glong ncolors;
   /*
    * if TRUE, the ncolors field should be a REAL color numbers(xpm, gif)
    * otherwise, the ncolors field is numbers of bits(i.e, 8 is 256-color)
    */
   gboolean real_ncolors;

   /*
    * Is there a alpha channel?
    */
   gint alpha;

   /*
    * bytes per pixel when loaded into memory
    * NOT for all image format
    */
   gint bpp;

   /*
    * Is the image valid? we can know before or after loading it,
    * and cache it.
    */
   gboolean valid;

   /* whether the image has been loaded? */
   gboolean loaded;

   /* this field is for image cache in imagelist*/
   ImageCache cache;
};

#ifdef __cplusplus
        }
#endif /* __cplusplus */

#endif
