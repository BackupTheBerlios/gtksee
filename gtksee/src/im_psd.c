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
 *   "Adobe's PSD format is not simple... Some effects in Photoshop
 * 4.0/5.0 may not be able to be displayed", said Hotaru.
 */
 
/*
 * Adobe and Adobe Photoshop are trademarks of Adobe Systems
 * Incorporated that may be registered in certain jurisdictions.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include "gtypes.h"
#include "im_psd.h"

/* #define DEBUG */

#define MAX_CHANNELS 30

typedef enum
{
	PSD_GRAY_IMAGE,
	PSD_INDEXED_IMAGE,
	PSD_RGB_IMAGE,
	PSD_RGBA_IMAGE,
	PSD_BITMAP_IMAGE,
	PSD_UNKNOWN_IMAGE
} PsdImageType;

typedef struct
{
	glong top;
	glong left;
	glong bottom;
	glong right;
	
	gint num_channels;
	gint channel_id[MAX_CHANNELS];
	glong channel_len[MAX_CHANNELS];
	
	gboolean has_mask;
	glong mask_top;
	glong mask_left;
	glong mask_bottom;
	glong mask_right;
	glong mask_throw_alpha;
	
	LayerMode mode;
	gint opacity;
	gint clipping;
	gboolean protecttrans;
	gboolean visible;
	gboolean paintonmask;
	gint recordfiller;
	glong rangeslen;
} PsdLayer;

typedef struct
{
	PsdImageType type;
	gboolean cmyk;
	guchar caption[256];
	gint channels;
	glong rows;
	glong columns;
	gint mode;
	glong cmaplen;
	guchar cmap[768];
	gint bpp;
	
	glong reslen;
	glong misclen;
	PsdLayer *layers;
	gint numlayers;
	gint curlayer;
	gint absolute_alpha;
	gint num_aux_channels;
} PsdImage;

/* return FALSE if failed */
gboolean	psd_get_short(FILE *fd, gint *val);
gboolean	psd_get_long(FILE *fd, glong *val);
gboolean	psd_get_string(FILE *fd, guchar *buf);

gboolean	psd_load_image_resource	(FILE *fd, PsdImage *image);
gboolean	psd_load_id_data	(FILE *fd, gint id, PsdImage *image);
gboolean	psd_load_misc_data	(FILE *fd, PsdImage *image, PsdLoadFunc func);
gboolean	psd_load_layer_record	(FILE *fd, PsdImage *image);
gboolean	psd_load_layer_pixeldata(FILE *fd, PsdImage *image, PsdLoadFunc func);
gboolean	psd_load_image_data	(FILE *fd, PsdImage *image, PsdLoadFunc func);
gboolean	psd_layer_has_alpha	(PsdLayer* layer);
void		psd_put_pixel_element	(PsdImage *image, guchar *line, gint x, gint off, gint ic);
LayerMode	psd_modekey_to_lmode	(guchar *modekey);

gboolean
psd_get_short(FILE *fd, gint *val)
{
	gint t;
	gint16 v;
	
	if ((v = fgetc(fd)) == EOF) return FALSE;
	if ((t = fgetc(fd)) == EOF) return FALSE;
	v = v<<8 | t;
	if (val != NULL)
	{
		*val = (gint) v;
	}
	return TRUE;
}

gboolean
psd_get_long(FILE *fd, glong *val)
{
	gint t;
	gint32 v;
	
	if ((v = fgetc(fd)) == EOF) return FALSE;
	if ((t = fgetc(fd)) == EOF) return FALSE;
	v = v<<8 | t;
	if ((t = fgetc(fd)) == EOF) return FALSE;
	v = v<<8 | t;
	if ((t = fgetc(fd)) == EOF) return FALSE;
	v = v<<8 | t;
	if (val != NULL)
	{
		*val = (glong) v;
	}
	return TRUE;
}

