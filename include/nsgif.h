/*
 * Copyright 2004 Richard Wilson <richard.wilson@netsurf-browser.org>
 * Copyright 2008 Sean Fox <dyntryx@gmail.com>
 * Copyright 2013-2021 Michael Drake <tlsa@netsurf-browser.org>
 *
 * This file is part of NetSurf's libnsgif, http://www.netsurf-browser.org/
 * Licenced under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 */

/**
 * \file
 * Interface to progressive animated GIF file decoding.
 */

#ifndef NSNSGIF_H
#define NSNSGIF_H

#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

#define NSGIF_INFINITE (UINT32_MAX)

typedef struct nsgif nsgif;

typedef struct nsgif_info {
	/** width of GIF (may increase during decoding) */
	uint32_t width;
	/** height of GIF (may increase during decoding) */
	uint32_t height;
	/** number of frames decoded */
	uint32_t frame_count;
	/** number of times to loop animation */
	int loop_max;
	/** number of animation loops so far */
	int loop_count;

	uint16_t delay_min;
} nsgif_info_t;

/* Error return values */
typedef enum {
	NSGIF_WORKING = 1,
	NSGIF_OK = 0,
	NSGIF_INSUFFICIENT_DATA = -1,
	NSGIF_INSUFFICIENT_FRAME_DATA = NSGIF_INSUFFICIENT_DATA,
	NSGIF_FRAME_DATA_ERROR = -2,
	NSGIF_DATA_ERROR = -4,
	NSGIF_INSUFFICIENT_MEMORY = -5,
	NSGIF_FRAME_NO_DISPLAY = -6,
	NSGIF_END_OF_FRAME = -7,
	NSGIF_FRAME_INVALID = -8,
	NSGIF_ANIMATION_COMPLETE = -9,
} nsgif_result;

/**
 * GIF rectangle structure.
 *
 * * Top left coordinate is `(x0, y0)`.
 * * Width is `x1 - x0`.
 * * Height is `y1 - y0`.
 * * Units are pixels.
 */
typedef struct nsgif_rect {
	/** x co-ordinate of redraw rectangle, left */
	uint32_t x0;
	/** y co-ordinate of redraw rectangle, top */
	uint32_t y0;
	/** x co-ordinate of redraw rectangle, right */
	uint32_t x1;
	/** y co-ordinate of redraw rectangle, bottom */
	uint32_t y1;
} nsgif_rect;

/**
 * Client bitmap type.
 *
 * These are client-created and destroyed, via the \ref bitmap callbacks,
 * but they are owned by a \ref nsgif.
 */
typedef void nsgif_bitmap_t;

/** Bitmap callbacks function table */
typedef struct nsgif_bitmap_cb_vt {
	/**
	 * Callback to create a bitmap with the given dimensions.
	 *
	 * \param[in]  width   Required bitmap width in pixels.
	 * \param[in]  height  Required bitmap height in pixels.
	 * \return pointer to client's bitmap structure or NULL on error.
	 */
	nsgif_bitmap_t* (*create)(int width, int height);

	/**
	 * Callback to free a bitmap.
	 *
	 * \param[in]  bitmap  The bitmap to destroy.
	 */
	void (*destroy)(nsgif_bitmap_t *bitmap);

	/**
	 * Get pointer to pixel buffer in a bitmap.
	 *
	 * The pixel buffer must be `width * height * sizeof(uint32_t)`.
	 *
	 * \param[in]  bitmap  The bitmap.
	 * \return pointer to bitmap's pixel buffer.
	 */
	uint8_t* (*get_buffer)(nsgif_bitmap_t *bitmap);

	/* The following functions are optional. */

	/**
	 * Set whether a bitmap can be plotted opaque.
	 *
	 * \param[in]  bitmap  The bitmap.
	 * \param[in]  opaque  Whether the current frame is opaque.
	 */
	void (*set_opaque)(nsgif_bitmap_t *bitmap, bool opaque);

	/**
	 * Tests whether a bitmap has an opaque alpha channel.
	 *
	 * \param[in]  bitmap  The bitmap.
	 * \return true if the bitmap is opaque, false otherwise.
	 */
	bool (*test_opaque)(nsgif_bitmap_t *bitmap);

	/**
	 * Bitmap modified notification.
	 *
	 * \param[in]  bitmap  The bitmap.
	 */
	void (*modified)(nsgif_bitmap_t *bitmap);
} nsgif_bitmap_cb_vt;

