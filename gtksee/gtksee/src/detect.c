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
 * 2003-09-02: Little changes. Eliminated buffers
 *
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include "gtypes.h"

#ifdef HAVE_LIBJPEG
#include <jpeglib.h>
#include "im_jpeg.h"
#endif

#ifdef HAVE_LIBTIFF
#include <tiffio.h>
#include "im_tiff.h"
#endif

#ifdef HAVE_LIBPNG
#include <png.h>
#include "im_png.h"
#endif

#include "im_gif.h"
#include "im_xpm.h"
#include "im_bmp.h"
#include "im_ico.h"
#include "im_pcx.h"
#include "im_pnm.h"
#include "im_psd.h"
#include "im_xbm.h"
#include "im_xcf.h"
#include "im_tga.h"
#include "im_xwd.h"
#include "im_sun.h"
#include "im_eps.h"
#include "im_sgi.h"

#include "detect.h"

typedef gboolean (*DetectFunc) (guchar *filename, ImageInfo *info);

/* Begin the code ... */

static gboolean
detect_jpeg(guchar *filename, ImageInfo *info)
{
#ifdef HAVE_LIBJPEG
   jpeg_info jinfo;

   if (jpeg_get_header(filename, &jinfo))
   {
      info->type = JPG;
      info->width = jinfo.image_width;
      info->height = jinfo.image_height;
      info->ncolors = jinfo.num_components * 8;
      info->real_ncolors = FALSE;
      info->alpha = 0;
      return TRUE;
   } else
   {
      return FALSE;
   }
#else
   return FALSE;
#endif
}

static gboolean
detect_tiff(guchar *filename, ImageInfo *info)
{
#ifdef HAVE_LIBTIFF
   tiff_info tinfo;

   if (tiff_get_header(filename, &tinfo))
   {
      info->type = TIF;
      info->width = tinfo.width;
      info->height = tinfo.height;
      info->ncolors = tinfo.ncolors;
      info->real_ncolors = FALSE;
      info->alpha = tinfo.alpha;
      return TRUE;
   } else
   {
      return FALSE;
   }
#else
   return FALSE;
#endif
}

static gboolean
detect_png(guchar *filename, ImageInfo *info)
{
#ifdef HAVE_LIBPNG
   png_info pnginfo;

   if (png_get_header(filename, &pnginfo))
   {
      info->type = PNG;
      info->width = pnginfo.width;
      info->height = pnginfo.height;
      switch (pnginfo.color_type)
      {
         case PNG_COLOR_TYPE_RGB :
            info->ncolors = 24;
            info->alpha = 0;
            info->bpp = 3;
            break;
         case PNG_COLOR_TYPE_RGB_ALPHA :
            info->ncolors = 24;
            info->alpha = 1;
            info->bpp = 4;
            break;
         case PNG_COLOR_TYPE_GRAY :
            info->ncolors = 8;
            info->alpha = 0;
            info->bpp = 1;
            break;
         case PNG_COLOR_TYPE_GRAY_ALPHA :
            info->ncolors = 8;
            info->alpha = 1;
            info->bpp = 2;
            break;
         case PNG_COLOR_TYPE_PALETTE :
            info->ncolors = 8;
            info->alpha = 0;
            info->bpp = 3;
            break;
      };
      info->real_ncolors = FALSE;
      return TRUE;
   } else
   {
      return FALSE;
   }
#else
   return FALSE;
#endif
}

static gboolean
detect_gif(guchar *filename, ImageInfo *info)
{
   gif_screen ginfo;

   if (gif_get_header(filename, &ginfo))
   {
      info->type = GIF;
      info->width = ginfo.Width;
      info->height = ginfo.Height;
      info->ncolors = ginfo.BitPixel;
      info->real_ncolors = TRUE;
      info->alpha = 0;
      return TRUE;
   } else
   {
      return FALSE;
   }
}

static gboolean
detect_xpm(guchar *filename, ImageInfo *info)
{
   xpm_info xinfo;

   if (xpm_get_header(filename, &xinfo))
   {
      info->type = XPM;
      info->width = xinfo.width;
      info->height = xinfo.height;
      info->ncolors = xinfo.ncolors;
      info->real_ncolors = TRUE;
      info->alpha = 0;
      return TRUE;
   } else
   {
      return FALSE;
   }
}

static gboolean
detect_bmp(guchar *filename, ImageInfo *info)
{
   bmp_info binfo;

   if (bmp_get_header(filename, &binfo))
   {
      info->type = BMP;
      info->width = binfo.width;
      info->height = binfo.height;
      info->ncolors = binfo.bitCnt;
      info->real_ncolors = FALSE;
      info->alpha = 0;
      return TRUE;
   } else
   {
      return FALSE;
   }
}

/* We don't do a quick detect for ico format. */
static gboolean
detect_ico(guchar *filename, ImageInfo *info)
{
   ico_info icinfo;

   if (ico_get_header(filename, &icinfo))
   {
      info->type = ICO;
      info->width = icinfo.width;
      info->height = icinfo.height;
      info->ncolors = icinfo.ncolors;
      info->real_ncolors = FALSE;
      info->alpha = 0;
      return TRUE;
   } else
   {
      return FALSE;
   }
}

static gboolean
detect_pcx(guchar *filename, ImageInfo *info)
{
   pcx_info pinfo;

   if (pcx_get_header(filename, &pinfo))
   {
      info->type = PCX;
      info->width = pinfo.width;
      info->height = pinfo.height;
      info->ncolors = pinfo.ncolors;
      info->real_ncolors = FALSE;
      info->alpha = 0;
      return TRUE;
   } else
   {
      return FALSE;
   }
}