gboolean
psd_get_string(FILE *fd, guchar *buf)
{
	gint len;
	
	if ((len = fgetc(fd)) == EOF) return FALSE;
	if (buf == NULL)
	{
		/* we just skip the string */
		fseek(fd, max(len, 1), SEEK_CUR);
	} else
	{
		if (len > 0)
		{
			if (fread(buf, len, 1, fd) < 1) return FALSE;
		} else
		{
			fseek(fd, 1, SEEK_CUR);
		}
		buf[len] = '\0';
	}
	return TRUE;
}


gboolean
psd_get_header(gchar *filename, psd_info *info)
{
	FILE *fd;
	guchar buf[7];
	gint channels, mode, dummy;
	glong rows, columns;
	
	if ((fd = fopen(filename, "rb")) == NULL) return FALSE;
	/* signature */
	if (fread(buf, 4, 1, fd) < 1 || strncmp(buf, "8BPS", 4) != 0)
	{
		fclose(fd);
		return FALSE;
	}
	/* version */
	if (!psd_get_short(fd, &dummy) || dummy != 1)
	{
		fclose(fd);
		return FALSE;
	}
	/* reserved & channels */
	if (fread(buf, 6, 1, fd) < 1 || !psd_get_short(fd, &channels))
	{
		fclose(fd);
		return FALSE;
	}
	
	/* rows */
	if (!psd_get_long(fd, &rows))
	{
		fclose(fd);
		return FALSE;
	}
	info->height = (gint)rows;
	
	/* columns */
	if (!psd_get_long(fd, &columns))
	{
		fclose(fd);
		return FALSE;
	}
	info->width = columns;
	
	/* bpp */
	if (!psd_get_short(fd, &dummy) || (dummy != 8 && dummy != 1))
	{
		fclose(fd);
		return FALSE;
	}
	
	/* mode */
	if (!psd_get_short(fd, &mode))
	{
		fclose(fd);
		return FALSE;
	}
	
	switch (mode)
	{
	  case 0: /* bitmap */
		info->ncolors = 1;
		info->alpha = 0;
		break;
	  case 1: /* grayscale */
	  case 2: /* indexed */
		info->ncolors = 8;
		info->alpha = 0;
		break;
	  case 3: /* rgb */
		info->ncolors = 24;
		info->alpha = 0;
		break;
	  case 4: /* cmyk */
		switch (channels)
		{
		  case 4: /* rgb */
			info->ncolors = 24;
			info->alpha = 0;
			break;
		  case 5: /* rgba */
			info->ncolors = 24;
			info->alpha = 1;
			break;
		  default:
			fclose(fd);
			return FALSE;
		}
		break;
	  default:
		fclose(fd);
		return FALSE;
	}
	
	fclose(fd);
	return TRUE;
}

