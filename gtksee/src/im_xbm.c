/*
 * GTK See -- a image viewer based on GTK+
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

/* These codes are mostly taken from:
 * X10 and X11 bitmap (XBM) loading and saving file filter for the GIMP.
 * XBM code Copyright (C) 1998 Gordon Matzigkeit
 *
 * 2003-09-02: Taken code from Zgv (C) Russell Marks
 *             Added control of file type
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <gtk/gtk.h>

#include "im_xbm.h"

#define XBM_BLACK (0x00)
#define XBM_WHITE (0xff)

gint     xbm_getval  (gint c, gint base);
gint     xbm_fgetc   (FILE *fp);
gboolean xbm_match   (FILE *fp, guchar *s);
gboolean xbm_get_int (FILE *fp, gint *val);

/* Begin the code */

gboolean
xbm_get_header(gchar *filename, xbm_info *info)
{
   FILE     *fp;
   guchar   buffer[512], *p;

   if ((fp = fopen(filename, "r")) == NULL)
   {
      return FALSE;
   }

   if (fgets(buffer, sizeof(buffer), fp) == NULL )
   {
      fclose(fp);
      return FALSE;
   }

   /* Check for comments */
   if (strncmp(buffer,"/*", 2) == 0)
   {
      fgets(buffer, sizeof(buffer), fp);
   }

   if (strncmp(buffer,"#define ", 8)   != 0     ||
         (p = strstr(buffer,"_width")) == NULL  ||
         (info->width = atoi(p + 6))   == 0)
   {
      fclose(fp);
      return FALSE;
   }

   if (fgets(buffer, sizeof(buffer), fp)== NULL ||
         strncmp(buffer,"#define ", 8)  != 0    ||
         (p = strstr(buffer,"_height")) == NULL ||
         (info->height = atoi(p + 7))   == 0)
   {
      fclose(fp);
      return FALSE;
   }

   fclose(fp);
   return TRUE;
}

gboolean
xbm_load(gchar *filename, XbmLoadFunc func)
{
   FILE     *fp;
   guchar   buffer[128], *p;
   gint     width, height, intbits = 0;
   gint     c, i, j;
   guchar   *data;


   if ((fp = fopen(filename, "r")) == NULL)
   {
      return FALSE;
   }

   if (fgets(buffer, sizeof(buffer), fp) == NULL )
   {
      fclose(fp);
      return FALSE;
   }

   /* Check for comments */
   if (strncmp(buffer,"/*", 2) == 0)
   {
      fgets(buffer, sizeof(buffer), fp);
   }

   if (strncmp(buffer,"#define ", 8)   != 0     ||
         (p = strstr(buffer,"_width")) == NULL  ||
         (width = atoi(p + 6))   == 0)
   {
      fclose(fp);
      return FALSE;
   }

   if (fgets(buffer, sizeof(buffer), fp)== NULL ||
         strncmp(buffer,"#define ", 8)  != 0    ||
         (p = strstr(buffer,"_height")) == NULL ||
         (height = atoi(p + 7))   == 0)
   {
      fclose(fp);
      return FALSE;
   }

   fgets(buffer, sizeof(buffer), fp);

   if (strstr(buffer,"_x_hot") != NULL)
   {
      fgets(buffer, sizeof(buffer), fp);
      fgets(buffer, sizeof(buffer), fp);
   }

   if (strstr(buffer," char ") != NULL)      /* XBM X11 format */
   {
      intbits = 8;
   }
   else
      if (strstr(buffer," short ") != NULL)  /* XBM X10 format */
      {
         intbits = 16;
      }
      else
      {
         fclose(fp);
         return FALSE;
      }

   data = g_malloc (width * sizeof(guchar));

   for (i = 0; i < height; i++)
   {
      for (j = 0; j < width; j++)
      {
         if (j % intbits == 0)
         {
            if (!xbm_get_int(fp, &c))
            {
               g_free(data);
               fclose(fp);
               return FALSE;
            }
         }
         data[j] = (c & 1) ? XBM_BLACK : XBM_WHITE;
         c >>= 1;
      }
      if ((*func) (data, width, 0, i, 1, -1, 0)) break;
   }

   g_free(data);
   fclose(fp);
   return TRUE;
}

gint
xbm_getval (gint c, gint base)
{
   static guchar *digits = "0123456789abcdefABCDEF";
   gint val;

   if (base == 16) base = 22;

   for (val = 0; val < base; val ++)
   {
      if (c == digits[val])
         return (val < 16) ? val : (val - 6);
   }
   return -1;
}

/* Same as fgetc, but skip C-style comments and insert whitespace. */
gint
xbm_fgetc(FILE *fp)
{
   gint comment, c;

   /* FIXME: insert whitespace as advertised. */
   comment = 0;
   do
   {
      c = fgetc (fp);
      if (comment)
      {
         if (c == '*')
            comment = 1;
         else if (comment == 1 && c == '/')
            comment = 0;
         else
            comment = 2;
      } else
      {
         if (c == '/')
         {
            c = fgetc (fp);
            if (c == '*')
            {
               comment = 2;
            } else
            {
               ungetc (c, fp);
               c = '/';
            }
         }
      }
   } while (comment && c != EOF);

   return c;
}

/* Match a string with a file. */
gboolean
xbm_match(FILE *fp, guchar *s)
{
   gint c;

   do
   {
      c = fgetc (fp);
      if (c == *s)
         s++;
      else
         break;
   } while (c != EOF && *s);

   if (!*s) return TRUE;

   if (c != EOF) ungetc (c, fp);
   return FALSE;
}

/* Read the next integer from the file, skipping all non-integers. */
gboolean
xbm_get_int(FILE *fp, gint *val)
{
   gint digval, base, c;

   do
   {
      c = xbm_fgetc(fp);
   } while (c != EOF && !isdigit (c));

   if (c == EOF) return FALSE;

   if (c == '0')
   {
      c = fgetc (fp);
      if (c == 'x' || c == 'X')
      {
         c = fgetc (fp);
         base = 16;
      } else
      if (isdigit (c))
      {
         base = 8;
      } else
      {
         ungetc (c, fp);
         return FALSE;
      }
   } else
   {
      base = 10;
   }

   *val = 0;
   for (;;)
   {
      digval = xbm_getval(c, base);
      if (digval < 0)
      {
         ungetc (c, fp);
         break;
      }
      *val *= base;
      *val += digval;
      c = fgetc (fp);
   }

   return TRUE;
}
