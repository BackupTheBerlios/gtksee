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

#ifdef HAVE_LIBTIFF

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <tiffio.h>
#include "im_tiff.h"

void
tiff_error_handler(const char *s1, const char *s2)
{
	/* we do nothing here, ignoring the warning messges */
}

gboolean
tiff_get_header(gchar *filename, tiff_info *info)
{
	TIFF *tif;
	gushort bps, spp;
	gushort extra, *extra_types;
	
	TIFFSetWarningHandler((TIFFErrorHandler)tiff_error_handler);
	TIFFSetErrorHandler((TIFFErrorHandler)tiff_error_handler);
	tif = TIFFOpen(filename, "r");
	if (tif == NULL) return FALSE;
	if (!TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &info->width) ||
	    !TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &info->height))
	{
		TIFFClose(tif);
		return FALSE;
	}

	if (!TIFFGetField (tif, TIFFTAG_BITSPERSAMPLE, &bps))
		bps = 1;

	if (bps > 8)
	{
		TIFFClose(tif);
		return FALSE;
	}

	if (!TIFFGetField (tif, TIFFTAG_SAMPLESPERPIXEL, &spp))
		spp = 1;

	if (!TIFFGetField (tif, TIFFTAG_EXTRASAMPLES, &extra, &extra_types))
		extra = 0;
	if (extra > 0 && (extra_types[0] == EXTRASAMPLE_ASSOCALPHA))
	{
		info->alpha = 1;
	} else
	{
		info->alpha = 0;
	}

	info->ncolors = (bps * spp) - info->alpha * 8;
	
	TIFFClose(tif);
	return TRUE;
}