guchar*
psd_load(gchar *filename, PsdLoadFunc func)
{
	FILE *fd;
	guchar buf[7];
	PsdImage psd_image;
	gint dummy;
	glong pos;
	
	psd_image.numlayers = 0;
	psd_image.caption[0] = '\0';
	psd_image.num_aux_channels = 0;

#ifdef DEBUG
	g_print("\n*********************************************\n");
	g_print("  PSD File: %s\n",filename);
	g_print("\n*********************************************\n\n");
#endif
	
	if ((fd = fopen(filename, "rb")) == NULL) return NULL;
	/* signature */
	if (fread(buf, 4, 1, fd) < 1 || strncmp(buf, "8BPS", 4) != 0)
	{
		fclose(fd);
		return NULL;
	}
	/* version */
	if (!psd_get_short(fd, &dummy) || dummy != 1)
	{
		fclose(fd);
		return NULL;
	}
	/* reserved & channels */
	if (fread(buf, 6, 1, fd) < 1 || !psd_get_short(fd, &psd_image.channels))
	{
		fclose(fd);
		return NULL;
	}
	
	/* rows */
	if (!psd_get_long(fd, &psd_image.rows))
	{
		fclose(fd);
		return NULL;
	}
	
	/* columns */
	if (!psd_get_long(fd, &psd_image.columns))
	{
		fclose(fd);
		return NULL;
	}
	
	/* bpp */
	if (!psd_get_short(fd, &psd_image.bpp) || (psd_image.bpp != 8 && psd_image.bpp != 1))
	{
		fclose(fd);
		return NULL;
	}
	
	/* mode */
	if (!psd_get_short(fd, &psd_image.mode))
	{
		fclose(fd);
		return NULL;
	}
	
	switch (psd_image.mode)
	{
	  case 0: /* bitmap */
		psd_image.type = PSD_BITMAP_IMAGE;
		psd_image.cmyk = FALSE;
		break;
	  case 1: /* grayscale */
		psd_image.type = PSD_GRAY_IMAGE;
		psd_image.cmyk = FALSE;
		break;
	  case 2: /* indexed */
		psd_image.type = PSD_INDEXED_IMAGE;
		psd_image.cmyk = FALSE;
		break;
	  case 3: /* rgb */
		psd_image.type = PSD_RGB_IMAGE;
		psd_image.cmyk = FALSE;
		break;
	  case 4: /* cmyk */
		psd_image.cmyk = TRUE;
		switch (psd_image.channels)
		{
		  case 4: /* rgb */
			psd_image.type = PSD_RGB_IMAGE;
			break;
		  case 5: /* rgba */
			psd_image.type = PSD_RGBA_IMAGE;
			break;
		  default:
			fclose(fd);
			return FALSE;
		}
		break;
	  default:
		fclose(fd);
		return NULL;
	}

#ifdef DEBUG
	g_print("width=%li height=%li type=%i\n",psd_image.columns,psd_image.rows,psd_image.type);
#endif
	
	/* colormap length */
	if (!psd_get_long(fd, &psd_image.cmaplen))
	{
		fclose(fd);
		return NULL;
	}
	
#ifdef DEBUG
	g_print("colormap=%li\n",psd_image.cmaplen/3);
#endif

	/* load colormap if needed */
	if (psd_image.cmaplen > 0)
	{
		if (psd_image.cmaplen <= 768)
		{
			if (fread(psd_image.cmap, psd_image.cmaplen, 1, fd) < 1)
			{
				fclose(fd);
				return NULL;
			}
		} else
		{
			if (fread(psd_image.cmap, 768, 1, fd) < 1)
			{
				fclose(fd);
				return NULL;
			}
			fseek(fd, psd_image.cmaplen - 768, SEEK_CUR);
		}
	}
	
#ifdef DEBUG
	g_print("Loading image resource...\n");
#endif

	/* image resource */
	if (!psd_get_long(fd, &psd_image.reslen))
	{
		fclose(fd);
		return NULL;
	}
	pos = ftell(fd);
#ifdef DEBUG
	g_print("	pos: %li  len: %li\n",pos,psd_image.reslen);
#endif
	if (psd_image.reslen > 0 && !psd_load_image_resource(fd, &psd_image))
	{
		fclose(fd);
		return NULL;
	}
	fseek(fd, pos + psd_image.reslen, SEEK_SET);
	
#ifdef DEBUG
	g_print("Loading misc data...\n");
#endif

	/* misc data */
	if (!psd_get_long(fd, &psd_image.misclen))
	{
		fclose(fd);
		if (strlen(psd_image.caption) > 0)
			return g_strdup(psd_image.caption);
		else
			return NULL;
	}
	pos = ftell(fd);
#ifdef DEBUG
	g_print("	pos: %li  len: %li\n",pos,psd_image.misclen);
#endif
	if (psd_image.misclen > 0 && !psd_load_misc_data(fd, &psd_image, func))
	{
		fclose(fd);
		if (strlen(psd_image.caption) > 0)
			return g_strdup(psd_image.caption);
		else
			return NULL;
	}
	fseek(fd, pos + psd_image.misclen, SEEK_SET);
	
	if (psd_image.numlayers <= 0)
	{
		psd_load_image_data(fd, &psd_image, func);
	}

	fclose(fd);
	if (strlen(psd_image.caption) > 0)
		return g_strdup(psd_image.caption);

	return NULL;
}

