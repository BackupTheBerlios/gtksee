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
 * 2003-09-02: Fixed little bug that cause problem
 *             reading BMP files.
 */

#include "config.h"

#define DISPLAY_SPEED 20

#define ABS(x) (((x)>=0)?(x):(-(x)))
#define MIN(x,y) (((x)<=(y))?(x):(y))
#define MAX(x,y) (((x)>=(y))?(x):(y))

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
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

#include "scale.h"
#include "scanline.h"
#include "detect.h"

#define RANDOM_SEED 314159265

static gint bgcolor_red, bgcolor_green, bgcolor_blue;

static GtkWidget *image_wid;
static GtkPreview *preview;
static guchar *scan_image_src;
static gint orig_image_width;
static gint current_scanflags;
static ImageCache image_cache = {NULL, 0, 0};
static gint first_scanline;
static gint last_scanline;
static gint total_scanline;

/* position fixed for old gtk+-1.0.x */
#ifndef GTK_HAVE_FEATURES_1_1_0
static gint left_pos, top_pos;
#endif

static gboolean scanline_cancelled = FALSE;

gboolean jpeg_scanline  (guchar *buffer,
             gint width,
             gint left,
             gint scanline,
             gint components,
             gint pass,
             LayerMode mode);
gboolean jpeg_scanline_buffered
            (guchar *buffer,
             gint width,
             gint left,
             gint scanline,
             gint components,
             gint pass,
             LayerMode mode);
gboolean gif_scanline   (guint *buffer,
             guchar *cmap[],
             gint transparent,
             gint width,
             gint scanline,
             gint frame);
gboolean gif_scanline_buffered
            (guint *buffer,
             guchar *cmap[],
             gint transparent,
             gint width,
             gint scanline,
             gint frame);

static gchar*  find_preprocess_extension
            (gchar *filename,
             gchar **command,
             gchar **args);
static gboolean   preprocess_second
            (gchar *filename);
static gchar*  find_filename  (gchar *filename);
/* src1, src2 and dest are pointers to one pixel
 * b1 and b2 are num_bytes values for src1 and src2
 * ha1 and ha2 tell me whether src1 and src2 has alpha
 */
static void pixel_apply_layer_mode
            (guchar *src1,
             guchar *src2,
             guchar *dest,
             gint b1,
             gint b2,
             gboolean ha1,
             gboolean ha2,
             LayerMode mode);
static void rgb_to_hsv  (gint *r,
             gint *g,
             gint *b);
static void hsv_to_rgb  (gint *h,
             gint *s,
             gint *v);
static void rgb_to_hls  (gint *r,
             gint *g,
             gint *b);
static void hls_to_rgb  (gint *h,
             gint *l,
             gint *s);
static gint hls_value   (gfloat n1,
             gfloat n2,
             gfloat hue);

static gchar*
find_preprocess_extension(gchar *filename, gchar **command, gchar **args)
{
   gchar *ext;

   ext = strrchr(filename, '.');
   if (ext == NULL) return NULL;
   if (strcmp(ext, ".gz") == 0)
   {
      *command = "gzip";
      *args = "-cfd";
      return ext;
   } else if (strcmp(ext, ".bz2") == 0)
   {
      *command = "bzip2";
      *args = "-cfd";
      return ext;
   } else if (strcmp(ext, ".bz") == 0)
   {
      *command = "bzip";
      *args = "-cd";
      return ext;
   } else if (strcmp(ext, ".zip") == 0)
   {
      *command = "unzip";
      *args = "-p";
      return ext;
   } else
   {
      return NULL;
   }
}

static gchar*
find_filename(gchar *filename)
{
   gchar *ext;

   if ((ext = strrchr(filename, '/')) == NULL)
   {
      return NULL;
   } else
   {
      ext++;
      if (*ext == '\0') return NULL;
      return ext;
   }
}

/*
 * Other special preprocesses, such as wmftogif.
 * The content of the 'filename' parameter should be
 * replace if the preprocess converts from one file to
 * another. Return value indicates whether any second
 * preprocess has been performed.
 */
static gboolean
preprocess_second(gchar *filename)
{
   guchar *ext, *tempfile, newfile[256];
   gint pid, status;

   ext = strrchr(filename, '.');
   if (ext == NULL) return FALSE;
   if (strcmp(ext, ".wmf") == 0)
   {
      /* try to preprocess it into /tmp */
      if ((tempfile = find_filename(filename)) == NULL)
         return FALSE;
      strcpy(newfile, "/tmp/");
      strncat(newfile, tempfile, ext - tempfile);
      strcat(newfile, ".gif");
      if ((pid = fork()) < 0) return FALSE;
      if (pid == 0)
      {
         /* child process */
         FILE* f;
         if (!(f = fopen("/dev/null","w")))
         {
            _exit(127);
         }
         if (-1 == dup2(fileno(f),fileno(stderr)))
         {
            _exit(127);
         }
         execlp ("wmftogif", "wmftogif",
            filename, newfile, NULL);
         _exit(127);
      } else
      {
         /* parent process */
         waitpid(pid, &status, 0);
         strcpy(filename, newfile);
         return TRUE;
      }
   }
   return FALSE;
}

