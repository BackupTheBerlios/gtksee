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
 * The GDK library in GTK+ has a well support for xpm images.
 * But GDK xpm support cannot read a header information ONLY.
 * So I wrote this simple function. As you can see, the codes
 * are mostly copy/paste from gdkpixmap.c
 */

/* 
 * 2003-09-02: Added control of file type
 * 
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include "gtypes.h"
#include "im_xpm.h"

gint  xpm_seek_string (FILE  *fp, const gchar *str, gint skip_comments);
gint  xpm_read_string (FILE  *fp, gchar **buffer, guint *buffer_size);
gint  xpm_seek_char   (FILE  *fp, gchar  c);

gchar* xpm_extract_color    (gchar *buffer);
gchar* xpm_skip_whitespaces (gchar *buffer);
gchar* xpm_skip_string      (gchar *buffer);

gboolean
xpm_get_header(gchar *filename, xpm_info *info)
{
   FILE *fp;
   gchar *buffer = NULL;
   gchar buf[512];
   gint width, height, ncols, cpp, n;
   guint buffer_size = 0;

   fp = fopen (filename, "r");
   if (fp == NULL) return FALSE;

   if(fgets(buf,sizeof(buf),fp)==NULL ||
         strncmp(buf, "/* XPM */", 9)!=0)
   {
      fclose(fp);
      return FALSE;
   }

/*
   if (!xpm_seek_string (fp, "XPM", FALSE))
   {
      fclose(fp);
      return FALSE;
   }
*/
   if (!xpm_seek_char (fp,'{'))
   {
      fclose(fp);
      return FALSE;
   }

   xpm_seek_char (fp, '"');
   fseek (fp, -1, SEEK_CUR);
   xpm_read_string (fp, &buffer, &buffer_size);

   n = sscanf (buffer,"%d %d %d %d", &width, &height, &ncols, &cpp);

   /* we have read all information that we need.
    * So close the file first.
    */
   fclose(fp);

   if (n < 4 || cpp >= 32) return FALSE;

   info -> width = width;
   info -> height = height;
   info -> ncolors = ncols;

   g_free(buffer);

   return TRUE;
}

gchar*
xpm_skip_whitespaces (gchar *buffer)
{
  gint32 index = 0;

  while (buffer[index] != 0 && (buffer[index] == 0x20 || buffer[index] == 0x09))
    index++;

  return &buffer[index];
}

gchar*
xpm_skip_string (gchar *buffer)
{
  gint32 index = 0;

  while (buffer[index] != 0 && buffer[index] != 0x20 && buffer[index] != 0x09)
    index++;

  return &buffer[index];
}

gint
xpm_seek_string (FILE  *fp,
                        const gchar *str,
                        gint   skip_comments)
{
  char instr[1024];

  while (!feof (fp))
    {
      fscanf (fp, "%1023s", instr);
      if (skip_comments == TRUE && strcmp (instr, "/*") == 0)
        {
          fscanf (fp, "%1023s", instr);
          while (!feof (fp) && strcmp (instr, "*/") != 0)
            fscanf (fp, "%1023s", instr);
          fscanf(fp, "%1023s", instr);
        }
      if (strcmp (instr, str)==0)
        return TRUE;
    }

  return FALSE;
}

gint
xpm_seek_char (FILE  *fp,
                      gchar  c)
{
  gint b, oldb;

  while ((b = getc(fp)) != EOF)
    {
      if (c != b && b == '/')
        {
          b = getc (fp);
          if (b == EOF)
            return FALSE;
          else if (b == '*')    /* we have a comment */
            {
              b = -1;
              do
                {
                  oldb = b;
                  b = getc (fp);
                  if (b == EOF)
                    return FALSE;
                }
              while (!(oldb == '*' && b == '/'));
            }
        }
      else if (c == b)
        return TRUE;
    }
  return FALSE;
}

gint
xpm_read_string (FILE  *fp,
                        gchar **buffer,
                        guint *buffer_size)
{
  gint c;
  guint cnt = 0;

  if ((*buffer) == NULL)
    {
      (*buffer_size) = 10 * sizeof (gchar);
      (*buffer) = g_new(gchar, *buffer_size);
    }

  do
    c = getc (fp);
  while (c != EOF && c != '"');

  if (c != '"')
    return FALSE;

  while ((c = getc(fp)) != EOF)
    {
      if (cnt == (*buffer_size))
        {
          guint new_size = (*buffer_size) * 2;
          if (new_size > (*buffer_size))
            *buffer_size = new_size;
          else
            return FALSE;

          (*buffer) = (gchar *) g_realloc ((*buffer), *buffer_size);
        }

      if (c != '"')
        (*buffer)[cnt++] = c;
      else
        {
          (*buffer)[cnt++] = 0;
          return TRUE;
        }
    }

  return FALSE;
}

/* Xlib crashed ince at a color name lengths around 125 */
#define MAX_COLOR_LEN 120

