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
 * This loader refers to:
 * JPEG loading and saving file filter for the GIMP
 *	-Peter Mattis
 */

#include "config.h"

#ifdef HAVE_LIBJPEG

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jpeglib.h>
#include <gtk/gtk.h>

#include "im_jpeg.h"

typedef struct my_error_mgr {
	struct jpeg_error_mgr pub;	/* "public" fields */

	jmp_buf setjmp_buffer;	/* for return to caller */
} *my_error_ptr;

static struct my_error_mgr jerr;

/*
 * Here's the routine that will replace the standard error_exit method:
 */

static void
my_error_exit (j_common_ptr cinfo)
{
	/* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
	my_error_ptr myerr = (my_error_ptr) cinfo->err;

	/* Always display the message. */
	/* We could postpone this until after returning, if we chose. */
	/*(*cinfo->err->output_message) (cinfo);*/

	/* Return control to the setjmp point */
	longjmp (myerr->setjmp_buffer, 1);
}

gboolean
jpeg_get_header(gchar *filename, jpeg_info *cinfo)
{
	FILE *infile;
	
	cinfo->err = jpeg_std_error (&jerr.pub);
	jerr.pub.error_exit = my_error_exit;

	if (setjmp (jerr.setjmp_buffer))
	{
		jpeg_destroy_decompress (cinfo);
		if (infile) fclose (infile);
		return FALSE;
	}

	if ((infile = fopen (filename, "rb")) == NULL)
	{
		return FALSE;
	}
	
	jpeg_create_decompress (cinfo);
	jpeg_stdio_src (cinfo, infile);
	jpeg_read_header (cinfo, TRUE);
	jpeg_destroy_decompress (cinfo);
	
	fclose (infile);
	return TRUE;
}

gboolean
jpeg_load(gchar *filename, JpegLoadFunc func)
{
	FILE *infile;
	JSAMPROW buffer[1];
	jpeg_info cinfo;
	
	cinfo.err = jpeg_std_error (&jerr.pub);
	jerr.pub.error_exit = my_error_exit;

	if (setjmp (jerr.setjmp_buffer))
	{
		jpeg_destroy_decompress (&cinfo);
		if (infile) fclose (infile);
		return FALSE;
	}

	if ((infile = fopen (filename, "rb")) == NULL)
	{
		return FALSE;
	}


	jpeg_create_decompress (&cinfo);
	jpeg_stdio_src (&cinfo, infile);
	jpeg_read_header (&cinfo, TRUE);

	if (cinfo.image_width < 0 || cinfo.num_components < 0)
	{
		fclose(infile);
		return FALSE;
	}
	
	cinfo.quantize_colors = FALSE;
	cinfo.do_fancy_upsampling = FALSE;
	cinfo.do_block_smoothing = FALSE;
	
	buffer[0] = (JSAMPROW)g_new (guchar, cinfo.image_width * cinfo.num_components);
	if (buffer[0] == NULL) {
		g_warning("Out of memory!");
		return FALSE;
	}

	jpeg_start_decompress (&cinfo);

	while (cinfo.output_scanline < cinfo.output_height)
	{
		jpeg_read_scanlines (&cinfo, buffer, 1);
		if ((*func) (buffer[0], cinfo.output_width, 0, cinfo.output_scanline-1, cinfo.output_components, -1, 0)) break;
	}

	jpeg_finish_decompress (&cinfo);
	jpeg_destroy_decompress (&cinfo);
	
	fclose(infile);

	g_free (buffer[0]);
	return TRUE;
}

#endif /* HAVE_LIBJPEG */