void
scanline_init(GdkColorContext *gcc, GtkStyle *style)
{
   bgcolor_red = (style->bg[GTK_STATE_NORMAL]).red >> 8;
   bgcolor_green = (style->bg[GTK_STATE_NORMAL]).green >> 8;
   bgcolor_blue = (style->bg[GTK_STATE_NORMAL]).blue >> 8;
   srand(RANDOM_SEED);
}

void
scanline_start()
{
   scanline_cancelled = FALSE;
}

void
scanline_cancel()
{
   scanline_cancelled = TRUE;
}

gboolean
jpeg_scanline(
   guchar *buffer,
   gint width,
   gint left,
   gint scanline,
   gint components,
   gint pass,
   LayerMode mode)
{
   register gint i, j, k;
   static gint alpha, cur_red, cur_green, cur_blue, display_height;
   static guchar src1[4], src2[4], dest[4];
   static GdkRectangle rect;
   register guchar *rgbbuffer = image_cache.buffer +
      (image_cache.buffer_width * scanline + left) * 3;

   if (pass <= 0)
   {
      cur_red = bgcolor_red;
      cur_green = bgcolor_green;
      cur_blue = bgcolor_blue;
   }
   for (i=0, j=0, k=0; i<width; i++)
   {
      if (pass > 0)
      {
         cur_red = rgbbuffer[k];
         cur_green = rgbbuffer[k + 1];
         cur_blue = rgbbuffer[k + 2];
      }
      switch (components)
      {
        case 1:
         rgbbuffer[k++] = buffer[j];
         rgbbuffer[k++] = buffer[j];
         rgbbuffer[k++] = buffer[j++];
         break;
        case 2:
         if (pass > 0 && mode > 0)
         {
            src1[0] = cur_red;
            src1[1] = cur_green;
            src1[2] = cur_blue;
            src1[3] = 255;
            src2[0] = buffer[j];
            src2[1] = buffer[j];
            src2[2] = buffer[j];
            src2[3] = buffer[j+1];
            pixel_apply_layer_mode(src1, src2, dest, 4, 4, TRUE, TRUE, mode);
            alpha = dest[3];
            rgbbuffer[k++] = dest[0] * alpha / 255 + cur_red * (255-alpha) / 255;
            rgbbuffer[k++] = dest[1] * alpha / 255 + cur_green * (255-alpha) / 255;
            rgbbuffer[k++] = dest[2] * alpha / 255 + cur_blue * (255-alpha) / 255;
         } else
         {
            alpha = buffer[j+1];
            rgbbuffer[k++] = buffer[j] * alpha / 255 + cur_red * (255-alpha) / 255;
            rgbbuffer[k++] = buffer[j] * alpha / 255 + cur_green * (255-alpha) / 255;
            rgbbuffer[k++] = buffer[j] * alpha / 255 + cur_blue * (255-alpha) / 255;
         }
         j += 2;
         break;
        case 3:
         rgbbuffer[k++] = buffer[j++];
         rgbbuffer[k++] = buffer[j++];
         rgbbuffer[k++] = buffer[j++];
         break;
        case 4:
         if (pass > 0 && mode > 0)
         {
            src1[0] = cur_red;
            src1[1] = cur_green;
            src1[2] = cur_blue;
            src1[3] = 255;
            src2[0] = buffer[j];
            src2[1] = buffer[j+1];
            src2[2] = buffer[j+2];
            src2[3] = buffer[j+3];
            pixel_apply_layer_mode(src1, src2, &buffer[j], 4, 4, TRUE, TRUE, mode);
         }
         alpha = buffer[j+3];
         rgbbuffer[k++] = buffer[j++] * alpha / 255 + cur_red * (255-alpha) / 255;
         rgbbuffer[k++] = buffer[j++] * alpha / 255 + cur_green * (255-alpha) / 255;
         rgbbuffer[k++] = buffer[j++] * alpha / 255 + cur_blue * (255-alpha) / 255;
         j++;
         break;
        default:
         break;
      }
   }
/* INICIO DEL AGREGADO DE DANIEL OME
   if(preview->buffer_width != width)
      preview->buffer_width = width;
*/
   //gtk_preview_draw_row(preview, rgbbuffer, left, scanline, width);
/*FINAL DEL AGREGADO DE DANIEL OME*/
//gtk_preview_draw_row(preview, rgbbuffer, left, scanline, width);
   if(preview->buffer_width != width)
      preview->buffer_width = width;

   gtk_preview_draw_row(preview, rgbbuffer, left, scanline, width);

   if (current_scanflags & SCANLINE_INTERACT)
   {
      while (gtk_events_pending()) gtk_main_iteration();
   }
   if (image_wid == NULL) return FALSE;
   if (!(current_scanflags & SCANLINE_DISPLAY)) return FALSE;

   if (pass >= 0)
   {
#ifdef GTK_HAVE_FEATURES_1_1_0
      rect.x = left;
      rect.y = scanline;
#else
      rect.x = left_pos + left;
      rect.y = top_pos + scanline;
#endif
      rect.width = width;
      rect.height = 1;
      gtk_widget_draw(image_wid, &rect);
      return scanline_cancelled;
   }

   if (++total_scanline < preview->buffer_height)
   {
      if (first_scanline < 0)
      {
         first_scanline = last_scanline = scanline;
         return FALSE;
      }
      if (ABS(last_scanline - scanline) == 1 &&
         ABS(first_scanline - scanline) < DISPLAY_SPEED)
      {
         last_scanline = scanline;
         return FALSE;
      }
      last_scanline = scanline;
   } else
   {
      if (first_scanline < 0) first_scanline = scanline;
      last_scanline = scanline;
   }

   display_height = ABS(last_scanline - first_scanline) + 1;
#ifdef GTK_HAVE_FEATURES_1_1_0
   rect.x = left;
   rect.y = MIN(first_scanline, last_scanline);
#else
   rect.x = left_pos + left;
   rect.y = top_pos + MIN(first_scanline, last_scanline);
#endif
   rect.width = width;
   rect.height = display_height;
   gtk_widget_draw(image_wid, &rect);
   first_scanline = -1;
   return scanline_cancelled;
}

