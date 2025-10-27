#include "frame_queue.h"
#include "api.h"
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>

/**
 * Frame buffer state
 */
typedef enum {
    FRAME_STATE_FREE,       // Available for writing
    FRAME_STATE_WRITING,    // Being written by producer
    FRAME_STATE_READY,      // Ready for rendering
    FRAME_STATE_RENDERING,  // Being rendered by consumer
} frame_state_t;

/**
 * Individual frame buffer
 */
typedef struct {
    uint8_t* data;
    frame_state_t state;
    frame_info_t info;
} frame_buffer_t;

/**
 * Frame queue implementation
 */
struct frame_queue {
    // Configuration
    int capacity;
    int width;
    int height;
    int pitch;
    frame_format_t format;
    size_t buffer_size;

    // Frame buffers
    frame_buffer_t* frames;

    // Queue indices (circular buffer)
    atomic_int write_idx;   // Next slot for producer to write
    atomic_int read_idx;    // Next slot for consumer to read

    // Synchronization
    pthread_mutex_t mutex;
    pthread_cond_t frame_ready;     // Signaled when frame submitted
    pthread_cond_t frame_consumed;  // Signaled when frame released
    atomic_bool shutdown;

    // Statistics
    atomic_int frames_queued;
    atomic_uint_least64_t frames_submitted;
    atomic_uint_least64_t frames_dropped;
    atomic_uint_least64_t frames_rendered;
    atomic_uint_least64_t total_latency_us;

    // Timing
    uint64_t start_time_us;
};

/**
 * Get current time in microseconds
 */
static uint64_t get_time_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

/**
 * Create frame queue
 */
frame_queue_t* frame_queue_create(int width,
                                   int height,
                                   frame_format_t format,
                                   int capacity) {
    if (width <= 0 || height <= 0 || capacity < 2) {
        LOG_error("frame_queue_create: Invalid parameters (w=%d, h=%d, cap=%d)\n",
                  width, height, capacity);
        return NULL;
    }

    frame_queue_t* queue = calloc(1, sizeof(frame_queue_t));
    if (!queue) {
        LOG_error("frame_queue_create: Failed to allocate queue\n");
        return NULL;
    }

    queue->capacity = capacity;
    queue->width = width;
    queue->height = height;
    queue->format = format;
    queue->pitch = frame_format_pitch(width, format);
    queue->buffer_size = queue->pitch * height;
    queue->start_time_us = get_time_us();

    // Allocate frame buffers
    queue->frames = calloc(capacity, sizeof(frame_buffer_t));
    if (!queue->frames) {
        LOG_error("frame_queue_create: Failed to allocate frame array\n");
        free(queue);
        return NULL;
    }

    for (int i = 0; i < capacity; i++) {
        queue->frames[i].data = malloc(queue->buffer_size);
        if (!queue->frames[i].data) {
            LOG_error("frame_queue_create: Failed to allocate buffer %d\n", i);
            // Cleanup partial allocation
            for (int j = 0; j < i; j++) {
                free(queue->frames[j].data);
            }
            free(queue->frames);
            free(queue);
            return NULL;
        }
        queue->frames[i].state = FRAME_STATE_FREE;
        queue->frames[i].info.width = width;
        queue->frames[i].info.height = height;
        queue->frames[i].info.pitch = queue->pitch;
        queue->frames[i].info.format = format;
    }

    // Initialize synchronization
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->frame_ready, NULL);
    pthread_cond_init(&queue->frame_consumed, NULL);
    atomic_init(&queue->shutdown, false);
    atomic_init(&queue->write_idx, 0);
    atomic_init(&queue->read_idx, 0);
    atomic_init(&queue->frames_queued, 0);
    atomic_init(&queue->frames_submitted, 0);
    atomic_init(&queue->frames_dropped, 0);
    atomic_init(&queue->frames_rendered, 0);
    atomic_init(&queue->total_latency_us, 0);

    LOG_info("Created frame queue: %dx%d, format=%d, capacity=%d, buffer_size=%zu\n",
             width, height, format, capacity, queue->buffer_size);

    return queue;
}

