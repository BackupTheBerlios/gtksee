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
 * This loader uses code taken from:
 * GIF loading and saving filter for the GIMP
 */

#include "config.h"

#define MAX_COMMENT_SIZE 30

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <gtk/gtk.h>

#include "im_gif.h"

typedef struct
{
  gint interlace;
  gint loop;
  gint default_delay;
  gint default_dispose;
} GIFSaveVals;

#define CM_RED           0
#define CM_GREEN         1
#define CM_BLUE          2

#define MAX_LWZ_BITS     12

#define INTERLACE          0x40
#define LOCALCOLORMAP      0x80
#define BitSet(byte, bit)  (((byte) & (bit)) == (bit))

#define ReadOK(file,buffer,len) (fread(buffer, len, 1, file) != 0)
#define LM_to_uint(a,b)         (((b)<<8)|(a))

typedef struct gif89_info_struct
{
  gint transparent;
  gint delayTime;
  gint inputFlag;
  gint disposal;
  gchar *comment;
} gif89_info;

gint ReadColorMap (FILE *, gint, GifColormap);
gint FreeColorMap (GifColormap);
gint DoExtension (FILE *, gint, gif89_info *);
gint GetDataBlock (FILE *, guchar *);
gint GetCode (FILE *, gint, gint);
gint LWZReadByte (FILE *, gint, gint);
gint32 ReadImage (FILE *, gint, gint, GifColormap, gif89_info *,
	gint, gint, gint,
	guint, guint, guint, guint, GifLoadFunc);

gboolean
gif_get_header(gchar *filename, gif_screen *screen)
{
	guchar buf[16];
	guchar version[4];
	FILE *fd;
	
	if (filename == NULL) return FALSE;
	
	fd = fopen(filename, "rb");
	
	if (fd == NULL) return FALSE;
	
	if (!ReadOK (fd, buf, 6))
	{
		fclose(fd);
		return FALSE;
	}

	if (strncmp ((char *) buf, "GIF", 3) != 0)
	{
		fclose(fd);
		return FALSE;
	}

	strncpy (version, (char *) buf + 3, 3);
	version[3] = '\0';

	if ((strcmp (version, "87a") != 0) && (strcmp (version, "89a") != 0))
	{
		fclose(fd);
		return FALSE;
	}
	if (!ReadOK (fd, buf, 7))
	{
		fclose(fd);
		return FALSE;
	}

	screen->Width = LM_to_uint (buf[0], buf[1]);
	screen->Height = LM_to_uint (buf[2], buf[3]);
	screen->BitPixel = 2 << (buf[4] & 0x07);
	screen->ColorResolution = (((buf[4] & 0x70) >> 3) + 1);
	screen->Background = buf[5];
	screen->AspectRatio = buf[6];
	
	fclose(fd);
	
	if (screen->Width == 0 || screen->Height == 0) return FALSE;
	
	return TRUE;
}