static gboolean
detect_pnm(guchar *filename, ImageInfo *info)
{
   pnm_info pnminfo;

   if (pnm_get_header(filename, &pnminfo))
   {
      info->type = PNM;
      info->width = pnminfo.width;
      info->height = pnminfo.height;
      info->ncolors = pnminfo.ncolors;
      info->real_ncolors = FALSE;
      info->alpha = 0;
      return TRUE;
   } else {
      return FALSE;
   }
}

static gboolean
detect_psd(guchar *filename, ImageInfo *info)
{
   psd_info psdinfo;

   if (psd_get_header(filename, &psdinfo))
   {
      info->type = PSD;
      info->width = psdinfo.width;
      info->height = psdinfo.height;
      info->ncolors = psdinfo.ncolors;
      info->real_ncolors = FALSE;
      info->alpha = psdinfo.alpha;
      return TRUE;
   } else {
      return FALSE;
   }
}

static gboolean
detect_xbm(guchar *filename, ImageInfo *info)
{
   xbm_info xbminfo;

   if (xbm_get_header(filename, &xbminfo))
   {
      info->type        = XBM;
      info->width       = xbminfo.width;
      info->height      = xbminfo.height;
      info->ncolors     = 2;
      info->real_ncolors= TRUE;
      info->alpha       = 0;
      return TRUE;
   } else {
      return FALSE;
   }
}

static gboolean
detect_xcf(guchar *filename, ImageInfo *info)
{
   xcf_info xcfinfo;

   if (xcf_get_header(filename, &xcfinfo))
   {
      info->type        = XCF;
      info->width       = xcfinfo.width;
      info->height      = xcfinfo.height;
      info->ncolors     = xcfinfo.ncolors;
      info->real_ncolors= FALSE;
      info->alpha       = xcfinfo.alpha;
      return TRUE;
   } else {
      return FALSE;
   }
}

static gboolean
detect_tga(guchar *filename, ImageInfo *info)
{
   tga_info tgainfo;

   if (tga_get_header(filename, &tgainfo))
   {
      info->type        = TGA;
      info->width       = tgainfo.Width;
      info->height      = tgainfo.Height;
      info->ncolors     = ((tgainfo.PixelDepth > 24) ? 24 : tgainfo.PixelDepth);
      info->alpha       = tgainfo.PixelDepth >> 5;
      info->real_ncolors= FALSE;
      return TRUE;
   } else {
      return FALSE;
   }
}

static gboolean
detect_xwd(guchar *filename, ImageInfo *info)
{
   xwd_info xwdinfo;

   if (xwd_get_header(filename, &xwdinfo))
   {
      info->type        = XWD;
      info->width       = (guint) xwdinfo.window_width;
      info->height      = (guint) xwdinfo.window_height;
      info->ncolors     = ((xwdinfo.pixmap_depth > 24) ? 24 : xwdinfo.pixmap_depth);
      info->alpha       = xwdinfo.pixmap_depth >> 5;
      info->real_ncolors= FALSE;
      return TRUE;
   } else {
      return FALSE;
   }
}

static gboolean
detect_sun(guchar *filename, ImageInfo *info)
{
   sun_info suninfo;

   if (sun_get_header(filename, &suninfo))
   {
      info->type        = SUN;
      info->width       = (guint) suninfo.width;
      info->height      = (guint) suninfo.height;
      info->ncolors     = ((suninfo.depth > 24) ? 24 : suninfo.depth);
      info->alpha       = suninfo.depth >> 5;
      info->real_ncolors= FALSE;
      return TRUE;
   } else {
      return FALSE;
   }
}

static gboolean
detect_eps(guchar *filename, ImageInfo *info)
{
   eps_info epsinfo;

   if (eps_get_header(filename, &epsinfo))
   {
      info->type        = EPS;
      info->width       = (guint) epsinfo.width;
      info->height      = (guint) epsinfo.height;
      info->ncolors     = 24;
      info->alpha       = 0;
      info->real_ncolors= FALSE;
      return TRUE;
   } else {
      return FALSE;
   }
}

static gboolean
detect_sgi(guchar *filename, ImageInfo *info)
{
   sgi_info sgiinfo;

   if (sgi_get_header(filename, &sgiinfo))
   {
      info->type        = SGI;
      info->width       = (guint) sgiinfo.xsize;
      info->height      = (guint) sgiinfo.ysize;
      info->ncolors     = 24;
      info->alpha       = 0;
      info->real_ncolors= FALSE;
      return TRUE;
   } else {
      return FALSE;
   }
}

static DetectFunc detect_funcs[MAX_IMAGE_TYPES - 1] =
{
   (DetectFunc)detect_jpeg,
   (DetectFunc)detect_png,
   (DetectFunc)detect_gif,
   (DetectFunc)detect_xcf,
   (DetectFunc)detect_xpm,
   (DetectFunc)detect_bmp,
   (DetectFunc)detect_ico,
   (DetectFunc)detect_pcx,
   (DetectFunc)detect_tiff,
   (DetectFunc)detect_pnm,
   (DetectFunc)detect_psd,
   (DetectFunc)detect_xbm,
   (DetectFunc)detect_tga,
   (DetectFunc)detect_xwd,
   (DetectFunc)detect_sun,
   (DetectFunc)detect_eps,
   (DetectFunc)detect_sgi,
};

gboolean
detect_image_type(guchar *filename, ImageInfo *info)
{
   gint i;

   if (info->type < UNKNOWN)
   {
      if ((*detect_funcs[info->type])(filename, info))
         return TRUE;
   }

   for (i = 0; i < UNKNOWN; i++)
   {
      if (i != info->type && i != ICO)
      {
         if ((*detect_funcs[i])(filename, info))
            return TRUE;
      }
   }

   return FALSE;
}