gchar*
xpm_extract_color (gchar *buffer)
{
  gint counter, numnames;
  gchar *ptr = NULL, ch, temp[128];
  gchar color[MAX_COLOR_LEN], *retcol;
  gint space;

  counter = 0;
  while (ptr == NULL)
    {
      if (buffer[counter] == 'c')
        {
          ch = buffer[counter + 1];
          if (ch == 0x20 || ch == 0x09)
            ptr = &buffer[counter + 1];
        }
      else if (buffer[counter] == 0)
        return NULL;

      counter++;
    }

  ptr = xpm_skip_whitespaces (ptr);

  if (ptr[0] == 0)
    return NULL;
  else if (ptr[0] == '#')
    {
      retcol = g_strdup (ptr);
      return retcol;
    }

  color[0] = 0;
  numnames = 0;

  space = MAX_COLOR_LEN - 1;
  while (space > 0)
    {
      sscanf (ptr, "%127s", temp);

      if (((gint)ptr[0] == 0) ||
     (strcmp ("s", temp) == 0) || (strcmp ("m", temp) == 0) ||
          (strcmp ("g", temp) == 0) || (strcmp ("g4", temp) == 0))
   {
     break;
   }
      else
        {
          if (numnames > 0)
       {
         space -= 1;
         strcat (color, " ");
       }
     if (space > 0)
       {
         strncat (color, temp, space);
         space -= min(space, strlen (temp));
       }
          ptr = xpm_skip_string (ptr);
          ptr = xpm_skip_whitespaces (ptr);
          numnames++;
        }
    }

  retcol = g_strdup (color);
  return retcol;
}

gboolean
xpm_load(gchar *filename, XpmLoadFunc func)
{
   FILE *fp = NULL;
   gchar **color_names;
   XpmColormap colors;
   gint transparent = -1;
   guint *rowdata;
   GdkColor gdkcolor;
   gint width, height, num_cols, cpp, cnt, n, ns, xcnt, ycnt;
   gchar *buffer = NULL, pixel_str[32];
   guint buffer_size = 0;
   gulong index;
   gchar *color_name;

   fp = fopen (filename, "r");
   if (fp == NULL) return FALSE;

   if (!xpm_seek_string (fp, "XPM", FALSE))
   {
      fclose(fp);
      return FALSE;
   }

   if (!xpm_seek_char (fp,'{'))
   {
      fclose(fp);
      return FALSE;
   }

   xpm_seek_char (fp, '"');
   fseek (fp, -1, SEEK_CUR);
   xpm_read_string (fp, &buffer, &buffer_size);

   n = sscanf (buffer,"%d %d %d %d", &width, &height, &num_cols, &cpp);

   if (n < 4 || cpp >= 32)
   {
      fclose(fp);
      return FALSE;
   }

   g_free(buffer);
   buffer = NULL;

   colors[0] = g_malloc(sizeof(guchar) * num_cols);
   colors[1] = g_malloc(sizeof(guchar) * num_cols);
   colors[2] = g_malloc(sizeof(guchar) * num_cols);
   color_names = g_malloc(sizeof(gchar *) * num_cols);

   for (cnt = 0; cnt < num_cols; cnt++)
   {
      xpm_seek_char (fp, '"');
      fseek (fp, -1, SEEK_CUR);
      xpm_read_string (fp, &buffer, &buffer_size);

      color_names[cnt] = g_malloc(sizeof(gchar) * cpp + 1);
      strncpy(color_names[cnt], buffer, cpp);
      color_names[cnt][cpp] = '\0';

      color_name = xpm_extract_color (&buffer[cpp]);

      if (color_name != NULL)
      {
         if (gdk_color_parse (color_name, &gdkcolor) == FALSE)
         {
            transparent = cnt;
         } else
               {
            colors[0][cnt] = gdkcolor.red >> 8;
            colors[1][cnt] = gdkcolor.green >> 8;
            colors[2][cnt] = gdkcolor.blue >> 8;
         }
         g_free (color_name);
      }
      g_free (buffer);
      buffer = NULL;
   }

   index = 0;
   rowdata = g_malloc(sizeof(guint) * width);

   for (ycnt = 0; ycnt < height; ycnt++)
   {
      xpm_read_string (fp, &buffer, &buffer_size);

      for (n = 0, cnt = 0, xcnt = 0; n < (width * cpp); n += cpp, xcnt++)
      {
         strncpy (pixel_str, &buffer[n], cpp);
         pixel_str[cpp] = '\0';
         ns = 0;

         while (ns < num_cols)
         {
            if (strcmp (pixel_str, color_names[ns]) == 0)
               break;
            else
               ns++;
         }

         if (ns >= num_cols) /* screwed up XPM file */
            ns = transparent;

         rowdata[xcnt] = ns;
      }
      g_free (buffer);
      buffer = NULL;
      if ((*func) (rowdata, colors, transparent, width, ycnt, 0)) break;
   }

   fclose (fp);

   for (n = 0; n < num_cols; n++) g_free(color_names[n]);
   g_free(color_names);
   g_free(colors[0]);
   g_free(colors[1]);
   g_free(colors[2]);
   g_free(rowdata);
   return TRUE;
}