gboolean
psd_load_image_resource(FILE *fd, PsdImage *image)
{
	guchar buf[256];
	glong pos, pos_in, cur, idlen;
	gint id;
	
	cur = pos = ftell(fd);
	
	while (cur < pos + image->reslen)
	{
		if (fread(buf, 4, 1, fd) < 1 || strncmp(buf, "8BIM", 4) != 0)
		{
			return TRUE;
		}
		if (!psd_get_short(fd, &id)) return TRUE;
		if (!psd_get_string(fd, buf)) return TRUE;
		if (!psd_get_long(fd, &idlen)) return TRUE;
#ifdef DEBUG
		g_print("	resource id: %i  name: %s  pos: %li  len: %li\n", id, buf, ftell(fd), idlen);
#endif
		if (idlen > 0)
		{
			pos_in = ftell(fd);
			if (!psd_load_id_data(fd, id, image)) return TRUE;
			fseek(fd, pos_in + idlen, SEEK_SET);
		}
		cur = ftell(fd);
	}
	return TRUE;
}

gboolean
psd_load_id_data(FILE *fd, gint id, PsdImage *image)
{
	switch (id)
	{
	  case 0x03f0: /* image caption */
		if (!psd_get_string(fd, image->caption)) return TRUE;
#ifdef DEBUG
		g_print("	Image caption: %s\n",image->caption);
#endif
		break;
	  case 0x03ee: /* aux channel name */
		image->num_aux_channels++;
		break;
	  default:
		break;
	}
	return TRUE;
}

gboolean
psd_load_misc_data(FILE *fd, PsdImage *image, PsdLoadFunc func)
{
	glong seclen, pos;
	gint16 t;
	
	if (!psd_get_long(fd, &seclen)) return FALSE;
	pos = ftell(fd);
#ifdef DEBUG
	g_print("pos: %li  section_len: %li\n", pos, seclen);
#endif
	if (!psd_get_short(fd, &image->numlayers)) return FALSE;
	if (image->numlayers == 0) return TRUE;
	
	/* converting numlayers */
	t = (gint16) image->numlayers;
	image->numlayers = abs(t);
	image->absolute_alpha = (t < 0);
	
#ifdef DEBUG
	g_print("layers=%i absolute_alpha=%i\n",image->numlayers,t<0);
	g_print("\n================================================\n\n");
#endif

	image->layers = g_malloc(sizeof(PsdLayer) * image->numlayers);
	
	/* reading layer records */
	for (image->curlayer = 0;
	     image->curlayer < image->numlayers;
	     image->curlayer++)
	{
#ifdef DEBUG
		g_print("loading layer record %i\n",image->curlayer);
#endif
		/* we trust the file pointer while reading records... */
		if (!psd_load_layer_record(fd, image))
		{
			g_free(image->layers);
			return FALSE;
		}
	}
	
#ifdef DEBUG
	g_print("\n================================================\n\n");
#endif

	/* reading layer pixel data */
	for (image->curlayer = 0;
	     image->curlayer < image->numlayers;
	     image->curlayer++)
	{
#ifdef DEBUG
		g_print("loading layer pixel data %i\n",image->curlayer);
#endif
		/* also, we trust the file pointer for each layer pixels... */
		if (!psd_load_layer_pixeldata(fd, image, func))
		{
			g_free(image->layers);
			return FALSE;
		}
	}
	
	fseek(fd, pos + seclen, SEEK_SET);
	
	g_free(image->layers);
	return TRUE;
}


/*
 * There aren't any length var for layer records, so we should
 * read(skip) all fields no matter whether we need them.
 */
