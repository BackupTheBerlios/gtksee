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
#include "im_pcx.h"

typedef struct {
    guint8 manufacturer;
    guint8 version;
    guint8 compression;
    guint8 bpp;
    gint16 x1, y1;
    gint16 x2, y2;
    gint16 hdpi;
    gint16 vdpi;
    guint8 colormap[48];
    guint8 reserved;
    guint8 planes;
    gint16 bytesperline;
    gint16 color;
    guint8 filler[58];
} pcx_header_struct;

gboolean	pcx_readline	(FILE *fd,
				 guchar* buffer,
				 gint bytes,
				 guint8 compression);

gboolean
pcx_get_header(gchar *filename, pcx_info *info)
{
	FILE *fd;
	pcx_header_struct pcx_header;
	
	fd = fopen (filename, "rb");
	if (fd == NULL)
	{
		return FALSE;
	}

	if (fread(&pcx_header, 128, 1, fd) == 0)
	{
		fclose(fd);
		return FALSE;
	}

	if(pcx_header.manufacturer != 10)
	{
		fclose(fd);
		return FALSE;
	}
	
	info->width = pcx_header.x2 - pcx_header.x1 + 1;
	info->height = pcx_header.y2 - pcx_header.y1 + 1;
	if (pcx_header.planes == 1 && pcx_header.bpp == 1)
	{
		info->ncolors = 1;
	} else
	if (pcx_header.planes == 4 && pcx_header.bpp == 1)
	{
		info->ncolors = 4;
	} else
	if (pcx_header.planes == 1 && pcx_header.bpp == 8)
	{
		info->ncolors = 8;
	} else
	if (pcx_header.planes == 3 && pcx_header.bpp == 8)
	{
		info->ncolors = 24;
	} else
	{
		fclose(fd);
		return FALSE;
	}
	
	fclose(fd);
	return TRUE;
}

