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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "scale.h"

#define EPSILON            0.0001

void
scale_region (guchar *srcPR,
      gint orig_width, gint orig_height,
      gint max_width , gint max_height,
      gint bytes     , ScaleFunc func)
{
   guchar   * src_m1, * src, * src_p1, * src_p2;
   guchar   * s_m1, * s, * s_p1, * s_p2;
   guchar   * dest, * d;
   gdouble  * row, * r;
   gint     src_row, src_col;
   gint     b;
   gint     width, height;
   gdouble  x_rat, y_rat;
   gdouble  x_cum, y_cum;
   gdouble  x_last, y_last;
   gdouble  * x_frac, y_frac, tot_frac;
   gfloat   dx, dy;
   gint     i;
   register gint j;
   gint     frac;
   gint     advance_dest_x, advance_dest_y;
   gint     minus_x, plus_x, plus2_x;

   if (max_width * orig_height < max_height * orig_width)
   {
      width = max_width;
      height = width * orig_height / orig_width;
   } else
   {
      height = max_height;
      width = height * orig_width / orig_height;
   }

   /*  the data pointers...  */
   dest   = (guchar *) g_malloc (width * bytes);

   /*  find the ratios of old x to new x and old y to new y  */
   x_rat = (gdouble) orig_width / (gdouble) width;
   y_rat = (gdouble) orig_height / (gdouble) height;

   /*  allocate an array to help with the calculations  */
   row    = (gdouble *) g_malloc (sizeof (gdouble) * width * bytes);
   x_frac = (gdouble *) g_malloc (sizeof (gdouble) * (width + orig_width));

   /*  initialize the pre-calculated pixel fraction array  */
   src_col = 0;
   x_cum = (gdouble) src_col;
   x_last = x_cum;

   for (i = 0; i < width + orig_width; i++)
   {
      if (x_cum + x_rat <= (src_col + 1 + EPSILON))
      {
         x_cum += x_rat;
         x_frac[i] = x_cum - x_last;
      } else
      {
         src_col ++;
         x_frac[i] = src_col - x_last;
      }
      x_last += x_frac[i];
   }

   /*  clear the "row" array  */
   memset (row, 0, sizeof (gdouble) * width * bytes);

   /*  counters...  */
   src_row = 0;
   y_cum = (gdouble) src_row;
   y_last = y_cum;

   /*  Get the first src row  */
   src = srcPR;
   /*  Get the next two if possible  */
   if (src_row < (orig_height - 1))
   {
      src_p1 = srcPR + orig_width * bytes;
   }

   if ((src_row + 1) < (orig_height - 1))
   {
      src_p2 = srcPR + orig_width * bytes * 2;
   }

   /*  Scale the selected region  */
   i = height;
   while (i)
   {
      src_col = 0;
      x_cum = (gdouble) src_col;

      /* determine the fraction of the src pixel we are using for y */
      if (y_cum + y_rat <= (src_row + 1 + EPSILON))
      {
         y_cum += y_rat;
         dy = y_cum - src_row;
         y_frac = y_cum - y_last;
         advance_dest_y = TRUE;
      } else
      {
         y_frac = (src_row + 1) - y_last;
         dy = 1.0;
         advance_dest_y = FALSE;
      }

      y_last += y_frac;

      s = src;
      s_m1 = (src_row > 0) ? src_m1 : src;
      s_p1 = (src_row < (orig_height - 1)) ? src_p1 : src;
      s_p2 = ((src_row + 1) < (orig_height - 1)) ? src_p2 : s_p1;

      r = row;

      frac = 0;
      j = width;

      while (j)
      {
         if (x_cum + x_rat <= (src_col + 1 + EPSILON))
         {
            x_cum += x_rat;
            dx = x_cum - src_col;
            advance_dest_x = TRUE;
         } else
         {
            dx = 1.0;
            advance_dest_x = FALSE;
         }

         tot_frac = x_frac[frac++] * y_frac;

         minus_x = (src_col > 0) ? -bytes : 0;
         plus_x = (src_col < (orig_width - 1)) ? bytes : 0;
         plus2_x = ((src_col + 1) < (orig_width - 1)) ? bytes * 2 : plus_x;

         for (b = 0; b < bytes; b++)
         {
            r[b] += s[b] * tot_frac;
         }

         if (advance_dest_x)
         {
            r += bytes;
            j--;
         } else
         {
            s_m1 += bytes;
            s    += bytes;
            s_p1 += bytes;
            s_p2 += bytes;
            src_col++;
         }
      }

      if (advance_dest_y)
      {
         tot_frac = 1.0 / (x_rat * y_rat);

         /*  copy "row" to "dest"  */
         d = dest;
         r = row;

         j = width;
         while (j--)
         {
            b = bytes;
            while (b--)
            {
               *d++ = (guchar) (*r++ * tot_frac);
            }
         }

         /*  set the pixel region span  */
         if ((*func) (dest, width, 0, (height - i), bytes, -1, 0)) break;

         /*  clear the "row" array  */
         memset (row, 0, sizeof (gdouble) * width * bytes);

         i--;
      } else
      {
         /*  Shuffle pointers  */
         s = src_m1;
         src_m1 = src;
         src = src_p1;
         src_p1 = src_p2;
         src_p2 = s;

         src_row++;
         if ((src_row + 1) < (orig_height - 1))
            src_p2 = srcPR + orig_width * (src_row + 2) * bytes;
      }
   }

   /*  free up temporary arrays  */
   g_free (row);
   g_free (x_frac);
   g_free (dest);
}

void
scale_region_preview(guchar *srcPR,
            gint orig_width, gint orig_height,
            gint max_width , gint max_height,
            gint bytes     , ScaleFunc func)
{
   guchar *row, *tmp1, *tmp2, *dest;
   gint width, height, x, y;
   gint len;
   register gint i;

   if (max_width * orig_height < max_height * orig_width)
   {
      width = max_width;
      height = width * orig_height / orig_width;
   } else
   {
      height = max_height;
      width = height * orig_width / orig_height;
   }
   len = orig_width * bytes;
   row = g_malloc(width * bytes * sizeof(guchar));
   for (y = 0; y < height; y++)
   {
      tmp1 = &srcPR[(y * orig_height / height) * len];
      for (x = 0; x < width; x++)
      {
         tmp2 = &tmp1[(x * orig_width / width) * bytes];
         dest = &row[x * bytes];
         for (i = 0; i < bytes; i++)
         {
            (*dest++) = (*tmp2++);
         }
      }
      if ((*func) (row, width, 0, y, bytes, -1, 0)) break;
   }
   g_free(row);
}
