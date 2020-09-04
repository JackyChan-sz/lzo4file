/* testmini.c -- very simple test program for the miniLZO library

   This file is part of the LZO real-time data compression library.

   Copyright (C) 1996-2017 Markus Franz Xaver Johannes Oberhumer
   All Rights Reserved.

   The LZO library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   The LZO library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with the LZO library; see the file COPYING.
   If not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

   Markus F.X.J. Oberhumer
   <markus@oberhumer.com>
   http://www.oberhumer.com/opensource/lzo/
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "..\\getopt\\getopt.h"


/*************************************************************************
// This program shows the basic usage of the LZO library.
// We will compress a block of data and decompress again.
//
// For more information, documentation, example programs and other support
// files (like Makefiles and build scripts) please download the full LZO
// package from
//    http://www.oberhumer.com/opensource/lzo/
**************************************************************************/

/* First let's include "minizo.h". */

#include "minilzo.h"


/* We want to compress the data block at 'in' with length 'IN_LEN' to
 * the block at 'out'. Because the input block may be incompressible,
 * we must provide a little more output space in case that compression
 * is not possible.
 */

#define IN_LEN      (128*1024ul)
#define OUT_LEN     (IN_LEN + IN_LEN / 16 + 64 + 3)

static unsigned char __LZO_MMODEL in  [ IN_LEN ];
static unsigned char __LZO_MMODEL out [ OUT_LEN ];


/* Work-memory needed for compression. Allocate memory in units
 * of 'lzo_align_t' (instead of 'char') to make sure it is properly aligned.
 */

#define HEAP_ALLOC(var,size) \
    lzo_align_t __LZO_MMODEL var [ ((size) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t) ]

static HEAP_ALLOC(wrkmem, LZO1X_1_MEM_COMPRESS);


/*************************************************************************
//
**************************************************************************/
#define HEAD_SIZE (8)
const char FILE_HEAD[HEAD_SIZE] = "MINILZO";
static const size_t element_size = sizeof(unsigned char);
static const size_t block_length_size = sizeof(lzo_uint);
int compress_file(char* name_in, char* name_out)
{
	bool error = false;
	FILE* file_in = NULL;
	bool file_in_opened = false;
	FILE* file_out = NULL;
	bool file_out_opened = false;

	if (!error)
	{
		if (0 == fopen_s(&file_in, name_in, "rb"))
		{
			file_in_opened = true;
		}
		else
		{
			error = true;
		}
	}

	if (!error)
	{
		if (0 == fopen_s(&file_out, name_out, "wb"))
		{
			file_out_opened = true;
		}
		else
		{
			error = true;
		}
	}

	if (!error)
	{
		fwrite(FILE_HEAD, 1, HEAD_SIZE, file_out);
	}

	bool end_of_file = false;
	while( !error && !end_of_file )
	{
		size_t actual_read = 0;
		size_t count = IN_LEN / element_size;
		actual_read = fread_s(in, IN_LEN, element_size, count, file_in);
		if (actual_read > 0)
		{
			lzo_uint in_len = actual_read;
			lzo_uint out_len = 0;
			if (LZO_E_OK == lzo1x_1_compress(in, in_len, out, &out_len, wrkmem))
			{
				fwrite(&out_len, 1, block_length_size, file_out);

				fwrite(out, element_size, out_len, file_out);
			}
		}
		else
		{
			end_of_file = true;
		}
	}

	if (file_out_opened)
	{
		fclose(file_out);
	}

	if (file_in_opened)
	{
		fclose(file_in);
	}

	return error ? 0 : -1;
}

int decompress_file(char* name_in, char* name_out)
{
	bool error = false;
	FILE* file_in = NULL;
	bool file_in_opened = false;
	FILE* file_out = NULL;
	bool file_out_opened = false;

	if (!error)
	{
		if (0 == fopen_s(&file_in, name_in, "rb"))
		{
			file_in_opened = true;
		}
		else
		{
			error = true;
		}
	}

	if (!error)
	{
		memset(out, 0, HEAD_SIZE);
		size_t actual_read = fread_s(out, OUT_LEN, element_size, HEAD_SIZE, file_in);
		if (0 != (memcmp(out, FILE_HEAD, HEAD_SIZE)))
		{
			error = true;
		}
	}

	if (!error)
	{
		if (0 == fopen_s(&file_out, name_out, "wb"))
		{
			file_out_opened = true;
		}
		else
		{
			error = true;
		}
	}

	if (!error)
	{
		bool end_of_file = false;
		while (!end_of_file)
		{
			size_t actual_read = fread_s(out, OUT_LEN, element_size, block_length_size, file_in);
			if (block_length_size == actual_read)
			{
				size_t block_length = 0;
				block_length <<= 8; block_length += out[3];
				block_length <<= 8; block_length += out[2];
				block_length <<= 8; block_length += out[1];
				block_length <<= 8; block_length += out[0];

				if (block_length > 0)
				{
					actual_read = fread_s(out, OUT_LEN, element_size, block_length, file_in);
					if (actual_read >= block_length)
					{
						lzo_uint out_len = actual_read;
						lzo_uint new_len = 0;
						if (LZO_E_OK == lzo1x_decompress(out, out_len, in, &new_len, NULL))
						{
							fwrite(in, element_size, new_len, file_out);
						}
					}
					else
					{
						end_of_file = true;
					}
				}
			}
			else
			{
				end_of_file = true;
			}
		}
	}

	if (file_out_opened)
	{
		fclose(file_out);
	}

	if (file_in_opened)
	{
		fclose(file_in);
	}

	return error ? 0 : -1;
}

