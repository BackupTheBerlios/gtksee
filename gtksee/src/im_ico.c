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
 * Many codes are taken from:
 * ICON plug-in for the GIMP
 * And transparent support for 16-color icon was added
 */
 
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "im_ico.h"

#define MAXCOLORS       256

#define BitSet(byte, bit)  (((byte) & (bit)) == (bit))

typedef struct {
	gshort idReserved;  /* always set to 0 */
	gshort idType;      /* always set to 1 */
	gshort idCount;     /* number of icon images */
	/* immediately followed by idCount TIconDirEntries */
} ICO_Header;

typedef struct {
	guchar bWidth;             /* Width */
	guchar bHeight;            /* Height */
	guint bColorCount;        /* # of colors used, see below */
	guchar bReserved;          /* not used, 0 */
	gushort wPlanes;        /* not used, 0 */
	gushort wBitCount;      /* not used, 0 */
	gulong dwBytesInRes;   /* total number of bytes in image */
	gulong dwImageOffset;  /* location of image from the beginning of file */
	gint bad_colors;
} ICO_TIconDirEntry;

typedef struct {
	glong biSize;      /* sizeof(TBitmapInfoHeader) */
	glong biWidth;     /* width of bitmap */
	glong biHeight;            /* height of bitmap, see notes */
	gshort biPlanes;           /* planes, always 1 */
	gshort biBitCount;         /* number of color bits */
	glong biCompression;    /* compression used, 0 */
	glong biSizeImage;         /* size of the pixel data, see notes */
	glong biXPelsPerMeter;  /* not used, 0 */
	glong biYPelsPerMeter;  /* not used, 0 */
	glong biClrUsed;           /* # of colors used, set to 0 */
	glong biClrImportant;   /* important colors, set to 0 */
} ICO_TBitmapInfoHeader;

typedef struct {
	ICO_TBitmapInfoHeader icHeader; /* image header info */
	guchar *icColors[3];         /* image palette */
} ICO_IconImage;

gint	ICO_read_iconheader	(FILE *fp, ICO_Header *header);
gint	ICO_read_direntry	(FILE *fp,
				 ICO_TIconDirEntry *direntry);
gint	ICO_read_bitmapinfo	(FILE *fp,
				 ICO_IconImage *iconimage,
				 ICO_TIconDirEntry *direntry);
gint	ICO_read_rgbquad	(FILE *fp,
				 ICO_IconImage *iconimage,
				 ICO_TIconDirEntry *direntry);

gboolean	ico_get_header	(gchar *filename, ico_info *info)
{
	FILE *fp;
	ICO_Header ICO_header;
	ICO_TIconDirEntry ICO_direntry;
	ICO_IconImage ICO_iconimage;
	guint i;
	
	fp = fopen(filename, "rb");
	if (fp == NULL) return FALSE;

	if (ICO_read_iconheader(fp, &ICO_header) ||
	    ICO_header.idReserved != 0 ||
	    ICO_header.idType != 1)
	{
		fclose(fp);
		return FALSE;
	}

	if (ICO_read_direntry(fp, &ICO_direntry))
	{
		fclose(fp);
		return FALSE;
	}
	
	info -> width = (gint)ICO_direntry.bWidth;
	info -> height = (gint)ICO_direntry.bHeight;

	if (ICO_direntry.bad_colors)
	{
		fseek(fp, ICO_direntry.dwImageOffset, SEEK_SET);
		if (ICO_read_bitmapinfo(fp, &ICO_iconimage, &ICO_direntry))
		{
			fclose(fp);
			return FALSE;
		}
		info -> ncolors = ICO_iconimage.icHeader.biBitCount;
	} else
	{
		info -> ncolors = 0;
		for (i = ICO_direntry.bColorCount; i > 1; i >>= 1, info->ncolors++) ;
	}
	fclose(fp);
	return TRUE;
}