/**
 * Destroy frame queue
 */
void frame_queue_destroy(frame_queue_t* queue) {
    if (!queue) return;

    LOG_info("Destroying frame queue\n");

    // Wake all threads
    frame_queue_shutdown(queue);

    // Cleanup synchronization primitives
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->frame_ready);
    pthread_cond_destroy(&queue->frame_consumed);

    // Free buffers
    for (int i = 0; i < queue->capacity; i++) {
        free(queue->frames[i].data);
    }
    free(queue->frames);
    free(queue);
}

/**
 * Signal shutdown
 */
void frame_queue_shutdown(frame_queue_t* queue) {
    if (!queue) return;

    atomic_store(&queue->shutdown, true);

    // Wake all waiting threads
    pthread_mutex_lock(&queue->mutex);
    pthread_cond_broadcast(&queue->frame_ready);
    pthread_cond_broadcast(&queue->frame_consumed);
    pthread_mutex_unlock(&queue->mutex);
}

/**
 * Check if shutdown
 */
int frame_queue_is_shutdown(frame_queue_t* queue) {
    return queue ? atomic_load(&queue->shutdown) : 1;
}

/**
 * Acquire frame for writing (producer)
 */
frame_handle_t frame_queue_acquire_write(frame_queue_t* queue) {
    if (!queue || atomic_load(&queue->shutdown)) {
        return FRAME_INVALID;
    }

    pthread_mutex_lock(&queue->mutex);

    // Find next free buffer
    int idx = atomic_load(&queue->write_idx);
    for (int i = 0; i < queue->capacity; i++) {
        int check_idx = (idx + i) % queue->capacity;
        if (queue->frames[check_idx].state == FRAME_STATE_FREE) {
            queue->frames[check_idx].state = FRAME_STATE_WRITING;
            queue->frames[check_idx].info.timestamp_us =
                get_time_us() - queue->start_time_us;
            pthread_mutex_unlock(&queue->mutex);
            return check_idx;
        }
    }

    // No free buffer available
    atomic_fetch_add(&queue->frames_dropped, 1);
    pthread_mutex_unlock(&queue->mutex);
    return FRAME_INVALID;
}

/**
 * Submit written frame (producer)
 */
void frame_queue_submit(frame_queue_t* queue, frame_handle_t handle) {
    if (!queue || handle < 0 || handle >= queue->capacity) return;

    pthread_mutex_lock(&queue->mutex);

    if (queue->frames[handle].state == FRAME_STATE_WRITING) {
        queue->frames[handle].state = FRAME_STATE_READY;
        atomic_fetch_add(&queue->frames_queued, 1);
        atomic_fetch_add(&queue->frames_submitted, 1);

        // Update write index for next acquisition
        atomic_store(&queue->write_idx, (handle + 1) % queue->capacity);

        // Signal consumer
        pthread_cond_signal(&queue->frame_ready);
    }

    pthread_mutex_unlock(&queue->mutex);
}

/**
 * Acquire frame for reading (consumer)
 */
