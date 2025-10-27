#ifndef FRAME_QUEUE_H
#define FRAME_QUEUE_H

/**
 * Thread-Safe Frame Queue for Rendering Pipeline
 *
 * This module implements a producer-consumer pattern for frame rendering,
 * addressing SDL2's requirement that creation and rendering occur on the
 * same thread. It enables threaded video processing while maintaining
 * compatibility with SDL's threading constraints.
 *
 * Problem Statement:
 * - SDL2 requires SDL_CreateRGBSurface() and SDL_Flip() to run in the same thread
 * - Some forks implement threaded rendering for performance but encounter conflicts
 * - Cores produce frames at variable rates, causing jitter if not properly buffered
 *
 * Solution:
 * - Producer (core thread): Writes rendered frames into queue
 * - Consumer (render thread): Reads frames and presents to display
 * - Triple buffering: Prevents tearing and allows smooth frame pacing
 * - Lock-free design: Uses atomics and mutexes only for synchronization, not data
 *
 * Usage Pattern:
 *
 *   // Initialization
 *   frame_queue_t* queue = frame_queue_create(width, height, format, 3);
 *
 *   // Producer thread (libretro core)
 *   void video_refresh_callback(const void* data, unsigned width,
 *                                unsigned height, size_t pitch) {
 *       frame_handle_t handle = frame_queue_acquire_write(queue);
 *       if (handle != FRAME_INVALID) {
 *           uint8_t* dst = frame_queue_get_buffer(queue, handle);
 *           memcpy(dst, data, height * pitch);
 *           frame_queue_submit(queue, handle);
 *       }
 *   }
 *
 *   // Consumer thread (render thread)
 *   void render_loop(void) {
 *       while (!shutdown) {
 *           frame_handle_t handle = frame_queue_acquire_read(queue, 16);  // 16ms timeout
 *           if (handle != FRAME_INVALID) {
 *               const uint8_t* buffer = frame_queue_get_buffer(queue, handle);
 *               backend->present(ctx, buffer, width, height, pitch);
 *               frame_queue_release(queue, handle);
 *           }
 *       }
 *   }
 *
 *   // Cleanup
 *   frame_queue_destroy(queue);
 */

#include <stdint.h>
#include <stddef.h>
#include <pthread.h>
#include <stdatomic.h>

// Pixel format for frame buffers
typedef enum {
    FRAME_FORMAT_RGB565,
    FRAME_FORMAT_BGR565,
    FRAME_FORMAT_XRGB8888,
    FRAME_FORMAT_ARGB8888,
} frame_format_t;

// Opaque frame handle
typedef int frame_handle_t;
#define FRAME_INVALID -1

// Frame metadata
typedef struct {
    int width;
    int height;
    int pitch;
    frame_format_t format;
    uint64_t timestamp_us;  // Microseconds since queue creation
} frame_info_t;

// Frame queue context
typedef struct frame_queue frame_queue_t;

/**
 * Create a frame queue with specified capacity.
 *
 * @param width Frame width in pixels
 * @param height Frame height in pixels
 * @param format Pixel format
 * @param capacity Number of frame buffers (typically 2-3)
 * @return Frame queue handle, or NULL on failure
 *
 * @note Capacity should be 2 (double buffer) or 3 (triple buffer).
 *       Higher values increase latency without improving smoothness.
 */
frame_queue_t* frame_queue_create(int width,
                                   int height,
                                   frame_format_t format,
                                   int capacity);

/**
 * Destroy frame queue and release all resources.
 *
 * @param queue Frame queue to destroy
 *
 * @note This will block until all pending frames are released.
 *       Call frame_queue_shutdown() first to wake blocked threads.
 */
void frame_queue_destroy(frame_queue_t* queue);

/**
 * Signal shutdown to wake blocked threads.
 *
 * @param queue Frame queue
 *
 * @note After calling this, all blocking operations will return
 *       FRAME_INVALID immediately. Useful for graceful shutdown.
 */
void frame_queue_shutdown(frame_queue_t* queue);

/**
 * Check if queue is in shutdown state.
 *
 * @param queue Frame queue
 * @return Non-zero if shutdown, zero otherwise
 */
int frame_queue_is_shutdown(frame_queue_t* queue);

/**
 * Acquire a frame buffer for writing (producer).
 *
 * @param queue Frame queue
 * @return Frame handle, or FRAME_INVALID if no buffer available
 *
 * @note This is non-blocking. If all buffers are in use, returns
 *       FRAME_INVALID immediately. Producer should drop the frame
 *       or wait briefly before retrying.
 */
frame_handle_t frame_queue_acquire_write(frame_queue_t* queue);

/**
 * Submit a written frame for rendering (producer).
 *
 * @param queue Frame queue
 * @param handle Frame handle from frame_queue_acquire_write()
 *
 * @note After calling this, the buffer belongs to the queue and
 *       should not be accessed by the producer until re-acquired.
 */
void frame_queue_submit(frame_queue_t* queue, frame_handle_t handle);