gboolean
tiff_load(gchar *filename, TiffLoadFunc func)
{
	TIFF *tif;
	gushort bps, spp, photomet;
	gint cols, rows, maxval, alpha;
	gushort *redmap, *greenmap, *bluemap;
	guchar cmap[768];
	gushort extra, *extra_types;

	gint col, row, start, i, j, val;
	guchar sample;
	gint bitsleft;
	gint gray_val, red_val, green_val, blue_val, alpha_val;

	guchar *source, *s, *dest, *d;

	TIFFSetWarningHandler((TIFFErrorHandler)tiff_error_handler);
	TIFFSetErrorHandler((TIFFErrorHandler)tiff_error_handler);
	tif = TIFFOpen (filename, "r");
	if (!tif) return FALSE;

	if (!TIFFGetField (tif, TIFFTAG_BITSPERSAMPLE, &bps))
		bps = 1;

	if (bps > 8)
	{
		TIFFClose(tif);
		return FALSE;
	}

	if (!TIFFGetField (tif, TIFFTAG_SAMPLESPERPIXEL, &spp))
		spp = 1;
	
	if (!TIFFGetField (tif, TIFFTAG_EXTRASAMPLES, &extra, &extra_types))
		extra = 0;

	if (!TIFFGetField (tif, TIFFTAG_IMAGEWIDTH, &cols))
	{
		TIFFClose(tif);
		return FALSE;
	}

	if (!TIFFGetField (tif, TIFFTAG_IMAGELENGTH, &rows))
	{
		TIFFClose(tif);
		return FALSE;
	}

	if (!TIFFGetField (tif, TIFFTAG_PHOTOMETRIC, &photomet))
	{
		TIFFClose(tif);
		return FALSE;
	}

	/* test if the extrasample represents an associated alpha channel... */
	if (extra > 0 && (extra_types[0] == EXTRASAMPLE_ASSOCALPHA))
	{
		alpha = 1;
	} else
	{
		alpha = 0;
	}

	if (photomet == PHOTOMETRIC_RGB && spp > extra + 3)
	{
		extra= spp - 3; 
		alpha= 1;
	} else
	if (photomet != PHOTOMETRIC_RGB && spp > extra + 1)
	{
		extra= spp - 1;
		alpha= 1;
	}

	maxval = (1 << bps) - 1;

	switch (photomet)
	{
		case PHOTOMETRIC_MINISBLACK:
		case PHOTOMETRIC_MINISWHITE:
			break;
		case PHOTOMETRIC_RGB:
			break;
		case PHOTOMETRIC_PALETTE:
			break;
		case PHOTOMETRIC_MASK:
		default:
			TIFFClose(tif);
			return FALSE;
	}

	/* Install colormap for INDEXED images only */
	if (photomet == PHOTOMETRIC_PALETTE)
	{
		if (!TIFFGetField (tif, TIFFTAG_COLORMAP, &redmap, &greenmap, &bluemap))
		{
			TIFFClose(tif);
			return FALSE;
		}

		for (i = 0, j = 0; i <= maxval; i++)
		{
			cmap[j++] = redmap[i] >> 8;
			cmap[j++] = greenmap[i] >> 8;
			cmap[j++] = bluemap[i] >> 8;
		}
	}

	source = g_new (guchar, TIFFScanlineSize (tif));
	dest = g_new (guchar, cols * 4);

/* Step through all <= 8-bit samples in an image */

#define NEXTSAMPLE(var)                       \
  {                                           \
      if (bitsleft == 0)                      \
      {                                       \
	  s++;                                \
	  bitsleft = 8;                       \
      }                                       \
      bitsleft -= bps;                        \
      var = ( *s >> bitsleft ) & maxval;      \
  }

	for (start = 0, row = 0; row < rows; row++)
	{
		d = dest;
		if (TIFFReadScanline (tif, source, row, 0) < 0)
		{
			g_free(dest);
			g_free(source);
			TIFFClose(tif);
			return TRUE;
		}

		/* Set s/bitleft ready to use NEXTSAMPLE macro */

		s = source;
		bitsleft = 8;

		switch (photomet)
		{
		  case PHOTOMETRIC_MINISBLACK:
			for (col = 0; col < cols; col++)
			{
				NEXTSAMPLE(gray_val);
				if (alpha)
				{
					NEXTSAMPLE(alpha_val);
					if (alpha_val)
					{
						sample = (gray_val * 65025) / (alpha_val * maxval);
						*d++ = sample;
						*d++ = sample;
						*d++ = sample;
					} else
					{
						*d++ = 0;
						*d++ = 0;
						*d++ = 0;
					}
					*d++ = alpha_val;
				} else
				{
					sample = (gray_val * 255) / maxval;
					*d++ = sample;
					*d++ = sample;
					*d++ = sample;
					*d++ = 255;
				}
				for (i = 0; alpha + i < extra; ++i)
				{
					NEXTSAMPLE(sample);
				}
			}
			break;

		  case PHOTOMETRIC_MINISWHITE:
			for (col = 0; col < cols; col++)
			{
				NEXTSAMPLE(gray_val);
				if (alpha)
				{
					NEXTSAMPLE(alpha_val);
					if (alpha_val)
					{
						sample = ((maxval - gray_val) * 65025) / (alpha_val * maxval);
						*d++ = sample;
						*d++ = sample;
						*d++ = sample;
					} else
					{
						*d++ = 0;
						*d++ = 0;
						*d++ = 0;
					}
					*d++ = alpha_val;
				} else
				{
					sample = ((maxval - gray_val) * 255) / maxval;
					*d++ = sample;
					*d++ = sample;
					*d++ = sample;
					*d++ = 255;
				}
				for (i = 0; alpha + i < extra; ++i)
				{
					NEXTSAMPLE(sample);
				}
			}
			break;

		  case PHOTOMETRIC_PALETTE:
			for (col = 0; col < cols; col++)
			{
				NEXTSAMPLE(sample);
				val = sample * 3;
				*d++ = cmap[val++];
				*d++ = cmap[val++];
				*d++ = cmap[val];
				if (alpha)
				{
					NEXTSAMPLE(sample);
					*d++ = sample;
				} else
				{
					*d++ = 255;
				}
				for (i= 0; alpha + i < extra; ++i)
				{
					NEXTSAMPLE(sample);
				}
			}
			break;
  
		  case PHOTOMETRIC_RGB:
			for (col = 0; col < cols; col++)
			{
				NEXTSAMPLE(red_val)
				NEXTSAMPLE(green_val)
				NEXTSAMPLE(blue_val)
				if (alpha)
				{
					NEXTSAMPLE(alpha_val)
					if (alpha_val)
					{
						*d++ = red_val * 255 / alpha_val;
						*d++ = green_val * 255 / alpha_val;
						*d++ = blue_val * 255 / alpha_val;
					} else
					{
						*d++ = 0;
						*d++ = 0;
						*d++ = 0;
					}
					*d++ = alpha_val;
				} else
				{
					*d++ = red_val;
					*d++ = green_val;
					*d++ = blue_val;
					*d++ = 255;
				}
				for (i = 0; alpha + i < extra; ++i)
				{
					NEXTSAMPLE(sample);
				}
			}
			break;

		  default:
			break;
		}
		if ((*func) (dest, cols, 0, row, 4, -1, 0)) break;
	}
	
	g_free(dest);
	g_free(source);
	TIFFClose(tif);
	return TRUE;
}

#endif /* HAVE_LIBTIFF */
