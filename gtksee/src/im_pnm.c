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
 * 2003-09-08: Little modifications for correct reading
 *             of PBM, PGM and PPM files
 */
#include "config.h"

#include <stdio.h>
#include <ctype.h>

#include "im_pnm.h"

gint  read_next_number  (FILE *fp);

gboolean
pnm_get_header(gchar *filename, pnm_info *info)
{
   FILE     *fp;
   guchar   buffer[2];

   if ((fp = fopen(filename, "rb")) == NULL)
   {
      return FALSE;
   }

   buffer[0] = fgetc(fp);
   buffer[1] = fgetc(fp);

   if (buffer[0] != 'P' || (buffer[1] < '1' || buffer[1] > '7'))
   {
      fclose(fp);
      return FALSE;
   }

   /* Check if is a thumbnail ... */
   if (buffer[1] == '7')
   {
      if (read_next_number(fp) != 332)
      {
         fclose(fp);
         return FALSE;
      }
   }

   info->width   = read_next_number(fp);
   info->height  = read_next_number(fp);

   //if ((buffer[1] !='1') && (buffer[1] !='4') && (buffer[1] !='7'))
   if ((buffer[1] !='1') && (buffer[1] !='4'))
   {
      info->maxcolor = read_next_number(fp);
   }

   switch (buffer[1])
   {
      case '1':   /* PBM Bitmap - pixels are black or white */
      case '4':   info->ncolors = 1;
                  break;

      case '2':   /* PGM Bitmap - grayscale */
      case '5':   info->ncolors = 8;
                  break;

      case '3':   /* PPM, PNM or PAM - color */
      case '6':
      case '7':   info->ncolors = 24;
                  break;

      default:    return FALSE;
   }

   return TRUE;
}

gboolean
pnm_load(gchar *filename, PnmLoadFunc func)
{
   FILE     *fp;
   guchar   *dest, buffer[256], curbyte;
   gint     width, height, maxval, i, j, c;

   if ((fp = fopen(filename, "rb")) == NULL)
   {
      return FALSE;
   }

   buffer[0] = fgetc(fp);
   buffer[1] = fgetc(fp);

   if (buffer[0] != 'P' || (buffer[1] < '1' && buffer[1] > '7'))
   {
      fclose(fp);
      return FALSE;
   }

   /* Check if is a thumbnail ... */
   if (buffer[1] == '7')
   {
      if (read_next_number(fp) != 332)
      {
         fclose(fp);
         return FALSE;
      }
   }

   width = read_next_number(fp);
   height= read_next_number(fp);

   //if ((buffer[1] !='1') && (buffer[1] !='4') && (buffer[1] !='7'))
   if ((buffer[1] !='1') && (buffer[1] !='4'))
   {
      maxval = read_next_number(fp);
   }

   switch (buffer[1])
   {
      case '1': /* PBM ascii bitmap */
                  dest = g_malloc(sizeof(guchar) * width);
                  for (i = 0; i < height; i++)
                  {
                     for (j = 0; j < width; )
                     {
                        if ((c = fgetc(fp)) == EOF)
                        {
                           fclose(fp);
                           g_free(dest);
                           return FALSE;
                        }

                        if (c=='0' || c=='1')
                        {
                           dest[j] = (c == '0') ? 0xff : 0x00;
                           j++;
                        }
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

gint
read_next_number(FILE *fp)
{
   gint  c, num, in_num, gotnum;

   num   = 0;
   in_num= 0;
   gotnum= 0;

   do
   {
      if(feof(fp))
         return(-1);

      if((c=fgetc(fp))=='#')
      {
         while(((c=fgetc(fp)) != '\n') && (c!=EOF));
      } else
      {
         if(isdigit(c))
         {
            num = 10 * num + c - 49 + (in_num=1);
         } else
         {
            gotnum = in_num;
         }
      }
   } while(!gotnum);

   return(num);
}