gchar*
gif_load(gchar *filename, GifLoadFunc func)
{
	guchar buf[16];
	guchar c;
	FILE *fd;
	GifColormap localColorMap;
	gint useGlobalColormap;
	gint bitPixel;
	gint imageCount = 0;
	gchar version[4];
	gif_screen GifScreen;
	gif89_info Gif89={-1, -1, -1, 0, NULL};
	
	if (filename == NULL) return NULL;
	
	fd = fopen(filename, "rb");
	
	if (fd == NULL) return NULL;
	
	if (!ReadOK (fd, buf, 6))
	{
		fclose(fd);
		return NULL;
	}

	if (strncmp ((char *) buf, "GIF", 3) != 0)
	{
		fclose(fd);
		return NULL;
	}

	strncpy (version, (char *) buf + 3, 3);
	version[3] = '\0';

	if ((strcmp (version, "87a") != 0) && (strcmp (version, "89a") != 0))
	{
		fclose(fd);
		return NULL;
	}
	if (!ReadOK (fd, buf, 7))
	{
		fclose(fd);
		return NULL;
	}

	GifScreen.Width = LM_to_uint (buf[0], buf[1]);
	GifScreen.Height = LM_to_uint (buf[2], buf[3]);
	GifScreen.BitPixel = 2 << (buf[4] & 0x07);
	GifScreen.ColorResolution = (((buf[4] & 0x70) >> 3) + 1);
	GifScreen.Background = buf[5];
	GifScreen.AspectRatio = buf[6];
	GifScreen.ColorMap[0] = g_malloc(sizeof(guchar) * GifScreen.BitPixel);
	GifScreen.ColorMap[1] = g_malloc(sizeof(guchar) * GifScreen.BitPixel);
	GifScreen.ColorMap[2] = g_malloc(sizeof(guchar) * GifScreen.BitPixel);
	
	if (BitSet(buf[4], LOCALCOLORMAP))
	{
		/* Global Colormap */
		if (ReadColorMap(
			fd, GifScreen.BitPixel,
			GifScreen.ColorMap))
		{
			FreeColorMap(GifScreen.ColorMap);
			fclose(fd);
			return NULL;
		}
	}

	for (;;)
	{
		if (!ReadOK (fd, &c, 1)) break;
		if (c == ';') break;
		if (c == '!')
		{
			/* Extension */
			if (!ReadOK (fd, &c, 1))
			{
				FreeColorMap(GifScreen.ColorMap);
				fclose(fd);
				return Gif89.comment;
			}
			DoExtension (fd, c, &Gif89);
			continue;
		}
		if (c != ',')
		{
			/* Not a valid start character */
			continue;
		}

		++imageCount;

		if (!ReadOK (fd, buf, 9)) break;

		useGlobalColormap = !BitSet (buf[8], LOCALCOLORMAP);

		bitPixel = 1 << ((buf[8] & 0x07) + 1);

		if (!useGlobalColormap)
		{
			localColorMap[0] = g_malloc(sizeof(guchar) * bitPixel);
			localColorMap[1] = g_malloc(sizeof(guchar) * bitPixel);
			localColorMap[2] = g_malloc(sizeof(guchar) * bitPixel);
			if (ReadColorMap (fd, bitPixel, localColorMap))
			{
				FreeColorMap(localColorMap);
				break;
			}
			ReadImage (fd, LM_to_uint (buf[4], buf[5]),
				LM_to_uint (buf[6], buf[7]),
				localColorMap, &Gif89,
				bitPixel,
				BitSet (buf[8], INTERLACE), imageCount,
				(guint) LM_to_uint (buf[0], buf[1]),
				(guint) LM_to_uint (buf[2], buf[3]),
				GifScreen.Width,
				GifScreen.Height,
				func);
			FreeColorMap(localColorMap);
		} else
		{
			ReadImage (fd, LM_to_uint (buf[4], buf[5]),
				LM_to_uint (buf[6], buf[7]),
				GifScreen.ColorMap, &Gif89,
				GifScreen.BitPixel,
				BitSet (buf[8], INTERLACE), imageCount,
				(guint) LM_to_uint (buf[0], buf[1]),
				(guint) LM_to_uint (buf[2], buf[3]),
				GifScreen.Width,
				GifScreen.Height,
				func);
		}
		/* we only need one image.
		 * I still have not decided to add animation gif support.
		 * So break here.
		 */
		break;
	}
	
	FreeColorMap(GifScreen.ColorMap);
	fclose(fd);
	return Gif89.comment;
}

gint
ReadColorMap (
	FILE *fd,
	gint   number,
	GifColormap buffer)
{
	gint i;
	guchar rgb[3];
	
	for (i = 0; i < number; ++i)
	{
		if (!ReadOK (fd, rgb, sizeof (rgb)))
		{
			return TRUE;
		}

		buffer[CM_RED][i] = (guchar)rgb[0];
		buffer[CM_GREEN][i] = (guchar)rgb[1];
		buffer[CM_BLUE][i] = (guchar)rgb[2];
	}

	return FALSE;
}

gint
FreeColorMap(GifColormap cmap)
{
	g_free(cmap[0]);
	g_free(cmap[1]);
	g_free(cmap[2]);
	return FALSE;
}