gboolean
jpeg_scanline_buffered(
   guchar *buffer,
   gint width,
   gint left,
   gint scanline,
   gint components,
   gint pass,
   LayerMode mode)
{
   register gint i, j, k;
   static guint ptr, alpha, cur_red, cur_green, cur_blue;
   static guchar src1[4], src2[4], dest[4];

   if (pass <= 0)
   {
      cur_red = bgcolor_red;
      cur_green = bgcolor_green;
      cur_blue = bgcolor_blue;
   }

   switch (components)
   {
     case 2:
      /* turning grayscale-alpha into RGB */
      ptr = (scanline * orig_image_width + left) * 3;
      for (i = 0, j = 0, k = 0; i < width; i++)
      {
         if (pass > 0)
         {
            cur_red = scan_image_src[ptr + k];
            cur_green = scan_image_src[ptr + k + 1];
            cur_blue = scan_image_src[ptr + k + 2];
         }
         if (pass > 0 && mode > 0)
         {
            src1[0] = cur_red;
            src1[1] = cur_green;
            src1[2] = cur_blue;
            src1[3] = 255;
            src2[0] = buffer[j];
            src2[1] = buffer[j];
            src2[2] = buffer[j];
            src2[3] = buffer[j+1];
            pixel_apply_layer_mode(src1, src2, dest, 4, 4, TRUE, TRUE, mode);
            alpha = dest[3];
            scan_image_src[ptr + k++] = (dest[0] * alpha / 255 + cur_red * (255-alpha) / 255);
            scan_image_src[ptr + k++] = (dest[1] * alpha / 255 + cur_green * (255-alpha) / 255);
            scan_image_src[ptr + k++] = (dest[2] * alpha / 255 + cur_blue * (255-alpha) / 255);
         } else
         {
            alpha = buffer[j+1];
            scan_image_src[ptr + k++] = (buffer[j] * alpha / 255 + cur_red * (255-alpha) / 255);
            scan_image_src[ptr + k++] = (buffer[j] * alpha / 255 + cur_green * (255-alpha) / 255);
            scan_image_src[ptr + k++] = (buffer[j] * alpha / 255 + cur_blue * (255-alpha) / 255);
         }
         j += 2;
      }
      break;
     case 4:
      /* turning RGBA into RGB, using bgcolor */
      ptr = (scanline * orig_image_width + left) * 3;
      for (i = 0, j = 0, k = 0; i < width; i++)
      {
         if (pass > 0)
         {
            cur_red = scan_image_src[ptr + k];
            cur_green = scan_image_src[ptr + k + 1];
            cur_blue = scan_image_src[ptr + k + 2];
            if (mode > 0)
            {
               src1[0] = cur_red;
               src1[1] = cur_green;
               src1[2] = cur_blue;
               src1[3] = 255;
               src2[0] = buffer[j];
               src2[1] = buffer[j+1];
               src2[2] = buffer[j+2];
               src2[3] = buffer[j+3];
               pixel_apply_layer_mode(src1, src2, &buffer[j], 4, 4, TRUE, TRUE, mode);
            }
         }
         alpha = buffer[j+3];
         scan_image_src[ptr + k++] = (buffer[j++] * alpha / 255 + cur_red * (255-alpha) / 255);
         scan_image_src[ptr + k++] = (buffer[j++] * alpha / 255 + cur_green * (255-alpha) / 255);
         scan_image_src[ptr + k++] = (buffer[j++] * alpha / 255 + cur_blue * (255-alpha) / 255);
         j++;
      }
      break;
     default:
      memcpy(scan_image_src + (scanline * width + left) * components, buffer, width * components);
      break;
   }
   return scanline_cancelled;
}

