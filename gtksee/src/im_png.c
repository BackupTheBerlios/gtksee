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

#include "config.h"

#ifdef HAVE_LIBPNG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <png.h>
#include <setjmp.h>

#include "im_png.h"

void
png_skip_warning(png_struct *pp, guchar *w)
{
}

typedef struct
{
   jmp_buf setjmp_buffer;  /* for return to caller */
} my_error_mgr;

static my_error_mgr jerr;

static void
my_error_exit(png_struct *pp, guchar *w)
{
   longjmp (jerr.setjmp_buffer, 1);
}

/*
 *--------------------
 * Begin the code ...
 *--------------------
 */

gboolean
png_get_header(gchar *filename, png_info *info)
{
   png_struct  *pp;
   png_info    *linfo;
   FILE        *fp;

   if ((fp=fopen(filename,"rb")) == NULL)
      return FALSE;

#if PNG_LIBPNG_VER > 88
   pp = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
   if (pp == NULL) {
      fclose(fp);
      return FALSE;
   }
   linfo = png_create_info_struct(pp);
   if (linfo == NULL) {
      fclose(fp);
      png_destroy_read_struct(&pp,NULL,NULL);
      return FALSE;
   }
#else
   pp = (png_structp)g_malloc(sizeof(png_struct));
   png_read_init(pp);
   linfo = (png_infop)g_malloc(sizeof(png_info));
#endif /* PNG_LIBPNG_VER > 88 */

   png_set_error_fn(pp, NULL, (png_error_ptr)my_error_exit, (png_error_ptr)png_skip_warning);

   if (setjmp (jerr.setjmp_buffer))
   {
   #if PNG_LIBPNG_VER > 88
      png_destroy_read_struct(&pp, &linfo, NULL);
   #else
      g_free(linfo);
      g_free(pp);
   #endif /* PNG_LIBPNG_VER > 88 */
      fclose(fp);
      return FALSE;
   }

   png_init_io(pp, fp);
   png_read_info(pp, linfo);

   info->width       = linfo->width;
   info->height      = linfo->height;
   info->valid       = linfo->valid;
   info->color_type  = linfo->color_type;

   g_free(linfo);
   g_free(pp);
   fclose(fp);

   return TRUE;
}

gboolean
png_load(gchar *filename, PngLoadFunc func)
{
   gint        bpp,              /* Bytes per pixel */
               num_passes,       /* Number of interlace passes in file */
               pass,             /* Current pass in file */
               scanline;         /* Beginning tile row */
   FILE        *fp;              /* File pointer */
   png_structp pp;               /* PNG read pointer */
   png_infop   info;             /* PNG info pointers */
   guchar      *pixel  = NULL;   /* Pixel data */

   if ((fp=fopen(filename,"rb")) == NULL)
      return FALSE;

#if PNG_LIBPNG_VER > 88
   pp = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
   if (pp == NULL) {
      fclose(fp);
      return FALSE;
   }
   info = png_create_info_struct(pp);
   if (info == NULL) {
      fclose(fp);
      png_destroy_read_struct(&pp,NULL,NULL);
      return FALSE;
   }
#else
   pp = (png_structp)g_malloc(sizeof(png_struct));
   png_read_init(pp);
   info = (png_infop)g_malloc(sizeof(png_info));
#endif /* PNG_LIBPNG_VER > 88 */

   png_set_error_fn(pp, NULL, (png_error_ptr)my_error_exit, (png_error_ptr)png_skip_warning);

   if (setjmp (jerr.setjmp_buffer))
   {
   #if PNG_LIBPNG_VER > 88
      png_destroy_read_struct(&pp, &info, NULL);
   #else
      g_free(info);
      g_free(pp);
   #endif /* PNG_LIBPNG_VER > 88 */
      fclose(fp);
      return FALSE;
   }

   png_init_io(pp, fp);
   png_read_info(pp, info);

   if (info->bit_depth < 8)
   {
      png_set_packing(pp);
      png_set_expand(pp);

      if (info->valid & PNG_INFO_sBIT)
         png_set_shift(pp, &(info->sig_bit));
   } else
   if (info->bit_depth == 16)
      png_set_strip_16(pp);

   /*
    * Turn on interlace handling...
    */
   if (info->interlace_type)
      num_passes = png_set_interlace_handling(pp);
   else
      num_passes = 1;

   switch (info->color_type)
   {
      case PNG_COLOR_TYPE_RGB :           /* RGB */
         bpp = 3;
         break;
      case PNG_COLOR_TYPE_RGB_ALPHA :     /* RGBA */
         bpp = 4;
         break;
      case PNG_COLOR_TYPE_GRAY :          /* Grayscale */
         bpp = 1;
         break;
      case PNG_COLOR_TYPE_GRAY_ALPHA :    /* Grayscale + alpha */
         bpp = 2;
         break;
      case PNG_COLOR_TYPE_PALETTE :       /* Indexed */
         bpp = info->num_trans ? 4:3;
         break;
   };

   pixel = g_malloc(sizeof(guchar) * info->width * bpp);

   for (pass = 0; pass < num_passes; pass++)
   {
      for (scanline = 0; scanline < info->height; scanline++)
      {
         if (info->color_type == PNG_COLOR_TYPE_PALETTE)
            png_set_expand(pp);

            png_read_row(pp, pixel, NULL);

         if ((*func) (pixel, info->width, 0, scanline, bpp, -1, 0)) goto png_read_cancelled;
      };
   };

   png_read_cancelled:
   png_read_end(pp, info);
#if PNG_LIBPNG_VER > 88
   png_destroy_read_struct (&pp, &info, NULL);
#else
   png_read_destroy(pp, info, NULL);
#endif /* PNG_LIBPNG_VER > 88 */

   g_free(pixel);
   g_free(pp);
   g_free(info);

   fclose(fp);

   return TRUE;
}

#endif /* HAVE_LIBPNG */