gboolean
psd_load_layer_record(FILE *fd, PsdImage *image)
{
	gint i;
	glong len, extra_len, pos, extra_pos;
	PsdLayer *layer;
	guchar key[4];
	gint flags;
#ifdef DEBUG
	guchar buf[256];
#endif
	
	layer = &image->layers[image->curlayer];
	if (!psd_get_long(fd, &layer->top))
		return FALSE;
	if (!psd_get_long(fd, &layer->left))
		return FALSE;
	if (!psd_get_long(fd, &layer->bottom))
		return FALSE;
	if (!psd_get_long(fd, &layer->right))
		return FALSE;
	
#ifdef DEBUG
	g_print("	top=%li left=%li bottom=%li right=%li\n", layer->top, layer->left, layer->bottom, layer->right);
#endif

	if (!psd_get_short(fd, &layer->num_channels))
		return FALSE;
	
#ifdef DEBUG
	g_print("	channels=%i\n",layer->num_channels);
#endif

	for (i = 0; i < layer->num_channels; i++)
	{
		if (!psd_get_short(fd, &layer->channel_id[i]))
			return FALSE;
		if (!psd_get_long(fd, &layer->channel_len[i]))
			return FALSE;
#ifdef DEBUG
		g_print("	channel: %i  id: %i  len: %li\n",i,layer->channel_id[i],layer->channel_len[i]);
#endif
	}
	
	/* blend signature */
	if (fread(key, 4, 1, fd) < 1 || strncmp(key, "8BIM", 4) != 0)
		return FALSE;
	
	/* blend key -> layer type */
	if (fread(key, 4, 1, fd) < 1) return FALSE;
	layer->mode = psd_modekey_to_lmode(key);
	
	/* opacity */
	if ((layer->opacity = fgetc(fd)) == EOF) return FALSE;
	
	/* clipping */
	if ((layer->clipping = fgetc(fd)) == EOF) return FALSE;
	
	/* flags */
	if ((flags = fgetc(fd)) == EOF) return FALSE;
	layer->protecttrans = ((flags & 1) > 0);
	layer->visible = ((flags & 2) > 0);
	layer->paintonmask = ((flags & 16) > 0);
	
	/* record filler */
	if ((layer->recordfiller = fgetc(fd)) == EOF) return FALSE;

	/* extra data length*/
	if (!psd_get_long(fd, &extra_len)) return FALSE;
	extra_pos = ftell(fd);
#ifdef DEBUG
	g_print("	extradata_pos=%li extradata_len=%li\n",extra_pos,extra_len);
#endif
	
	/* reading mask data */
	if (!psd_get_long(fd, &len)) return FALSE;
#ifdef DEBUG
	g_print("	mask data len=%li\n",len);
#endif
	layer->has_mask = (len > 0);
	if (len > 0)
	{
		pos = ftell(fd);
		if (!psd_get_long(fd, &layer->mask_top))
			return FALSE;
		if (!psd_get_long(fd, &layer->mask_left))
			return FALSE;
		if (!psd_get_long(fd, &layer->mask_bottom))
			return FALSE;
		if (!psd_get_long(fd, &layer->mask_right))
			return FALSE;
		if ((layer->mask_throw_alpha = fgetc(fd)) == EOF)
			return FALSE;
		fseek(fd, pos + len, SEEK_SET);
	}
	
#ifdef DEBUG
	if (layer->has_mask)
		g_print("	mask top=%li left=%li bottom=%li right=%li throw_alpha=%li\n",
			layer->mask_top,
			layer->mask_left,
			layer->mask_bottom,
			layer->mask_right,
			layer->mask_throw_alpha);
#endif

	/* ranges data */
	if (!psd_get_long(fd, &len)) return FALSE;
	layer->rangeslen = len;
	fseek(fd, len, SEEK_CUR);
	
#ifdef DEBUG
	g_print("	blendkey=%c%c%c%c opacity=%i clipping=%i flags=%i\n	protecttrans=%i visible=%i recordfiller=%i rangeslen=%li\n",
		key[0],
		key[1],
		key[2],
		key[3],
		layer->opacity,
		layer->clipping,
		flags,
		layer->protecttrans,
		layer->visible,
		layer->recordfiller,
		layer->rangeslen);
#endif

	/* skip layer name */
#ifdef DEBUG
	if (!psd_get_string(fd, buf)) return FALSE;
	g_print("	loading layer name: %s\n",buf);
#else
	if (!psd_get_string(fd, NULL)) return FALSE;
#endif

	fseek(fd, extra_pos + extra_len, SEEK_SET);
	
	return TRUE;
}