gboolean
gif_scanline(guint *buffer, guchar *cmap[], gint transparent, gint width, gint scanline, gint frame)
{
   register gint i, k, c, display_height;
   static GdkRectangle rect;
   register guchar *rgbbuffer = image_cache.buffer +
      image_cache.buffer_width * scanline * 3;

   for (i=0, k=0; i<width; i++)
   {
      if ((c = buffer[i]) == transparent)
      {
         rgbbuffer[k++] = bgcolor_red;
         rgbbuffer[k++] = bgcolor_green;
         rgbbuffer[k++] = bgcolor_blue;
      } else
      {
         rgbbuffer[k++] = cmap[0][c];
         rgbbuffer[k++] = cmap[1][c];
         rgbbuffer[k++] = cmap[2][c];
      }
   }
   gtk_preview_draw_row(preview, rgbbuffer, 0, scanline, width);
   if (current_scanflags & SCANLINE_INTERACT)
   {
      while (gtk_events_pending()) gtk_main_iteration();
   }
   if (image_wid == NULL) return FALSE;
   if (!(current_scanflags & SCANLINE_DISPLAY)) return FALSE;
   if (++total_scanline < preview->buffer_height)
   {
      if (first_scanline < 0)
      {
         first_scanline = last_scanline = scanline;
         return FALSE;
      }
      if (ABS(last_scanline - scanline) == 1 &&
         ABS(first_scanline - scanline) < DISPLAY_SPEED)
      {
         last_scanline = scanline;
         return FALSE;
      }
      last_scanline = scanline;
   } else
   {
      if (first_scanline < 0) first_scanline = scanline;
      last_scanline = scanline;
   }
   display_height = ABS(last_scanline - first_scanline) + 1;
#ifdef GTK_HAVE_FEATURES_1_1_0
   rect.x = 0;
   rect.y = MIN(first_scanline, last_scanline);
#else
   rect.x = left_pos;
   rect.y = top_pos + MIN(first_scanline, last_scanline);
#endif
   rect.width = width;
   rect.height = display_height;
   gtk_widget_draw(image_wid, &rect);
   first_scanline = -1;
   return scanline_cancelled;
}

gboolean
gif_scanline_buffered(guint *buffer, guchar *cmap[], gint transparent, gint width, gint scanline, gint frame)
{
   register gchar *j;
   register gint i, c;

   for (i=0, j=scan_image_src+width*scanline*3; i<width; i++)
   {
      if ((c = buffer[i]) == transparent)
      {
         *(j++) = bgcolor_red;
         *(j++) = bgcolor_green;
         *(j++) = bgcolor_blue;
      } else
      {
         *(j++) = cmap[0][c];
         *(j++) = cmap[1][c];
         *(j++) = cmap[2][c];
      }
   }
   return scanline_cancelled;
}