gboolean	ico_load	(gchar *filename, IcoLoadFunc func)
{
	FILE *fp;
	ICO_Header ICO_header;
	ICO_TIconDirEntry ICO_direntry;
	ICO_IconImage ICO_iconimage;
	gint xormasklen, andmasklen, xornum;
	gint b;
	gint i, j, k, t, width, height, idx;
	guint transparent;
	guchar *buf,*icAND;
	guint *icXOR, *p;
	
	fp = fopen(filename, "rb");
	if (fp == NULL) return FALSE;

	if (ICO_read_iconheader(fp, &ICO_header))
	{
		fclose(fp);
		return FALSE;
	}

	if (ICO_read_direntry(fp, &ICO_direntry))
	{
		fclose(fp);
		return FALSE;
	}
	
	/*for (i=1; i<ICO_header.idCount; i++)
	{
		if (ICO_read_direntry(fp, &temp_direntry)
		{
			fclose(fp);
			return FALSE;
		}
	}*/

	fseek(fp, ICO_direntry.dwImageOffset, SEEK_SET);

	if (ICO_read_bitmapinfo(fp, &ICO_iconimage, &ICO_direntry))
	{
		fclose(fp);
		return FALSE;
	}

	if (ICO_read_rgbquad(fp, &ICO_iconimage, &ICO_direntry))
	{
		fclose(fp);
		return FALSE;
	}

	height = ICO_direntry.bHeight;
	width = ICO_direntry.bWidth;

	transparent = ICO_direntry.bColorCount;
	xornum = width * height;
	xormasklen = xornum * ICO_iconimage.icHeader.biBitCount / 8;
	andmasklen = xornum / 8;

	icXOR = g_new(guint, xornum);
	icAND = g_new(guchar, xornum);

	buf = g_new(guchar, xormasklen);

	fread((void *)buf,1,xormasklen,fp);
	b=fread((void *)icAND,1,andmasklen,fp);
	if (b<andmasklen) {
		fclose(fp);
		g_free(ICO_iconimage.icColors[0]);
		g_free(ICO_iconimage.icColors[1]);
		g_free(ICO_iconimage.icColors[2]);
		g_free(icXOR);
		g_free(icAND);
		g_free(buf);
		return FALSE;
	}

	/* probably not the most efficient code, but it's fast, and
	 * since the icon's are small anyway, doesn't take too much
	 * memory
	 */
	for (j=0; j<height; j++)
	{
		p = &icXOR[(height-j-1)*width];
		idx = 0;
		for (i=0; i<width*ICO_iconimage.icHeader.biBitCount/8; i++)
		{
			k = buf[j*height*ICO_iconimage.icHeader.biBitCount/8+i];
			switch(ICO_iconimage.icHeader.biBitCount)
			{
				case 1:
					p[idx++]=BitSet(k,0x80);
					p[idx++]=BitSet(k,0x40);
					p[idx++]=BitSet(k,0x20);
					p[idx++]=BitSet(k,0x10);
					p[idx++]=BitSet(k,0x08);
					p[idx++]=BitSet(k,0x04);
					p[idx++]=BitSet(k,0x02);
					p[idx++]=BitSet(k,0x01);
					break;
				case 4:
					t = j*width+i*2;
					p[idx++]=BitSet(icAND[t/8],1<<(7-t%8))?
						transparent:((k>>4) & 0x0f);
					t++;
					p[idx++]=BitSet(icAND[t/8],1<<(7-t%8))?
						transparent:(k & 0x0f);
					break;
				case 8:
					/*t = j*width+i;
					*p++=BitSet(icAND[t/8],1<<(7-t%8))?
						transparent:k;*/
					p[idx++]=k;
					break;
				default:
					fclose(fp);
					g_free(ICO_iconimage.icColors[0]);
					g_free(ICO_iconimage.icColors[1]);
					g_free(ICO_iconimage.icColors[2]);
					g_free(icXOR);
					g_free(icAND);
					g_free(buf);
					return FALSE;
			}
		}
		if ((*func) (&icXOR[(height-j-1)*width], ICO_iconimage.icColors, transparent, width, height-j-1, 0)) break;
	}
	fclose(fp);
	g_free(ICO_iconimage.icColors[0]);
	g_free(ICO_iconimage.icColors[1]);
	g_free(ICO_iconimage.icColors[2]);
	g_free(icXOR);
	g_free(icAND);
	g_free(buf);
	return TRUE;
}

gint
ICO_read_iconheader(FILE *fp, ICO_Header *header)
{
	gushort i;

	/* read Reserved bit */
	if (fread((void *)&i,1,sizeof(i),fp) != sizeof(i))
		return(-1);
	header -> idReserved = i;

	/* read Resource Type, 1 is for icons, abort if different */
	if (fread((void *)&i,1,sizeof(i),fp) != sizeof(i))
		return(-1);
	header -> idType = i;
	if (header -> idType != 1)
		return(-1);

	/* read Number of images, abort if invalid value (0) */
	if (fread((void *)&i,1,sizeof(i),fp) != sizeof(i))
		return(-1);
	header -> idCount = i;              /* Number of images (>0) */
	if (header -> idCount==0)
		return(-1);
	
	return(0);
}

gint
ICO_read_direntry(FILE *fp, ICO_TIconDirEntry *direntry)
{
	gushort i;
	guchar c;
	gulong l;

	/* read Width, in pixels */
	if (fread((void *)&c,1,sizeof(c),fp) != sizeof(c))
		return(-1);
	direntry -> bWidth = c;

	/* read Height, in pixels */
	if (fread((void *)&c,1,sizeof(c),fp) != sizeof(c))
		return(-1);
	direntry -> bHeight = c;

	if (direntry -> bWidth < 1 || direntry -> bHeight < 1)
		return(-1);

	/* Number of colors in image */
	if (fread((void *)&c,1,sizeof(c),fp) != sizeof(c))
		return(-1);
	direntry -> bColorCount = c;
	direntry -> bad_colors = 0;
	if (direntry -> bColorCount==0)
	{
		direntry -> bad_colors = 1;
	}
	if (direntry -> bColorCount>256)
	{
		return(-1);
	}

	/* Reserved (must be 0) */
	if (fread((void *)&c,1,sizeof(c),fp) != sizeof(c))
		return(-1);
	direntry -> bReserved = c;
	
	/* Color Planes, not used, 0 */
	if (fread((void *)&i,1,sizeof(i),fp) != sizeof(i))
		return(-1);
	direntry -> wPlanes = i;

	/* Bits per pixel, not used, 0 */
	if (fread((void *)&i,1,sizeof(i),fp) != sizeof(i))
		return(-1);
	direntry -> wBitCount = i;

	/* size of image data, in bytes */
	if (fread((void *)&l,1,sizeof(l),fp) != sizeof(l))
		return(-1);
	direntry -> dwBytesInRes = l;
	if (direntry -> dwBytesInRes == 0)
		return(-1);

	/* offset from beginning image info */
	if (fread((void *)&l,1,sizeof(l),fp) != sizeof(l))
		return(-1);
	direntry -> dwImageOffset = l;
	if (direntry -> dwImageOffset == 0)
		return(-1);

	return(0);
}