gboolean
psd_load_layer_pixeldata(FILE *fd, PsdImage *image, PsdLoadFunc func)
{
	gint channeli, linei, j, compression, c, n, k, t;
	glong top, left, bottom, right, width, height, len, off, pos;
	PsdLayer *layer;
	guchar *pixeldata, *tmpline;
	gboolean in;
	
	layer = &image->layers[image->curlayer];
	len = sizeof(guchar) * image->columns * image->rows * 4;
	pixeldata = g_malloc(len);
	
	/* make the whole layer visible */
	memset(pixeldata, psd_layer_has_alpha(layer)?0:255, len);

	/* do layers clipped together */
	while (TRUE)
	{	
		for (channeli = 0; channeli < layer->num_channels; channeli++)
		{
			pos = ftell(fd);
			if (layer->channel_id[channeli] == -2)
			{
				top = layer->mask_top;
				left = layer->mask_left;
				bottom = layer->mask_bottom;
				right = layer->mask_right;
				width = right - left;
				height = bottom - top;
			} else
			{
				top = layer->top;
				left = layer->left;
				bottom = layer->bottom;
				right = layer->right;
				width = right - left;
				height = bottom - top;
			}
			
			psd_get_short(fd, &compression);
#ifdef DEBUG
			g_print("	channel: %i  id: %i\n",channeli,layer->channel_id[channeli]);
			g_print("	pos: %li  len: %li\n",pos,layer->channel_len[channeli]);
			g_print("	compression: %i\n", compression);
#endif

			switch(layer->channel_id[channeli])
			{
			  case 0: /* red/gray */
			  case 1: /* green */
			  case 2: /* blue */
				switch(image->type)
				{
				  case PSD_GRAY_IMAGE:
					off = -1;
					break;
				  case PSD_INDEXED_IMAGE:
					off = -2;
				  case PSD_RGB_IMAGE:
				  case PSD_RGBA_IMAGE:
				  default:
					off = layer->channel_id[channeli];
					break;
				}
				break;
			  case -1: /* alpha */
				off = 3;
				break;
			  case -2: /* mask */
				off = 4;
				break;
			  default: /* unknown id.... */
				off = -3;
				break;
			}
			
			/* FIX THIS: do not know how to do with paint_on_mask */
			if (layer->paintonmask) off = -3;
			
			switch (compression)
			{
			  case 0: /* raw data */
				for (linei = top; linei < bottom; linei++)
				{
					tmpline = &pixeldata[(linei*image->columns)<<2];
					
					for (j = left; j < right; j++)
					{
						c = fgetc(fd);
						if (linei >= 0 && linei < image->rows &&
						    j>= 0 && j < image->columns)
						{
							psd_put_pixel_element(image, tmpline, j, off, c);
						}
					}
				}
				break;
			  case 1: /* RLE */
				fseek(fd, height * 2, SEEK_CUR);
				for (linei = top; linei < bottom; linei++)
				{
					tmpline = &pixeldata[(linei*image->columns)<<2];
					j = left;
					while (j < right)
					{
						if ((n = fgetc(fd)) == EOF) break;
						if (n == 128) continue;
						if (n < 128) /* copy n+1 chars */
						{
							n++;
							in = linei >= 0 && linei < image->rows &&
							     j >= 0 && j < image->columns; 
							for (k = 0; k < n; k++)
							{
								c = fgetc(fd);
								if (in)
								{
									psd_put_pixel_element(image, tmpline, j, off, c);
								}
								j++;
							}
						} else /* duplicate next byte -n+1 times */
						{
							n = 257 - n;
							c = fgetc(fd);
							in = linei >= 0 && linei < image->rows &&
							     j >= 0 && j < image->columns;
							for (k = 0; k < n; k++)
							{
								if (in)
								{
									psd_put_pixel_element(image, tmpline, j, off, c);
								}
								j++;
							}
						}
					}
				}
				break;
			  default: /* *unknown* */
				break;
			}
			fseek(fd, pos + layer->channel_len[channeli], SEEK_SET);
		}
	
		/* do mask filling, opacity... */
		top = max(0, layer->top);
		left = max(0, layer->left);
		bottom = min(image->rows, layer->bottom);
		right = min(image->columns, layer->right);
		for (linei = top; linei < bottom; linei++)
		{
			tmpline = &pixeldata[(linei*image->columns)<<2];	
			for (j = left; j < right; j++)
			{
				t = j<<2;
				/* mask filling */
				if (layer->has_mask && !layer->paintonmask)
				{
					if (linei < layer->mask_top ||
					    linei >= layer->mask_bottom ||
					    j < layer->mask_left ||
					    j >= layer->mask_right)
					{
						tmpline[t+3] = (guchar)layer->mask_throw_alpha;
					}
				}
				/* opacity */
				tmpline[t+3] = tmpline[t+3] * layer->opacity / 255;
			}
		}
	
		if (image->curlayer == image->numlayers - 1 ||
		    !image->layers[image->curlayer + 1].clipping) break;
		image->curlayer++;
		layer = &image->layers[image->curlayer];
	}
	
	/* send pixels row by row... */
	width = right - left;
	for (linei = top ; linei < bottom; linei++)
	{
		tmpline = &pixeldata[(linei*image->columns+left)*4];
		if ((*func) (tmpline, width, left, linei, 4, image->curlayer, layer->mode)) break;
	}
	
	g_free(pixeldata);
	return TRUE;
}