void
load_scaled_image(
   gchar *orig_filename,
   ImageInfo *info,
   gint max_width,
   gint max_height,
   GtkWidget *window,
   GtkWidget *container,
   gint scanflags)
{
   gint width, height, pid, status, comp;
   gboolean no_scale, is_tempfile;
   gchar *d, *ext, *tempfile;
   gchar filename[256];
   gchar *cmd, *args;
   ImageType type;
   gint orig_width, orig_height, ncolors;

   current_scanflags = scanflags;
   first_scanline = -1;
   total_scanline = 0;

   /* Whether the image file should be preprocessed */
   if ((ext = find_preprocess_extension(orig_filename, &cmd, &args)) == NULL)
   {
      strcpy(filename, orig_filename);
      is_tempfile = FALSE;
   } else
   {
      /* try to preprocess it into /tmp */
      if ((tempfile = find_filename(orig_filename)) == NULL) return;
      strcpy(filename, "/tmp/");
      strncat(filename, tempfile, ext - tempfile);
      is_tempfile = TRUE;
      if ((pid = fork()) < 0) return;
      if (pid == 0)
      {
         /* child process */
         FILE* f;
         if (!(f = fopen(filename,"w")))
         {
            _exit(127);
         }
         if (-1 == dup2(fileno(f),fileno(stdout)))
         {
            _exit(127);
         }
         execlp (cmd, cmd, args, orig_filename, NULL);
         _exit(127);
      } else
      {
         /* parent process */
         waitpid(pid, &status, 0);
         /* we need to get the header field for gzipped images */
         detect_image_type(filename, info);
      }
   }

   if (preprocess_second(filename))
   {
      detect_image_type(filename, info);
      is_tempfile = TRUE;
   }

   if (info->width < 0)
   {
      if (is_tempfile) unlink(filename);
      info->valid = FALSE;
      return;
   }

   type = info->type;
   orig_width = info->width;
   orig_height = info->height;
   ncolors = info->ncolors;

   orig_image_width = orig_width;

   info->desc = NULL;
   info->has_desc = FALSE;

   no_scale = max_width < 0 || max_height < 0 ||
      (orig_width <= max_width && orig_height <= max_height);

/* PARCHE
   no_scale = max_width < 0 || max_height < 0;
*/
   if (no_scale)
   {
      width = orig_width;
      height = orig_height;
      if (container == NULL)
      {
         image_wid = NULL;
      } else
      {
         image_wid = container;
         preview = GTK_PREVIEW(image_wid);
         gtk_preview_size(preview, width, height);
         gtk_widget_queue_resize(image_wid);
         if (image_cache.buffer != NULL)
         {
            g_free(image_cache.buffer);
         }
         image_cache.buffer = g_malloc(width * 3 * height);
         image_cache.buffer_width = width;
         image_cache.buffer_height = height;
         if (current_scanflags & SCANLINE_DISPLAY)
         {
            while (image_wid->allocation.width < width ||
               image_wid->allocation.height < height ||
               gtk_events_pending())
               gtk_main_iteration();
         }
#ifndef GTK_HAVE_FEATURES_1_1_0
         left_pos = (image_wid->allocation.width - width) / 2;
         top_pos = (image_wid->allocation.height - height) / 2;
#endif
      }

      switch (type)
      {
         case JPG:
#ifdef HAVE_LIBJPEG
            jpeg_load(filename, (JpegLoadFunc)jpeg_scanline);
#else
            return;
#endif
            break;
         case TIF:
#ifdef HAVE_LIBTIFF
            tiff_load(filename, (TiffLoadFunc)jpeg_scanline);
#else
            return;
#endif
            break;
         case PNG:
#ifdef HAVE_LIBPNG
            png_load(filename, (PngLoadFunc)jpeg_scanline);
#else
            return;
#endif
            break;
         case BMP:
            bmp_load(filename, (BmpLoadFunc)jpeg_scanline);
            break;
         case GIF:
            d = gif_load(filename, (GifLoadFunc)gif_scanline);
            if (d != NULL && strlen(d) > 0)
            {
               info->desc = d;
               info->has_desc = TRUE;
            }
            break;
         case XPM:
            xpm_load(filename, (XpmLoadFunc)gif_scanline);
            break;
         case ICO:
            ico_load(filename, (IcoLoadFunc)gif_scanline);
            break;
         case PCX:
            pcx_load(filename, (PcxLoadFunc)jpeg_scanline);
            break;
         case PNM:
            pnm_load(filename, (PnmLoadFunc)jpeg_scanline);
            break;
         case PSD:
            d = psd_load(filename, (PsdLoadFunc)jpeg_scanline);
            if (d != NULL && strlen(d) > 0)
            {
               info->desc = d;
               info->has_desc = TRUE;
            }
            break;
         case XBM:
           xbm_load(filename, (XbmLoadFunc)jpeg_scanline);
            break;
         case XCF:
            xcf_load(filename, (XcfLoadFunc)jpeg_scanline);
            break;
         case TGA:
            tga_load(filename, (TgaLoadFunc)jpeg_scanline);
            break;
         case XWD:
            xwd_load(filename, (XwdLoadFunc)jpeg_scanline);
         case SUN:
            sun_load(filename, (SunLoadFunc)jpeg_scanline);
         case EPS:
            eps_load(filename, (EpsLoadFunc)jpeg_scanline);
         case SGI:
            sgi_load(filename, (SgiLoadFunc)jpeg_scanline);
            break;
         default:
            return;
      }
   } else
   {
      if (max_width * orig_height < max_height * orig_width)
      {
         width = max_width;
         height = width * orig_height / orig_width;
      } else
      {
         height = max_height;
         width = height * orig_width / orig_height;
      }

      if (container == NULL)
      {
         image_wid = NULL;
      } else
      {
         image_wid = container;
         preview = GTK_PREVIEW(image_wid);
         gtk_preview_size(preview, width, height);
         gtk_widget_queue_resize(image_wid);
         if (image_cache.buffer != NULL)
         {
            g_free(image_cache.buffer);
         }
         image_cache.buffer = g_malloc(width * 3 * height);
         image_cache.buffer_width = width;
         image_cache.buffer_height = height;
         if (current_scanflags & SCANLINE_DISPLAY)
         {
            while (image_wid->allocation.width < width ||
               image_wid->allocation.height < height ||
               gtk_events_pending())
               gtk_main_iteration();
         }
#ifndef GTK_HAVE_FEATURES_1_1_0
         left_pos = (image_wid->allocation.width - width) / 2;
         top_pos = (image_wid->allocation.height - height) / 2;
#endif
      }

      if (type == JPG)
      {
#ifdef HAVE_LIBJPEG
         comp = ncolors / 8;
         scan_image_src = g_malloc(orig_width * orig_height * comp);
         jpeg_load(filename, (JpegLoadFunc)jpeg_scanline_buffered);
#else
         return;
#endif
      } else
      if (type == TIF)
      {
#ifdef HAVE_LIBTIFF
         comp = 3;
         scan_image_src = g_malloc(orig_width * orig_height * 3);
         tiff_load(filename, (TiffLoadFunc)jpeg_scanline_buffered);
#else
         return;
#endif
      } else
      if (type == PNG)
      {
#ifdef HAVE_LIBPNG
         if (info->bpp == 1) comp = 1;
         else comp = 3;
         scan_image_src = g_malloc(orig_width * orig_height * comp);
         png_load(filename, (PngLoadFunc)jpeg_scanline_buffered);
#else
         return;
#endif
      } else
      if (type == PNM)
      {
         comp = ncolors / 8;
         if (comp < 1) comp = 1;
         scan_image_src = g_malloc(orig_width * orig_height * comp);
         pnm_load(filename, (PnmLoadFunc)jpeg_scanline_buffered);
      } else
      if (type == XBM)
      {
         comp = 1;
         scan_image_src = g_malloc(orig_width * orig_height);
         xbm_load(filename, (XbmLoadFunc)jpeg_scanline_buffered);
      } else
      if (type == BMP)
      {
         comp = 3;
         scan_image_src = g_malloc(orig_width * orig_height * 3);
         bmp_load(filename, (BmpLoadFunc)jpeg_scanline_buffered);
      } else
      if (type == GIF)
      {
         comp = 3;
         scan_image_src = g_malloc(orig_width * orig_height * 3);
         d = gif_load(filename, (GifLoadFunc)gif_scanline_buffered);
         if (d != NULL && strlen(d) > 0)
         {
            info->desc = d;
            info->has_desc = TRUE;
         }
      } else
      if (type == XPM)
      {
         comp = 3;
         scan_image_src = g_malloc(orig_width * orig_height * 3);
         xpm_load(filename, (XpmLoadFunc)gif_scanline_buffered);
      } else
      if (type == ICO)
      {
         comp = 3;
         scan_image_src = g_malloc(orig_width * orig_height * 3);
         ico_load(filename, (IcoLoadFunc)gif_scanline_buffered);
      } else
      if (type == PCX)
      {
         comp = 3;
         scan_image_src = g_malloc(orig_width * orig_height * 3);
         pcx_load(filename, (PcxLoadFunc)jpeg_scanline_buffered);
      } else
      if (type == PSD)
      {
         comp = 3;
         scan_image_src = g_malloc(orig_width * orig_height * 3);
         d = psd_load(filename, (PsdLoadFunc)jpeg_scanline_buffered);
         if (d != NULL && strlen(d) > 0)
         {
            info->desc = d;
            info->has_desc = TRUE;
         }
      } else
      if (type == XCF)
      {
         comp = 3;
         scan_image_src = g_malloc(orig_width * orig_height * 3);
         xcf_load(filename, (XcfLoadFunc)jpeg_scanline_buffered);
      } else
      if (type == TGA)
      {
         comp = 3;
         scan_image_src = g_malloc(orig_width * orig_height * 3);
         tga_load(filename, (TgaLoadFunc)jpeg_scanline_buffered);
      } else
      if (type == XWD)
      {
         comp = 3;
         scan_image_src = g_malloc(orig_width * orig_height * 3);
         xwd_load(filename, (XwdLoadFunc)jpeg_scanline_buffered);
      }
      if (type == SUN)
      {
         comp = 3;
         scan_image_src = g_malloc(orig_width * orig_height * 3);
         sun_load(filename, (SunLoadFunc)jpeg_scanline_buffered);
      }
      if (type == EPS)
      {
         comp = 3;
         scan_image_src = g_malloc(orig_width * orig_height * 3);
         eps_load(filename, (EpsLoadFunc)jpeg_scanline_buffered);
      }
      if (type == SGI)
      {
         comp = 3;
         scan_image_src = g_malloc(orig_width * orig_height * 3);
         sgi_load(filename, (SgiLoadFunc)jpeg_scanline_buffered);
      }

      /* Important!!!
       * When the GIF image was read to buffer, it has
       * been turned into RGB, but not indexed.
       * So, we just use jpeg_scanline.
       * It's always 3 bytes per pixel.
       * Don't worry about the transparent color. :)
       */
      if (!scanline_cancelled)
      {
         if (current_scanflags & SCANLINE_PREVIEW)
         {
            scale_region_preview(scan_image_src,
               orig_width, orig_height,
               max_width, max_height,
               comp, (ScaleFunc)jpeg_scanline);
         } else
         {
            scale_region(scan_image_src,
               orig_width, orig_height,
               max_width, max_height,
               comp, (ScaleFunc)jpeg_scanline);
         }
      }
      g_free(scan_image_src);
   }

   /* If the filename is decompressed one, unlink it */
   if (is_tempfile) unlink(filename);
}