frame_handle_t frame_queue_acquire_read(frame_queue_t* queue, int timeout_ms) {
    if (!queue) return FRAME_INVALID;

    pthread_mutex_lock(&queue->mutex);

    // Calculate timeout
    struct timespec ts;
    if (timeout_ms > 0) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        ts.tv_sec = tv.tv_sec + (timeout_ms / 1000);
        ts.tv_nsec = (tv.tv_usec + (timeout_ms % 1000) * 1000) * 1000;
        if (ts.tv_nsec >= 1000000000) {
            ts.tv_sec++;
            ts.tv_nsec -= 1000000000;
        }
    }

    // Wait for ready frame
    while (!atomic_load(&queue->shutdown)) {
        int idx = atomic_load(&queue->read_idx);
        if (queue->frames[idx].state == FRAME_STATE_READY) {
            queue->frames[idx].state = FRAME_STATE_RENDERING;
            atomic_fetch_sub(&queue->frames_queued, 1);
            pthread_mutex_unlock(&queue->mutex);
            return idx;
        }

        // Wait for frame or timeout
        int ret;
        if (timeout_ms > 0) {
            ret = pthread_cond_timedwait(&queue->frame_ready, &queue->mutex, &ts);
            if (ret == ETIMEDOUT) {
                pthread_mutex_unlock(&queue->mutex);
                return FRAME_INVALID;
            }
        } else if (timeout_ms < 0) {
            // Infinite wait
            pthread_cond_wait(&queue->frame_ready, &queue->mutex);
        } else {
            // Non-blocking
            pthread_mutex_unlock(&queue->mutex);
            return FRAME_INVALID;
        }
    }

    pthread_mutex_unlock(&queue->mutex);
    return FRAME_INVALID;
}

/**
 * Release rendered frame (consumer)
 */
void frame_queue_release(frame_queue_t* queue, frame_handle_t handle) {
    if (!queue || handle < 0 || handle >= queue->capacity) return;

    pthread_mutex_lock(&queue->mutex);

    if (queue->frames[handle].state == FRAME_STATE_RENDERING) {
        // Calculate latency
        uint64_t now = get_time_us() - queue->start_time_us;
        uint64_t latency = now - queue->frames[handle].info.timestamp_us;
        atomic_fetch_add(&queue->total_latency_us, latency);
        atomic_fetch_add(&queue->frames_rendered, 1);

        // Return to free pool
        queue->frames[handle].state = FRAME_STATE_FREE;

        // Update read index
        atomic_store(&queue->read_idx, (handle + 1) % queue->capacity);

        // Signal producer
        pthread_cond_signal(&queue->frame_consumed);
    }

    pthread_mutex_unlock(&queue->mutex);
}

/**
 * Get buffer pointer
 */
uint8_t* frame_queue_get_buffer(frame_queue_t* queue, frame_handle_t handle) {
    if (!queue || handle < 0 || handle >= queue->capacity) {
        return NULL;
    }
    return queue->frames[handle].data;
}

/**
 * Get frame info
 */
int frame_queue_get_info(frame_queue_t* queue,
                         frame_handle_t handle,
                         frame_info_t* info) {
    if (!queue || handle < 0 || handle >= queue->capacity || !info) {
        return -1;
    }

    pthread_mutex_lock(&queue->mutex);
    *info = queue->frames[handle].info;
    pthread_mutex_unlock(&queue->mutex);

    return 0;
}

/**
 * Get queue statistics
 */
void frame_queue_get_stats(frame_queue_t* queue,
                           int* frames_queued,
                           uint64_t* frames_dropped,
                           uint64_t* avg_latency_us) {
    if (!queue) return;

    if (frames_queued) {
        *frames_queued = atomic_load(&queue->frames_queued);
    }

    if (frames_dropped) {
        *frames_dropped = atomic_load(&queue->frames_dropped);
    }

    if (avg_latency_us) {
        uint64_t rendered = atomic_load(&queue->frames_rendered);
        uint64_t total_latency = atomic_load(&queue->total_latency_us);
        *avg_latency_us = rendered > 0 ? total_latency / rendered : 0;
    }
}

/**
 * Reset statistics
 */
void frame_queue_reset_stats(frame_queue_t* queue) {
    if (!queue) return;

    atomic_store(&queue->frames_submitted, 0);
    atomic_store(&queue->frames_dropped, 0);
    atomic_store(&queue->frames_rendered, 0);
    atomic_store(&queue->total_latency_us, 0);
    queue->start_time_us = get_time_us();
}