/* PS2 style, without layers */
gboolean
psd_load_image_data(FILE *fd, PsdImage *image, PsdLoadFunc func)
{
	gint compression, i, j, k, w, c, n, off;
	glong len;
	guchar *dest, *tmpline;
	
	if (!psd_get_short(fd, &compression)) return FALSE;
	
#ifdef DEBUG
	g_print("====================================================\n");
	g_print("channels: %i  aux channels: %i  compression: %i\n",image->channels,image->num_aux_channels,compression);
#endif

	len = 4 * image->columns * image->rows;
	dest = g_malloc(len);
	memset(dest, 255, len);

	if (compression == 1) /* RLE */
	{
		fseek(fd, 2 * image->rows * image->channels, SEEK_CUR);
	}
	
	for (i = 0; i < image->channels; i++)
	{
		if (image->cmyk)
		{
			if (i == 3 || i > 4)
			{
				off = -3;
			} else
			{
				off = (i<3) ? i : 3;
			}
		} else
		{
			switch (image->type)
			{
			  case PSD_BITMAP_IMAGE:
			  case PSD_GRAY_IMAGE:
				off = (i==0) ? -1 : 3;
				break;
			  case PSD_INDEXED_IMAGE:
				off = (i==0) ? -2 : 3;
				break;
			  case PSD_RGB_IMAGE:
			  case PSD_RGBA_IMAGE:
				off = min(i, 3);
				break;
			  default:
			  	off = -3;
				break;
			}
		}
		for (j = 0; j < image->rows; j++)
		{
			w = 0;
			tmpline = &dest[(j*image->columns)<<2];
			while (w < image->columns)
			{
				switch (compression)
				{
				  case 0: /* raw data */
					c = fgetc(fd);
					if (image->bpp == 1)
					{
						for (k = 0; k < 8 && w < image->columns; k++)
						{
							psd_put_pixel_element(image, tmpline, w++, off, (c&0x80)?0:255);
							c <<= 1;
						}
					} else
					{
						psd_put_pixel_element(image, tmpline, w, off, c);
						w++;
					}
					break;
				  case 1: /* RLE */
					if ((n = fgetc(fd)) == EOF) break;
					if (n == 128) continue;
					if (n < 128) /* copy n+1 chars */
					{
						n++;
						for (k = 0; k < n; k++)
						{
							c = fgetc(fd);
							psd_put_pixel_element(image, tmpline, w, off, c);
							w++;
						}
					} else /* duplicate next byte -n+1 times */
					{
						c = fgetc(fd);
						n = 257 - n;
						for (k = 0; k < n; k++)
						{
							psd_put_pixel_element(image, tmpline, w, off, c);
							w++;
						}
					}
					break;
				  default: /* *unknown* */
					break;
				}
			}
		}
	}
	
	for (i = 0; i < image->rows; i++)
	{
		if ((*func) (&dest[(i*image->columns)<<2], image->columns, 0, i, 4, 0, NORMAL_MODE)) break;
	}
	
	g_free(dest);
	return TRUE;
}