static void
pixel_apply_layer_mode(
   guchar *src1,
   guchar *src2,
   guchar *dest,
   gint b1,
   gint b2,
   gboolean ha1,
   gboolean ha2,
   LayerMode mode)
{
   static gint b, alpha, t, t2, red1, green1, blue1, red2, green2, blue2;

   alpha = (ha1 || ha2) ? max(b1, b2) - 1 : b1;
   switch (mode)
   {
     case DISSOLVE_MODE:
      for (b = 0; b < alpha; b++)
         dest[b] = src2[b];
      /*  dissolve if random value is > opacity  */
      t = (rand() & 0xFF);
      dest[alpha] = (t > src2[alpha]) ? 0 : src2[alpha];
      break;
     case MULTIPLY_MODE:
      for (b = 0; b < alpha; b++)
         dest[b] = (src1[b] * src2[b]) / 255;
      if (ha1 && ha2)
         dest[alpha] = min(src1[alpha], src2[alpha]);
      else if (ha2)
         dest[alpha] = src2[alpha];
      break;
     case SCREEN_MODE:
      for (b = 0; b < alpha; b++)
         dest[b] = 255 - ((255 - src1[b]) * (255 - src2[b])) / 255;
      if (ha1 && ha2)
         dest[alpha] = min(src1[alpha], src2[alpha]);
      else if (ha2)
         dest[alpha] = src2[alpha];
      break;
     case OVERLAY_MODE:
      for (b = 0; b < alpha; b++)
      {
         /* screen */
         t = 255 - ((255 - src1[b]) * (255 - src2[b])) / 255;
         /* mult */
         t2 = (src1[b] * src2[b]) / 255;
         dest[b] = (t * src1[b] + t2 * (255 - src1[b])) / 255;
      }
      if (ha1 && ha2)
         dest[alpha] = min(src1[alpha], src2[alpha]);
      else if (ha2)
         dest[alpha] = src2[alpha];
      break;
     case DIFFERENCE_MODE:
      for (b = 0; b < alpha; b++)
      {
         t = src1[b] - src2[b];
         dest[b] = (t < 0) ? -t : t;
      }
      if (ha1 && ha2)
         dest[alpha] = min(src1[alpha], src2[alpha]);
      else if (ha2)
         dest[alpha] = src2[alpha];
      break;
     case ADDITION_MODE:
      for (b = 0; b < alpha; b++)
      {
         t = src1[b] + src2[b];
         dest[b] = (t > 255) ? 255 : t;
      }
      if (ha1 && ha2)
         dest[alpha] = min(src1[alpha], src2[alpha]);
      else if (ha2)
         dest[alpha] = src2[alpha];
      break;
     case SUBTRACT_MODE:
      for (b = 0; b < alpha; b++)
      {
         t = (gint)src1[b] - (gint)src2[b];
         dest[b] = (t < 0) ? 0 : t;
      }
      if (ha1 && ha2)
         dest[alpha] = min(src1[alpha], src2[alpha]);
      else if (ha2)
         dest[alpha] = src2[alpha];
      break;
     case DARKEN_ONLY_MODE:
      for (b = 0; b < alpha; b++)
      {
         t = src1[b];
         t2 = src2[b];
         dest[b] = (t < t2) ? t : t2;
      }
      if (ha1 && ha2)
         dest[alpha] = min(src1[alpha], src2[alpha]);
      else if (ha2)
         dest[alpha] = src2[alpha];
      break;
     case LIGHTEN_ONLY_MODE:
      for (b = 0; b < alpha; b++)
      {
         t = src1[b];
         t2 = src2[b];
         dest[b] = (t < t2) ? t2 : t;
      }
      if (ha1 && ha2)
         dest[alpha] = min(src1[alpha], src2[alpha]);
      else if (ha2)
         dest[alpha] = src2[alpha];
      break;
     case HUE_MODE:
     case SATURATION_MODE:
     case VALUE_MODE:
      red1 = src1[0]; green1 = src1[1]; blue1 = src1[2];
      red2 = src2[0]; green2 = src2[1]; blue2 = src2[2];
      rgb_to_hsv (&red1, &green1, &blue1);
      rgb_to_hsv (&red2, &green2, &blue2);
      switch (mode)
      {
        case HUE_MODE:
         red1 = red2;
         break;
        case SATURATION_MODE:
         green1 = green2;
         break;
        case VALUE_MODE:
         blue1 = blue2;
         break;
        default:
         break;
      }
      hsv_to_rgb (&red1, &green1, &blue1);
      dest[0] = red1; dest[1] = green1; dest[2] = blue1;
      if (ha1 && ha2)
         dest[3] = min(src1[3], src2[3]);
      else if (ha2)
         dest[3] = src2[3];
      break;
     case COLOR_MODE:
      red1 = src1[0]; green1 = src1[1]; blue1 = src1[2];
      red2 = src2[0]; green2 = src2[1]; blue2 = src2[2];
      rgb_to_hls (&red1, &green1, &blue1);
      rgb_to_hls (&red2, &green2, &blue2);
      red1 = red2;
      blue1 = blue2;
      hls_to_rgb (&red1, &green1, &blue1);
      dest[0] = red1; dest[1] = green1; dest[2] = blue1;
      if (ha1 && ha2)
         dest[3] = min(src1[3], src2[3]);
      else if (ha2)
         dest[3] = src2[3];
      break;
     case NORMAL_MODE:
     default:
      for (b = 0; b < b2; b++) dest[b] = src2[b];
      break;
   }
}

