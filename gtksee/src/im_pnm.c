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

/*
 * 2003-09-08: Little modifications for correct reading
 *             of PBM, PGM and PPM files
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include "im_pnm.h"

gboolean
pnm_get_header(gchar *filename, pnm_info *info)
{
   FILE     *fp;
   guchar   buffer[256];
   guchar   type;

   if ((fp = fopen(filename, "r")) == NULL)
   {
      return FALSE;
   }

   if (!fgets(buffer, 255, fp))
   {
      fclose(fp);
      return FALSE;
   }

   if (buffer[0] != 'P' || (buffer[1] < '1' && buffer[1] > '7'))
   {
      fclose(fp);
      return FALSE;
   }

   type = buffer[1];

   while (TRUE) {
      if (!fgets(buffer, 255, fp))
      {
         fclose(fp);
         return FALSE;
      }

      if (buffer[0]!='#' && buffer[0]!='\n') break;
   }

   fclose(fp);

   if (sscanf(buffer, "%i %i", &info->width, &info->height) != 2)
   {
      return FALSE;
   }

   switch (type)
   {
      case '1':   /* PBM Bitmap - pixels are black or white */
      case '4':   info -> ncolors = 1;
                  break;
      case '7':
      case '2':   /* PGM Bitmap - grayscale */
      case '5':   info -> ncolors = 8;
                  break;

      case '3':   /* PPM or PNM - color */
      case '6':   info -> ncolors = 24;
                  break;

      default:    return FALSE;
   }

   return TRUE;
}

gboolean
pnm_load(gchar *filename, PnmLoadFunc func)
{
   FILE     *fp;
   guchar   *dest, buffer[256],
            type, curbyte;
   gint     width, height, max,
            i, j, c;

   if ((fp = fopen(filename, "r")) == NULL)
   {
      return FALSE;
   }

   if (!fgets(buffer, 255, fp))
   {
      fclose(fp);
      return FALSE;
   }

   if (buffer[0] != 'P' || (buffer[1] < '1' && buffer[1] > '7'))
   {
      fclose(fp);
      return FALSE;
   }

   type = buffer[1];

   while (TRUE) {
      if (!fgets(buffer, 255, fp))
      {
         fclose(fp);
         return FALSE;
      }

      if (buffer[0]!='#' && buffer[0]!='\n') break;
   }

   if (sscanf(buffer, "%i %i", &width, &height) != 2)
   {
      fclose(fp);
      return FALSE;
   }

   if (type!='1' && type !='4' && type !='7')
   {                          /* Maximum gray or color value */

      if ((!fgets(buffer, 255, fp)) ||
            (sscanf(buffer, "%i", &max) != 1 || max < 1))
      {
         fclose(fp);
         return FALSE;
      }
   }

   switch (type)
   {
      case '1': /* PBM ascii bitmap */
                  dest = g_malloc(sizeof(guchar) * width);
                  for (i = 0; i < height; i++)
                  {
                     for (j = 0; j < width; j++)
                     {
                        if (fscanf(fp, "%i", &c) != 1)
                        {
                           fclose(fp);
                           g_free(dest);
                           return FALSE;
                        }
                        dest[j] = (c == 0) ? 0xff : 0x00;
                     }
                     if ((*func) (dest, width, 0, i, 1, -1, 0)) break;
                  }
                  g_free(dest);
                  break;

      case '4': /* PBM raw bitmap */
                  dest = g_malloc(sizeof(guchar) * width);
                  for (i = 0; i < height; i++)
                  {
                     for (j = 0; j < width; j++)
                     {
                        if (j % 8 == 0)
                        {
                           if ((c = fgetc(fp)) == EOF)
                           {
                              fclose(fp);
                              g_free(dest);
                              return FALSE;
                           }
                           curbyte = (guchar)c;
                        }
                        dest[j] = (curbyte & 0x80) ? 0x00 : 0xff;
                        curbyte <<= 1;
                     }
                     if ((*func) (dest, width, 0, i, 1, -1, 0)) break;
                  }
                  g_free(dest);
                  break;

      case '2': /* PGM ascii gray */
                  dest = g_malloc(sizeof(guchar) * width);
                  for (i = 0; i < height; i++)
                  {
                     for (j = 0; j < width; j++)
                     {
                        if (fscanf(fp, "%i", &c) != 1)
                        {
                           fclose(fp);
                           g_free(dest);
                           return FALSE;
                        }
                        dest[j] = (guchar)c;
                     }
                     if ((*func) (dest, width, 0, i, 1, -1, 0)) break;
                  }
                  g_free(dest);
                  break;

      case '5': /* PGM raw gray */
                  dest = g_malloc(sizeof(guchar) * width);
                  for (i = 0; i < height; i++)
                  {
                     if (!fread(dest, width, 1, fp)) break;
                     if ((*func) (dest, width, 0, i, 1, -1, 0)) break;
                  }
                  g_free(dest);
                  break;

      case '3': /* PPM or PNM ascii rgb */
                  dest = g_malloc(sizeof(guchar) * width * 3);
                  for (i = 0; i< height; i++)
                  {
                     for (j = 0; j < width * 3; j++)
                     {
                        if (fscanf(fp, "%i", &c) != 1)
                        {
                           fclose(fp);
                           g_free(dest);
                           return FALSE;
                        }
                        dest[j] = (guchar)c;
                     }
                     if ((*func) (dest, width, 0, i, 3, -1, 0)) break;
                  }
                  g_free(dest);
                  break;

      case '6': /* PPM or PNM raw rgb */
                  dest = g_malloc(sizeof(guchar) * width * 3);
                  for (i = 0; i< height; i++)
                  {
                     if (!fread(dest, width * 3, 1, fp)) break;
                     if ((*func) (dest, width, 0, i, 3, -1, 0)) break;
                  }
                  g_free(dest);
                  break;

      case '7': /* PAM raw rgb */
                  dest = g_malloc(sizeof(guchar) * width * 3);
                  for (i = 0; i< height; i++)
                  {
                     for (j=0; j<width * 3; j++)
                     {
                        c         = fgetc(fp);
                        dest[j++] = (( c >>5 ) *255)/7;
                        dest[j++] = (((c >>2 ) &7)*255)/7;
                        dest[j]   = (((c )&3 ) *255)/3;
                     }

                     //if (!fread(dest, width, 1, fp)) break;
                     if ((*func) (dest, width, 0, i, 3, -1, 0)) break;
                  }
                  g_free(dest);
                  break;

      default:    fclose(fp);
                  return FALSE;
   }

   fclose(fp);
   return TRUE;
}