gboolean
psd_layer_has_alpha(PsdLayer* layer)
{
	gint i;

	for (i = 0; i < layer->num_channels; i++)
	{
		if (layer->channel_id[i] == -1)
		{
			return TRUE;
		}
	}
	return FALSE;
}

void
psd_put_pixel_element(PsdImage *image, guchar *line, gint x, gint off, gint ic)
{
	gint t = x<<2;
	guchar c = (guchar) ic;
	
	switch (off)
	{
	  case -1: /* gray */
		line[t++] = c;
		line[t++] = c;
		line[t] = c;
		break;
	  case -2: /* indexed */
		line[t++] = image->cmap[ic];
		line[t++] = image->cmap[256+ic];
		line[t] = image->cmap[512+ic];
		break;
	  case -3: /* do nothing */
		break;
	  case 4: /* mask */
		line[t++] = line[t] * c / 255;
		line[t++] = line[t] * c / 255;
		line[t++] = line[t] * c / 255;
		line[t++] = line[t] * c / 255;
		break;
	  default:
		line[t+off] = c;
		break;
	}
}

LayerMode
psd_modekey_to_lmode(guchar *modekey)
{
	if (strncmp(modekey, "norm", 4)==0) return NORMAL_MODE;
	if (strncmp(modekey, "dark", 4)==0) return DARKEN_ONLY_MODE;
	if (strncmp(modekey, "lite", 4)==0) return LIGHTEN_ONLY_MODE;
	if (strncmp(modekey, "hue ", 4)==0) return HUE_MODE;
	if (strncmp(modekey, "sat ", 4)==0) return SATURATION_MODE;
	if (strncmp(modekey, "colr", 4)==0) return COLOR_MODE;
	if (strncmp(modekey, "mul ", 4)==0) return MULTIPLY_MODE;
	if (strncmp(modekey, "scrn", 4)==0) return SCREEN_MODE;
	if (strncmp(modekey, "diss", 4)==0) return DISSOLVE_MODE;
	if (strncmp(modekey, "diff", 4)==0) return DIFFERENCE_MODE;
	if (strncmp(modekey, "over", 4)==0) return OVERLAY_MODE;
	if (strncmp(modekey, "lum ", 4)==0) return VALUE_MODE;    /* ?  */

	/* unsupported layer-blend mode, using alternative modes... */

	if (strncmp(modekey, "hLit", 4)==0) return NORMAL_MODE;
	if (strncmp(modekey, "sLit", 4)==0) return NORMAL_MODE;
	
	return NORMAL_MODE;
}
