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

#ifndef __IM_EPS_H__
#define __IM_EPS_H__

#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

#include <gtk/gtk.h>

typedef struct _eps_info
{
  guint     resolution;    /* resolution (dpi) at which to run ghostscript */
  guint     width, height; /* desired size (ghostscript may ignore this) */
  gint      pnm_type;      /* 4: pbm, 5: pgm, 6: ppm, 7: automatic */
  gint      textalpha;     /* antialiasing: 1,2, or 4 TextAlphaBits */
  gint      graphicsalpha; /* antialiasing: 1,2, or 4 GraphicsAlphaBits */
  gboolean  is_epsf;
} eps_info;

typedef  gboolean (*EpsLoadFunc)    (guchar *buffer, gint width, gint left, gint scanline, gint components, gint pass, gint mode);

gboolean eps_get_header             (gchar *filename, eps_info *info);
gboolean eps_load                   (gchar *filename, EpsLoadFunc func);

#ifdef __cplusplus
   }
#endif /* __cplusplus */

#endif