/**
 * Create the NSGIF object.
 *
 * \param[in]  bitmap_vt  Bitmap operation functions v-table.
 * \param[out] gif_out    Return NSGIF object on success.
 * \return NSGIF_OK on success, or appropriate error otherwise.
 */
nsgif_result nsgif_create(const nsgif_bitmap_cb_vt *bitmap_vt, nsgif **gif_out);

/**
 * Scan the source image data.
 *
 * This is used to feed the source data into LibNSGIF. This must be called
 * before calling \ref nsgif_frame_decode.
 *
 * It can be called multiple times with, with increasing sizes. If it is called
 * several times, as more data is available (e.g. slow network fetch) the data
 * already given to \ref nsgif_data_scan must be provided each time.
 *
 * For example, if you call \ref nsgif_data_scan with 25 bytes of data, and then
 * fetch another 10 bytes, you would need to call \ref nsgif_data with a size of
 * 35 bytes, and the whole 35 bytes must be contiguous memory. It is safe to
 * `realloc` the source buffer between calls to \ref nsgif_data_scan. (The
 * actual data pointer is allowed to be different.)
 *
 * If an error occurs, all previously scanned frames are retained.
 *
 * \param[in]  gif     The NSGIF object.
 * \param[in]  size    Number of bytes in data.
 * \param[in]  data    Raw source GIF data.
 *
 * \return Error return value.
 *         - NSGIF_FRAME_DATA_ERROR for GIF frame data error
 *         - NSGIF_INSUFFICIENT_DATA reached unexpected end of source data
 *         - NSGIF_INSUFFICIENT_MEMORY for memory error
 *         - NSGIF_DATA_ERROR for GIF error
 *         - NSGIF_OK for successful decoding
 *         - NSGIF_WORKING for successful decoding if more frames are expected
 */
nsgif_result nsgif_data_scan(
		nsgif *gif,
		size_t size,
		const uint8_t *data);

/**
 * Prepare to show a frame.
 *
 * \param[in]  gif        The NSGIF object.
 * \param[out] area       The area in pixels that must be redrawn.
 * \param[out] delay_cs   Time to wait after frame_new before next frame in cs.
 * \param[out] frame_new  The frame to decode.
 */
nsgif_result nsgif_frame_prepare(
		nsgif *gif,
		nsgif_rect *area,
		uint32_t *delay_cs,
		uint32_t *frame_new);

/**
 * Decodes a GIF frame.
 *
 * \param[in]  gif     The nsgif object.
 * \param[in]  frame   The frame number to decode.
 * \param[out] bitmap  On success, returns pointer to the client-allocated,
 *                     nsgif-owned client bitmap structure.
 * \return Error return value.
 *         - NSGIF_FRAME_DATA_ERROR for GIF frame data error
 *         - NSGIF_DATA_ERROR for GIF error (invalid frame header)
 *         - NSGIF_INSUFFICIENT_DATA reached unexpected end of source data
 *         - NSGIF_INSUFFICIENT_MEMORY for insufficient memory to process
 *         - NSGIF_OK for successful decoding
 */
nsgif_result nsgif_frame_decode(
		nsgif *gif,
		uint32_t frame,
		nsgif_bitmap_t **bitmap);

/**
 * Reset a GIF animation.
 *
 * Some animations are only meant to loop N times, and then show the
 * final frame forever. This function resets the loop and frame counters,
 * so that the animation can be replayed without the overhead of recreating
 * the NSGIF object and rescanning the raw data.
 *
 * \param[in]  gif  A NSGIF object.
 *
 * \return NSGIF_OK on success, or appropriate error otherwise.
 */
nsgif_result nsgif_reset(
		nsgif *gif);

/**
 * Get information about a GIF from an NSGIF object.
 *
 * \param[in]  gif  The NSGIF object to get info for.
 *
 * \return The gif info, or NULL on error.
 */
const nsgif_info_t *nsgif_get_info(const nsgif *gif);

/**
 * Free a NSGIF object.
 *
 * \param[in]  gif  The NSGIF to free.
 */
void nsgif_destroy(nsgif *gif);

#endif