static void
rgb_to_hsv (gint *r,
            gint *g,
            gint *b)
{
  gint red, green, blue;
  gfloat h, s, v;
  gint min, max;
  gint delta;

  h = 0.0;

  red = *r;
  green = *g;
  blue = *b;

  if (red > green)
    {
      if (red > blue)
        max = red;
      else
        max = blue;

      if (green < blue)
        min = green;
      else
        min = blue;
    }
  else
    {
      if (green > blue)
        max = green;
      else
        max = blue;

      if (red < blue)
        min = red;
      else
        min = blue;
    }

  v = max;

  if (max != 0)
    s = ((max - min) * 255) / (gfloat) max;
  else
    s = 0;

  if (s == 0)
    h = 0;
  else
    {
      delta = max - min;
      if (red == max)
        h = (green - blue) / (gfloat) delta;
      else if (green == max)
        h = 2 + (blue - red) / (gfloat) delta;
      else if (blue == max)
        h = 4 + (red - green) / (gfloat) delta;
      h *= 42.5;

      if (h < 0)
        h += 255;
      if (h > 255)
        h -= 255;
    }

  *r = h;
  *g = s;
  *b = v;
}

static void
hsv_to_rgb (gint *h,
            gint *s,
            gint *v)
{
  gfloat hue, saturation, value;
  gfloat f, p, q, t;

  if (*s == 0)
    {
      *h = *v;
      *s = *v;
      *v = *v;
    }
  else
    {
      hue = *h * 6.0 / 255.0;
      saturation = *s / 255.0;
      value = *v / 255.0;

      f = hue - (gint) hue;
      p = value * (1.0 - saturation);
      q = value * (1.0 - (saturation * f));
      t = value * (1.0 - (saturation * (1.0 - f)));

      switch ((gint) hue)
        {
        case 0:
          *h = value * 255;
          *s = t * 255;
          *v = p * 255;
          break;
        case 1:
          *h = q * 255;
          *s = value * 255;
          *v = p * 255;
          break;
        case 2:
          *h = p * 255;
          *s = value * 255;
          *v = t * 255;
          break;
        case 3:
          *h = p * 255;
          *s = q * 255;
          *v = value * 255;
          break;
        case 4:
          *h = t * 255;
          *s = p * 255;
          *v = value * 255;
          break;
        case 5:
          *h = value * 255;
          *s = p * 255;
          *v = q * 255;
          break;
        }
    }
}