gint
DoExtension (FILE *fd,
	     int   label,
	     gif89_info *Gif89)
{
	guchar buf[256];
	gint count;

	switch (label)
	{
		case 0x01:			/* Plain Text Extension */
			break;
		case 0xff:			/* Application Extension */
			break;
		case 0xfe:			/* Comment Extension */
			while ((count = GetDataBlock (fd, buf)) != 0)
			{
				if (Gif89->comment == NULL)
				{
					count = (count<MAX_COMMENT_SIZE)?count:MAX_COMMENT_SIZE;
					Gif89->comment = g_malloc(sizeof(gchar) * (count + 1));
					strncpy(Gif89->comment, buf, count);
					Gif89->comment[count] = '\0';
				}
			}
			return FALSE;
		case 0xf9:			/* Graphic Control Extension */
			(void) GetDataBlock (fd, buf);
			Gif89->disposal = (buf[0] >> 2) & 0x7;
			Gif89->inputFlag = (buf[0] >> 1) & 0x1;
			Gif89->delayTime = LM_to_uint (buf[1], buf[2]);
			if ((buf[0] & 0x1) != 0)
				Gif89->transparent = buf[3];
			else
				Gif89->transparent = -1;
			break;
		default:
			break;
	}

	while (GetDataBlock (fd, NULL) > 0)
		;

	return FALSE;
}

gint ZeroDataBlock;

gint
GetDataBlock (FILE	*fd,
	      guchar	*buf)
{
	guchar count;
	
	if (!ReadOK (fd, &count, 1))
	{
		return -1;
	}
	
	ZeroDataBlock = count == 0;
	
	if (count == 0) return 0;
	if (buf != NULL)
	{
		if (!ReadOK (fd, buf, count))
		{
			return -1;
		}
	} else
	{
		fseek(fd, count, SEEK_CUR);
	}
	
	return count;
}

gint
GetCode (FILE *fd,
	 gint   code_size,
	 gint   flag)
{
	static guchar buf[280];
	static gint curbit, lastbit, done, last_byte;
	gint i, j, ret, count;

	if (flag)
	{
		curbit = 0;
		lastbit = 0;
		done = FALSE;
		return 0;
	}

	if ((curbit + code_size) >= lastbit)
	{
		if (done)
		{
			/*if (curbit >= lastbit)
			{
				return -1;
			}*/
			return -1;
		}
		buf[0] = buf[last_byte - 2];
		buf[1] = buf[last_byte - 1];
		
		if ((count = GetDataBlock (fd, &buf[2])) == 0)
			done = TRUE;
		if (count < 0) return -1;
		last_byte = 2 + count;
		curbit = (curbit - lastbit) + 16;
		lastbit = (2 + count) * 8;
	}
	
	ret = 0;
	for (i = curbit, j = 0; j < code_size; ++i, ++j)
		ret |= ((buf[i / 8] & (1 << (i % 8))) != 0) << j;
	
	curbit += code_size;

	return ret;
}