/**
 * Acquire the next frame for reading (consumer).
 *
 * @param queue Frame queue
 * @param timeout_ms Maximum time to wait in milliseconds, or 0 for non-blocking
 * @return Frame handle, or FRAME_INVALID on timeout or shutdown
 *
 * @note This blocks until a frame is available or timeout expires.
 *       For 60 FPS, use timeout_ms=16 (one frame period).
 */
frame_handle_t frame_queue_acquire_read(frame_queue_t* queue, int timeout_ms);

/**
 * Release a rendered frame (consumer).
 *
 * @param queue Frame queue
 * @param handle Frame handle from frame_queue_acquire_read()
 *
 * @note After calling this, the buffer returns to the free pool
 *       and may be overwritten by the producer.
 */
void frame_queue_release(frame_queue_t* queue, frame_handle_t handle);

/**
 * Get pointer to frame buffer data.
 *
 * @param queue Frame queue
 * @param handle Frame handle
 * @return Pointer to buffer, or NULL if handle invalid
 *
 * @note The pointer is valid as long as the handle is held
 *       (between acquire and release/submit).
 */
uint8_t* frame_queue_get_buffer(frame_queue_t* queue, frame_handle_t handle);

/**
 * Get frame metadata.
 *
 * @param queue Frame queue
 * @param handle Frame handle
 * @param info Output parameter for frame info
 * @return 0 on success, negative on error
 */
int frame_queue_get_info(frame_queue_t* queue,
                         frame_handle_t handle,
                         frame_info_t* info);

/**
 * Get queue statistics for debugging and profiling.
 *
 * @param queue Frame queue
 * @param frames_queued Output: Number of frames waiting to be rendered
 * @param frames_dropped Output: Total frames dropped due to full queue
 * @param avg_latency_us Output: Average frame latency in microseconds
 *
 * @note Statistics are approximate and updated atomically.
 */
void frame_queue_get_stats(frame_queue_t* queue,
                           int* frames_queued,
                           uint64_t* frames_dropped,
                           uint64_t* avg_latency_us);

/**
 * Reset queue statistics.
 *
 * @param queue Frame queue
 */
void frame_queue_reset_stats(frame_queue_t* queue);

/**
 * Helper: Calculate bytes per pixel for format.
 *
 * @param format Pixel format
 * @return Bytes per pixel (2 or 4)
 */
static inline int frame_format_bpp(frame_format_t format) {
    switch (format) {
        case FRAME_FORMAT_RGB565:
        case FRAME_FORMAT_BGR565:
            return 2;
        case FRAME_FORMAT_XRGB8888:
        case FRAME_FORMAT_ARGB8888:
            return 4;
        default:
            return 0;
    }
}

/**
 * Helper: Calculate pitch (bytes per scanline) for format.
 *
 * @param width Frame width in pixels
 * @param format Pixel format
 * @return Pitch in bytes
 */
static inline int frame_format_pitch(int width, frame_format_t format) {
    return width * frame_format_bpp(format);
}

/**
 * Integration with existing code:
 *
 * In minarch.c, replace direct rendering with frame queue:
 *
 * // Global queue
 * static frame_queue_t* video_queue = NULL;
 * static pthread_t render_thread;
 *
 * // Initialize during video setup
 * void init_video(int width, int height) {
 *     video_queue = frame_queue_create(width, height, FRAME_FORMAT_RGB565, 3);
 *     pthread_create(&render_thread, NULL, render_thread_func, NULL);
 * }
 *
 * // Libretro video callback (producer - runs on core thread)
 * void retro_video_refresh(const void* data, unsigned width,
 *                          unsigned height, size_t pitch) {
 *     frame_handle_t handle = frame_queue_acquire_write(video_queue);
 *     if (handle != FRAME_INVALID) {
 *         uint8_t* dst = frame_queue_get_buffer(video_queue, handle);
 *         memcpy(dst, data, height * pitch);
 *         frame_queue_submit(video_queue, handle);
 *     } else {
 *         // Queue full, drop frame (rare)
 *         LOG_warn("Dropped frame: queue full\n");
 *     }
 * }
 *
 * // Render thread (consumer - dedicated SDL thread)
 * void* render_thread_func(void* arg) {
 *     while (!quit) {
 *         frame_handle_t handle = frame_queue_acquire_read(video_queue, 16);
 *         if (handle != FRAME_INVALID) {
 *             const uint8_t* buffer = frame_queue_get_buffer(video_queue, handle);
 *             frame_info_t info;
 *             frame_queue_get_info(video_queue, handle, &info);
 *
 *             // Present using backend or SDL
 *             backend->present(ctx, buffer, info.width, info.height, info.pitch);
 *
 *             frame_queue_release(video_queue, handle);
 *         }
 *     }
 *     return NULL;
 * }
 *
 * // Cleanup
 * void shutdown_video(void) {
 *     frame_queue_shutdown(video_queue);
 *     pthread_join(render_thread, NULL);
 *     frame_queue_destroy(video_queue);
 * }
 */

#endif // FRAME_QUEUE_H