void print_help(void)
{
	printf("\nCompress file.");
	printf("\n     lzo4file -c source_file destination_file\n\n");
	printf("\nDecompress file.");
	printf("\n     lzo4file -d source_file destination_file\n\n");
	system("pause");
}

typedef enum {
	OPERATION_NONE,
	OPERATION_PRINT_HELP,
	OPERATION_COMPRESS,
	OPERATION_DECOMPRESS,
	OPERATION_MAX
}OPERATION_TYPE;

static char name_in[_MAX_PATH];
static char name_out[_MAX_PATH];
OPERATION_TYPE option_process(int argc, char *argv[])
{
	OPERATION_TYPE op_type = OPERATION_NONE;

	int c = -1;
	while (-1 != (c = getopt(argc, (char*const*)argv, "c:d:h")))
	{
		switch (c)
		{
		case 'c':
			strcpy_s(name_in, _MAX_PATH, optarg);
			strcpy_s(name_out, _MAX_PATH, argv[optind]);

			op_type = OPERATION_COMPRESS;
			break;
		case 'd':
			strcpy_s(name_in, _MAX_PATH, optarg);
			strcpy_s(name_out, _MAX_PATH, argv[optind]);

			op_type = OPERATION_DECOMPRESS;
			break;
		case 'h':
			op_type = OPERATION_PRINT_HELP;
			break;
		default:
			break;
		}
	}

	return op_type;
}

int main(int argc, char *argv[])
{
    int r;
    lzo_uint in_len;
    lzo_uint out_len;
    lzo_uint new_len;

    //if (argc < 0 && argv == NULL)   /* avoid warning about unused args */
    //    return 0;

	OPERATION_TYPE op_type = option_process(argc, argv);
	if (OPERATION_COMPRESS == op_type)
	{
		compress_file(name_in, name_out);
	}
	else if (OPERATION_DECOMPRESS == op_type)
	{
		decompress_file(name_in, name_out);
	}
	else
	{
		print_help();
	}
	return 0;

    printf("\nLZO real-time data compression library (v%s, %s).\n",
           lzo_version_string(), lzo_version_date());
    printf("Copyright (C) 1996-2017 Markus Franz Xaver Johannes Oberhumer\nAll Rights Reserved.\n\n");


/*
 * Step 1: initialize the LZO library
 */
    if (lzo_init() != LZO_E_OK)
    {
        printf("internal error - lzo_init() failed !!!\n");
        printf("(this usually indicates a compiler bug - try recompiling\nwithout optimizations, and enable '-DLZO_DEBUG' for diagnostics)\n");
        return 3;
    }
	//compress_file("d:/qu.txt", "d:/qu.lzo");
	decompress_file("d:/qu.lzo", "d:/qu.new");
/*
 * Step 2: prepare the input block that will get compressed.
 *         We just fill it with zeros in this example program,
 *         but you would use your real-world data here.
 */
    in_len = IN_LEN;
    lzo_memset(in,0,in_len);

/*
 * Step 3: compress from 'in' to 'out' with LZO1X-1
 */
    r = lzo1x_1_compress(in,in_len,out,&out_len,wrkmem);
    if (r == LZO_E_OK)
        printf("compressed %lu bytes into %lu bytes\n",
            (unsigned long) in_len, (unsigned long) out_len);
    else
    {
        /* this should NEVER happen */
        printf("internal error - compression failed: %d\n", r);
        return 2;
    }
    /* check for an incompressible block */
    if (out_len >= in_len)
    {
        printf("This block contains incompressible data.\n");
        return 0;
    }

/*
 * Step 4: decompress again, now going from 'out' to 'in'
 */
    new_len = in_len;
    r = lzo1x_decompress(out,out_len,in,&new_len,NULL);
    if (r == LZO_E_OK && new_len == in_len)
        printf("decompressed %lu bytes back into %lu bytes\n",
            (unsigned long) out_len, (unsigned long) in_len);
    else
    {
        /* this should NEVER happen */
        printf("internal error - decompression failed: %d\n", r);
        return 1;
    }

    printf("\nminiLZO simple compression test passed.\n");
    return 0;
}


/* vim:set ts=4 sw=4 et: */
