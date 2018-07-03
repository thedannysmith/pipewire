/* Spa
 * Copyright (C) 2018 Wim Taymans <wim.taymans@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <string.h>
#include <stdio.h>

#include <spa/utils/defs.h>

#define VOLUME_MIN 0.0f
#define VOLUME_NORM 1.0f

static void
channelmix_copy(void *data, int n_dst, void *dst[n_dst],
	   int n_src, const void *src[n_src], void *matrix, int n_bytes)
{
	int i, j;
	float *m = matrix;
	float v = m[0];

	if (v <= VOLUME_MIN) {
		for (i = 0; i < n_src; i++)
			memset(dst[i], 0, n_bytes);
	}
	else if (v == VOLUME_NORM) {
		for (i = 0; i < n_src; i++)
			memcpy(dst[i], src[i], n_bytes);
	}
	else {
		float **d = (float **) dst;
		float **s = (float **) src;
		int n_samples = n_bytes / sizeof(float);

		for (i = 0; i < n_src; i++)
			for (j = 0; j < n_samples; j++)
				d[i][j] = s[i][j] * v;
	}
}

static void
channelmix_f32_n_m(void *data, int n_dst, void *dst[n_dst],
		   int n_src, const void *src[n_src], void *matrix, int n_bytes)
{
	int i, j, n, n_samples;
	float **d = (float **) dst;
	float **s = (float **) src;
	float *m = matrix;

	n_samples = n_bytes / sizeof(float);
	for (n = 0; n < n_samples; n++) {
		for (i = 0; i < n_dst; i++) {
			float sum = 0.0f;

			for (j = 0; j < n_src; j++)
				sum += s[j][n] * m[i * n_src + j];

			d[i][n] = sum;
		}
	}
}

static void
channelmix_f32_1_2(void *data, int n_dst, void *dst[n_dst],
		   int n_src, const void *src[n_src], void *matrix, int n_bytes)
{
	int n, n_samples;
	float **d = (float **) dst;
	const float *s = src[0];
	float *m = matrix;
	float v = m[0];

	n_samples = n_bytes / sizeof(float);
	if (v <= VOLUME_MIN) {
		memset(d[0], 0, n_bytes);
	}
	else if (v == VOLUME_NORM) {
		for (n = 0; n < n_samples; n++)
			d[0][n] = d[1][n] = s[n];
	}
	else {
		for (n = 0; n < n_samples; n++)
			d[0][n] = d[1][n] = s[n] * v;
	}
}

static void
channelmix_f32_2_1(void *data, int n_dst, void *dst[n_dst],
		   int n_src, const void *src[n_src], void *matrix, int n_bytes)
{
	int n, n_samples;
	float *d = dst[0];
	float **s = (float **) src;
	float *m = matrix;
	float v = m[0];

	n_samples = n_bytes / sizeof(float);
	if (v <= VOLUME_MIN) {
		memset(d, 0, n_bytes);
	}
	else if (v == VOLUME_NORM) {
		for (n = 0; n < n_samples; n++)
			d[n] = (s[0][n] + s[1][n]) * 0.5f;
	}
	else {
		v *= 0.5f;
		for (n = 0; n < n_samples; n++)
			d[n] = (s[0][n] + s[1][n]) * v;
	}
}

typedef void (*channelmix_func_t) (void *data, int n_dst, void *dst[n_dst],
				   int n_src, const void *src[n_src],
				   void *matrix, int n_bytes);


static const struct channelmix_info {
	uint32_t src_chan;
	uint32_t dst_chan;

	channelmix_func_t func;
	uint32_t flags;
} channelmix_table[] =
{
	{ -2, -2, channelmix_copy, 0 },
	{ 1, 2, channelmix_f32_1_2, 0 },
	{ 2, 1, channelmix_f32_2_1, 0 },
	{ -1, -1, channelmix_f32_n_m, 0 },
};

#define MATCH_CHAN(a,b)	((a) == -1 || (a) == (b))

static const struct channelmix_info *find_channelmix_info(uint32_t src_chan, uint32_t dst_chan)
{
	int i;

	if (src_chan == dst_chan)
		return &channelmix_table[0];

	for (i = 1; i < SPA_N_ELEMENTS(channelmix_table); i++) {
		if (MATCH_CHAN(channelmix_table[i].src_chan, src_chan) &&
		    MATCH_CHAN(channelmix_table[i].dst_chan, dst_chan))
			return &channelmix_table[i];
	}
	return NULL;
}