gint
ICO_read_bitmapinfo(FILE *fp, ICO_IconImage *iconimage, ICO_TIconDirEntry *direntry)
{
	gulong l;
	gushort i;

	/* read bitmap info an perform some primitive sanity checks */

	/* sizeof(TBitmapInfoHeader) */
	if (fread((void *)&l,1,sizeof(l),fp) != sizeof(l))
		return(-1);
	iconimage -> icHeader.biSize = l;

	/* width of bitmap */
	if (fread((void *)&l,1,sizeof(l),fp) != sizeof(l))
		return(-1);
	iconimage -> icHeader.biWidth = l;

	/* height of bitmap, see notes (icon.h) */
	if (fread((void *)&l,1,sizeof(l),fp) != sizeof(l))
		return(-1);
	iconimage -> icHeader.biHeight = l;

	if (iconimage -> icHeader.biWidth < 1 || iconimage -> icHeader.biHeight < 1)
		return(-1);

	/* planes, always 1 */
	if (fread((void *)&i,1,sizeof(i),fp) != sizeof(i))
		return(-1);
	iconimage -> icHeader.biPlanes = i;

	/* number of color bits (2,4,8) */
	if (fread((void *)&i,1,sizeof(i),fp) != sizeof(i))
		return(-1);
	iconimage -> icHeader.biBitCount = i;
	if (direntry -> bad_colors)
	{
		if (iconimage -> icHeader.biBitCount == 8)
			direntry -> bColorCount = 256;
		else
			direntry -> bColorCount = 1 << (iconimage -> icHeader.biBitCount);
	}

	/* compression used, 0 */
	if (fread((void *)&l,1,sizeof(l),fp) != sizeof(l))
		return(-1);
	iconimage -> icHeader.biCompression = l;
	if (iconimage -> icHeader.biCompression != 0)
		return(-1);

	/* size of the pixel data, see icon.h */
	if (fread((void *)&l,1,sizeof(l),fp) != sizeof(l))
		return(-1);
	iconimage -> icHeader.biSizeImage = l;

  /* used to abort on this, but it's not really that important...
      should be the len of the XORMASK + ANDMASK...

      as the saying goes, be liberal in what you accept,
      and strict in what you output (or something to that effect 
    */

/*  if (ih.biSizeImage==0)
    return(-1); */

	/* not used, 0 */
	if (fread((void *)&l,1,sizeof(l),fp) != sizeof(l))
		return(-1);
	iconimage -> icHeader.biXPelsPerMeter = l;

	/* not used, 0 */
	if (fread((void *)&l,1,sizeof(l),fp) != sizeof(l))
		return(-1);
	iconimage -> icHeader.biYPelsPerMeter = l;

	/* # of colors used, set to 0 */
	if (fread((void *)&l,1,sizeof(l),fp) != sizeof(l))
		return(-1);
	iconimage -> icHeader.biClrUsed = l;

	/* important colors, set to 0 */
	if (fread((void *)&l,1,sizeof(l),fp) != sizeof(l))
		return(-1);
	iconimage -> icHeader.biClrImportant = l;

	return(0);
}

gint
ICO_read_rgbquad(FILE *fp, ICO_IconImage *iconimage, ICO_TIconDirEntry *direntry)
{
	gint j;
	guchar c;
	gint colors;

	colors = direntry -> bColorCount;
	iconimage->icColors[0] = g_malloc(sizeof(guchar) * colors);
	iconimage->icColors[1] = g_malloc(sizeof(guchar) * colors);
	iconimage->icColors[2] = g_malloc(sizeof(guchar) * colors);

	for (j=0; j<colors; j++)
	{
		if (fread((void *)&c,sizeof(c),1,fp) < 1)
			return(-1);
		iconimage->icColors[2][j] = c;

		if (fread((void *)&c,sizeof(c),1,fp) < 1) 
			return(-1);
		iconimage->icColors[1][j] = c;

		if (fread((void *)&c,sizeof(c),1,fp) < 1)
			return(-1);
		iconimage->icColors[0][j] = c;

		if (fread((void *)&c,sizeof(c),1,fp) < 1)
			return(-1);
	}

	/*if (colors==255)
	{
		gchar buf[10];
		fread((void *)&buf,4,1,fp);
	}*/
	return(0);
}