static void
rgb_to_hls (gint *r,
            gint *g,
            gint *b)
{
  gint red, green, blue;
  gfloat h, l, s;
  gint min, max;
  gint delta;

  red = *r;
  green = *g;
  blue = *b;

  if (red > green)
    {
      if (red > blue)
        max = red;
      else
        max = blue;

      if (green < blue)
        min = green;
      else
        min = blue;
    }
  else
    {
      if (green > blue)
        max = green;
      else
        max = blue;

      if (red < blue)
        min = red;
      else
        min = blue;
    }

  l = (max + min) / 2.0;

  if (max == min)
    {
      s = 0.0;
      h = 0.0;
    }
  else
    {
      delta = (max - min);

      if (l < 128)
        s = 255 * (gfloat) delta / (gfloat) (max + min);
      else
        s = 255 * (gfloat) delta / (gfloat) (511 - max - min);

      if (red == max)
        h = (green - blue) / (gfloat) delta;
      else if (green == max)
        h = 2 + (blue - red) / (gfloat) delta;
      else
        h = 4 + (red - green) / (gfloat) delta;

      h = h * 42.5;

      if (h < 0)
        h += 255;
      if (h > 255)
        h -= 255;
    }

  *r = h;
  *g = l;
  *b = s;
}

static gint
hls_value (gfloat n1,
           gfloat n2,
           gfloat hue)
{
  gfloat value;

  if (hue > 255)
    hue -= 255;
  else if (hue < 0)
    hue += 255;
  if (hue < 42.5)
    value = n1 + (n2 - n1) * (hue / 42.5);
  else if (hue < 127.5)
    value = n2;
  else if (hue < 170)
    value = n1 + (n2 - n1) * ((170 - hue) / 42.5);
  else
    value = n1;

  return (gint) (value * 255);
}

static void
hls_to_rgb (gint *h,
            gint *l,
            gint *s)
{
  gfloat hue, lightness, saturation;
  gfloat m1, m2;

  hue = *h;
  lightness = *l;
  saturation = *s;

  if (saturation == 0)
    {
      /*  achromatic case  */
      *h = lightness;
      *l = lightness;
      *s = lightness;
    }
  else
    {
      if (lightness < 128)
        m2 = (lightness * (255 + saturation)) / 65025.0;
      else
        m2 = (lightness + saturation - (lightness * saturation)/255.0) / 255.0;

      m1 = (lightness / 127.5) - m2;

      /*  chromatic case  */
      *h = hls_value (m1, m2, hue + 85);
      *l = hls_value (m1, m2, hue);
      *s = hls_value (m1, m2, hue - 85);
    }
}

ImageCache*
scanline_get_cache(void)
{
   return &image_cache;
}
