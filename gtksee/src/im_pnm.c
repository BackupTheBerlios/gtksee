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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include "im_pnm.h"

gboolean
pnm_get_header(gchar *filename, pnm_info *info)
{
	FILE *file;
	guchar buffer[256];
	guchar type;

	file = fopen(filename, "r");
	if (file == NULL) return FALSE;
    
	if (!fgets(buffer, 255, file))
	{
		fclose(file);
		return FALSE;
	}
	
	if (buffer[0] != 'P' || (buffer[1] < '1' && buffer[1] > '6'))
	{
		fclose(file);
		return FALSE;
	}
	
	type = buffer[1];
	
	while (TRUE) {
		if (!fgets(buffer, 255, file))
		{
			fclose(file);
			return FALSE;
		}
		
		if (buffer[0]!='#') break;
	}
	
	if (sscanf(buffer, "%i %i", &info->width, &info->height) != 2)
	{
		fclose(file);
		return FALSE;
	}
	
	switch (type)
	{
	  case '1':
	  case '4':
		info -> ncolors = 1;
		break;
	  case '2':
	  case '5':
		info -> ncolors = 8;
		break;
	  case '3':
	  case '6':
		info -> ncolors = 24;
		break;
	  default: /* never reach */
		fclose(file);
		return FALSE;
	}
	
	return TRUE;
}

gboolean
pnm_load(gchar *filename, PnmLoadFunc func)
{
	FILE *file;
	guchar buffer[256];
	guchar type;
	gint width, height, max;
	gint i, j, c;
	guchar *dest, curbyte;

	file = fopen(filename, "r");
	if (file == NULL) return FALSE;
    
	if (!fgets(buffer, 255, file))
	{
		fclose(file);
		return FALSE;
	}
	
	if (buffer[0] != 'P' || (buffer[1] < '1' && buffer[1] > '6'))
	{
		fclose(file);
		return FALSE;
	}
	
	type = buffer[1];
	
	while (TRUE) {
		if (!fgets(buffer, 255, file))
		{
			fclose(file);
			return FALSE;
		}
		
		if (buffer[0]!='#') break;
	}
	
	if (sscanf(buffer, "%i %i", &width, &height) != 2)
	{
		fclose(file);
		return FALSE;
	}
	
	if (!fgets(buffer, 255, file))
	{
		fclose(file);
		return FALSE;
	}
	
	if (sscanf(buffer, "%i", &max) != 1 || max < 1)
	{
		fclose(file);
		return FALSE;
	}

	switch (type)
	{
	  case '2': /* ascii gray */
		dest = g_malloc(sizeof(guchar) * width);
		for (i = 0; i < height; i++)
		{
			for (j = 0; j < width; j++)
			{
				if (!fgets(buffer, 255, file)) { i = height; break; }
				if (sscanf(buffer, "%i", &c) != 1) { i = height; break; }
				dest[j] = (guchar)c;
			}
			if (i >= height) break;
			if ((*func) (dest, width, 0, i, 1, -1, 0)) break;
		}
		g_free(dest);
		break;
	  case '3': /* ascii rgb */
		dest = g_malloc(sizeof(guchar) * width * 3);
		for (i = 0; i< height; i++)
		{
			for (j = 0; j < width * 3; j++)
			{
				if (!fgets(buffer, 255, file)) { i = height; break; }
				if (sscanf(buffer, "%i", &c) != 1) { i = height; break; }
				dest[j] = (guchar)c;
			}
			if (i >= height) break;
			if ((*func) (dest, width, 0, i, 3, -1, 0)) break;
		}
		g_free(dest);
		break;
	  case '5': /* raw gray */
		dest = g_malloc(sizeof(guchar) * width);
		for (i = 0; i < height; i++)
		{
			if (!fread(dest, width, 1, file)) break;
			if ((*func) (dest, width, 0, i, 1, -1, 0)) break;
		}
		g_free(dest);
		break;
	  case '6': /* raw rgb */
		dest = g_malloc(sizeof(guchar) * width * 3);
		for (i = 0; i< height; i++)
		{
			if (!fread(dest, width * 3, 1, file)) break;
			if ((*func) (dest, width, 0, i, 3, -1, 0)) break;
		}
		g_free(dest);
		break;
	  case '1': /* ascii bitmap */
		dest = g_malloc(sizeof(guchar) * width);
		for (i = 0; i < height; i++)
		{
			for (j = 0; j < width; j++)
			{
				if (!fgets(buffer, 255, file)) { i = height; break; }
				dest[j] = (buffer[0] == '0') ? 0xff : 0x00;
			}
			if (i >= height) break;
			if ((*func) (dest, width, 0, i, 1, -1, 0)) break;
		}
		g_free(dest);
		break;
	  case '4': /* raw bitmap */
		dest = g_malloc(sizeof(guchar) * width);
		for (i = 0; i < height; i++)
		{
			for (j = 0; j < width; j++)
			{
				if (j % 8 == 0)
				{
					if ((c = fgetc(file)) == EOF) { i = height; break; }
					curbyte = (guchar)c;
				}
				dest[j] = (curbyte & 0x80) ? 0x00 : 0xff;
				curbyte <<= 1;
			}
			if (i >= height) break;
			if ((*func) (dest, width, 0, i, 1, -1, 0)) break;
		}
		g_free(dest);
		break;
	  default:
		fclose(file);
		return FALSE;
	}
	
	fclose(file);
	return TRUE;
}