gboolean
pcx_load(gchar *filename, PcxLoadFunc func)
{
	FILE *fd;
	pcx_header_struct pcx_header;
	gint width, height, bytes, x, y, c, t1, t2, pix, ptr;
	guchar *dest, *line, *line0, *line1, *line2, *line3;
	guchar cmap[768];
	
	fd = fopen (filename, "rb");
	if (fd == NULL)
	{
		return FALSE;
	}

	if (fread(&pcx_header, 128, 1, fd) == 0)
	{
		fclose(fd);
		return FALSE;
	}

	if(pcx_header.manufacturer != 10)
	{
		fclose(fd);
		return FALSE;
	}
	
	width = pcx_header.x2 - pcx_header.x1 + 1;
	height = pcx_header.y2 - pcx_header.y1 + 1;
	bytes = pcx_header.bytesperline;
	
	if (pcx_header.planes == 1 && pcx_header.bpp == 1)
	{
		/* loading 1-bpp pcx */
		dest = (guchar *) g_malloc (width * 3 * sizeof(guchar));
		line = (guchar *) g_malloc (bytes * sizeof(guchar));
		for (y = 0; y < height; y++)
		{
			if (!pcx_readline(fd, line, bytes, pcx_header.compression)) break;
			for (x = 0, ptr = 0; x < width; x++)
			{
				if (line[x / 8] & (128 >> (x % 8)))
				{
					dest[ptr++]= 0xff;
					dest[ptr++]= 0xff;
					dest[ptr++]= 0xff;
				} else
				{
					dest[ptr++]= 0;
					dest[ptr++]= 0;
					dest[ptr++]= 0;
				}
			}
			if ((*func) (dest, width, 0, y, 3, -1, 0)) break;
		}
		g_free(line);	
		g_free(dest);
	} else
	if (pcx_header.planes == 4 && pcx_header.bpp == 1)
	{
		/* loading 4-bpp pcx */
		dest = (guchar *) g_malloc (width * 3 * sizeof(guchar));
		line0 = (guchar *) g_malloc (bytes * sizeof(guchar));
		line1 = (guchar *) g_malloc (bytes * sizeof(guchar));
		line2 = (guchar *) g_malloc (bytes * sizeof(guchar));
		line3 = (guchar *) g_malloc (bytes * sizeof(guchar));
		for (y = 0; y < height; y++)
		{
			if (!pcx_readline(fd, line0, bytes, pcx_header.compression)) break;
			if (!pcx_readline(fd, line1, bytes, pcx_header.compression)) break;
			if (!pcx_readline(fd, line2, bytes, pcx_header.compression)) break;
			if (!pcx_readline(fd, line3, bytes, pcx_header.compression)) break;
			for (x = 0, ptr = 0; x < width; x++)
			{
				t1 = x / 8;
				t2 = (128 >> (x % 8));
				pix = (((line0[t1] & t2) == 0) ? 0 : 1) +
					(((line1[t1] & t2) == 0) ? 0 : 2) +
					(((line2[t1] & t2) == 0) ? 0 : 4) +
					(((line3[t1] & t2) == 0) ? 0 : 8);
				pix *= 3;
				dest[ptr++] = pcx_header.colormap[pix];
				dest[ptr++] = pcx_header.colormap[pix + 1];
				dest[ptr++] = pcx_header.colormap[pix + 2];
			}
			if ((*func) (dest, width, 0, y, 3, -1, 0)) break;
		}
		g_free(line0);
		g_free(line1);
		g_free(line2);
		g_free(line3);
		g_free(dest);
	} else
	if (pcx_header.planes == 1 && pcx_header.bpp == 8)
	{
		/* loading 8-bpp pcx */
		dest = (guchar *) g_malloc (width * 3 * sizeof(guchar));
		line = (guchar *) g_malloc (bytes * sizeof(guchar));
		fseek(fd, -768L, SEEK_END);
		if (fread(cmap, 768, 1, fd) == 0) return FALSE;
		fseek(fd, 128, SEEK_SET);
		for (y = 0; y < height; y++)
		{
			if (!pcx_readline(fd, line, bytes, pcx_header.compression)) break;
			for (x = 0, ptr = 0; x < width; x++)
			{
				pix = line[x] * 3;
				dest[ptr++] = cmap[pix];
				dest[ptr++] = cmap[pix + 1];
				dest[ptr++] = cmap[pix + 2];
			}
			if ((*func) (dest, width, 0, y, 3, -1, 0)) break;
		}
		g_free(line);
		g_free(dest);
	} else
	if (pcx_header.planes == 3 && pcx_header.bpp == 8)
	{
		/* loading 24-bpp pcx(rgb) */
		dest = (guchar *) g_malloc (width * 3 * sizeof(guchar));
		line = (guchar *) g_malloc(bytes * 3 * sizeof(guchar));
		for (y = 0; y < height; y++)
		{
			for (c= 0; c < 3; c++)
			{
				if (!pcx_readline(fd, line, bytes, pcx_header.compression)) break;
				for (x= 0; x < width; x++)
				{
					dest[x * 3 + c] = line[x];
				}
			}
			if ((*func) (dest, width, 0, y, 3, -1, 0)) break;
		}
		g_free(line);
		g_free(dest);
	} else {
		fclose(fd);
		return FALSE;
	}

	fclose(fd);
	return TRUE;
}

gboolean
pcx_readline(FILE *fd, guchar *buffer, gint bytes, guint8 compression)
{
	guchar count = 0;
	gint value = 0;

	if (compression)
	{
		while (bytes--)
		{
			if (count == 0)
			{
				if ((value = fgetc(fd)) == EOF) return FALSE;
				if (value < 0xc0)
				{
					count= 1;
				} else
				{
					count= value - 0xc0;
					if ((value = fgetc(fd)) == EOF) return FALSE;
				}
			}
			count--;
			*(buffer++)= (guchar)value;
		}
	} else {
		if (fread(buffer, bytes, 1, fd) == 0) return FALSE;
	}
	return TRUE;
}