int
LWZReadByte (FILE *fd,
	     int   flag,
	     int   input_code_size)
{
	static gint fresh = FALSE;
	gint code, incode;
	static gint code_size, set_code_size;
	static gint max_code, max_code_size;
	static gint firstcode, oldcode, temp;
	static gint clear_code, end_code;
	static gint table[2][(1 << MAX_LWZ_BITS)];
	static gint stack[(1 << (MAX_LWZ_BITS)) * 2], *sp;
	register gint i;
	
	if (flag)
	{
		set_code_size = input_code_size;
		code_size = set_code_size + 1;
		clear_code = 1 << set_code_size;
		end_code = clear_code + 1;
		max_code_size = 2 * clear_code;
		max_code = clear_code + 2;
		
		if (GetCode (fd, 0, TRUE) < 0) return -1;
		
		fresh = TRUE;
		
		for (i = 0; i < clear_code; ++i)
		{
			table[0][i] = 0;
			table[1][i] = i;
		}
		/*for (; i < (1 << MAX_LWZ_BITS); ++i)
			table[0][i] = table[1][i] = 0;*/
		sp = stack;
		
		return 0;
	}
	else if (fresh)
	{
		fresh = FALSE;
		do
		{
			if ((temp = GetCode (fd, code_size, FALSE)) < 0) return -1;
			firstcode = oldcode = temp;
		} while (firstcode == clear_code);
		return firstcode;
	}
	
	if (sp > stack)
		return *--sp;
	
	while ((code = GetCode (fd, code_size, FALSE)) >= 0)
	{
		if (code == clear_code)
		{
			for (i = 0; i < clear_code; ++i)
			{
				table[0][i] = 0;
				table[1][i] = i;
			}
			/* is this require? */
			/*for (; i < (1 << MAX_LWZ_BITS); ++i)
				table[0][i] = table[1][i] = 0;*/
			code_size = set_code_size + 1;
			max_code_size = 2 * clear_code;
			max_code = clear_code + 2;
			sp = stack;
			if ((temp = GetCode (fd, code_size, FALSE)) < 0) return -1;
			firstcode = oldcode = temp;
			return firstcode;
		}
		else if (code == end_code)
		{
			gint count;
			if (ZeroDataBlock)
				return -2;
			
			while ((count = GetDataBlock (fd, NULL)) > 0)
				;
			
			return -2;
		}
		
		incode = code;
		
		if (code >= max_code)
		{
			*sp++ = firstcode;
			code = oldcode;
		}
		
		while (code >= clear_code && sp < stack + ((1<<MAX_LWZ_BITS)*2-1))
		{
			*sp++ = table[1][code];
			if (code == table[0][code])
			{
				return -1;
			}
			code = table[0][code];
		}
		
		*sp++ = firstcode = table[1][code];
		
		if ((code = max_code) < (1 << MAX_LWZ_BITS))
		{
			table[0][code] = oldcode;
			table[1][code] = firstcode;
			++max_code;
			if ((max_code >= max_code_size) &&
				(max_code_size < (1 << MAX_LWZ_BITS)))
			{
				max_code_size *= 2;
				++code_size;
			}
		}
		oldcode = incode;
		if (sp > stack)
			return *--sp;
	}
	return code;
}

gint32
ReadImage (FILE *fd,
	   gint   len,
	   gint   height,
	   GifColormap  cmap,
	   gif89_info *Gif89,
	   gint   ncols,
	   gint   interlace,
	   gint   number,
	   guint   leftpos,
	   guint   toppos,
	   guint screenwidth,
	   guint screenheight,
	   GifLoadFunc func)
{
	guint *dest, *temp;
	guchar c;
	gint xpos = 0, ypos = 0, pass = 0;
	gint v;
	gboolean alpha_frame = FALSE;
	guchar   highest_used_index;

	/*
	 **  Initialize the Compression routines
	 */
	if (!ReadOK (fd, &c, 1))
	{
		return -1;
	}
	if (LWZReadByte (fd, TRUE, c) < 0)
	{
		return -1;
	}
	
	if (number != 1)
	{
		alpha_frame = TRUE;
	}
	
	if (alpha_frame)
	{
		dest = (guint *) g_malloc (sizeof(guint) * len * 2);
	} else
	{
		dest = (guint *) g_malloc (sizeof(guint) * len);
	}
	
	while ((v = LWZReadByte (fd, FALSE, c)) >= 0)
	{
		if (alpha_frame)
		{
			if (((guchar)v > highest_used_index) && !(v == Gif89->transparent))
				highest_used_index = (guchar)v;
			temp = &dest[xpos << 1];
			temp[0] = (guint) v;
			temp[1] = (guint) ((v == Gif89->transparent) ? 0 : 255);
		} else
		{
			if ((guchar)v > highest_used_index)
				highest_used_index = (guchar)v;
			temp = &dest[xpos];
			temp[0] = (guint) v;
		}
		
		++xpos;
		if (xpos == len)
		{
			/* Send one row here! */
			if ((*func) (dest, cmap, Gif89->transparent, len, ypos, number)) break;
			
			xpos = 0;
			if (interlace)
			{
				switch (pass)
				{
				case 0:
				case 1:
					ypos += 8;
					break;
				case 2:
					ypos += 4;
					break;
				case 3:
					ypos += 2;
					break;
				}
				if (ypos >= height)
				{
					++pass;
					switch (pass)
					{
						case 1:
							ypos = 4;
							break;
						case 2:
							ypos = 2;
							break;
						case 3:
							ypos = 1;
							break;
						default:
							goto fini;
					}
				}
			} else
			{
				++ypos;
			}
		}
		if (ypos >= height)
			break;
	}
	
	fini:
	
		if (LWZReadByte (fd, FALSE, c) >= 0)
			;
		
		g_free (dest);
	
	return 0;
}
