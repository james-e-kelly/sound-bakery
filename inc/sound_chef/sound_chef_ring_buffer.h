#ifndef SOUND_CHEF_RING_BUFFER
#define SOUND_CHEF_RING_BUFFER

#include "sound_chef/sound_chef.h"

#ifdef __cplusplus
extern "C"
{
#endif

// 64 covers x86_64 and most ARM cores. Some platforms (Apple M-series L2 pairs,
// some POWER chips) prefetch 128-byte pairs; bump if profiling shows false sharing.
#ifndef SC_CACHE_LINE_SIZE
    #define SC_CACHE_LINE_SIZE 64
#endif

/**
 * @brief Multi-producer, single-consumer byte ring buffer.
 *
 * Producers CAS-reserve a slot on @ref reserveWriteIndex, memcpy their bytes,
 * then publish by advancing @ref committedWriteIndex from their reserved
 * position to (position + size). Publication is strictly in reserve order:
 * a slow producer holds up any producer that reserved after it, until it
 * commits. In return the consumer only reads one atomic and knows everything
 * before that point is valid. Alternative schemes (per-slot committed flags,
 * Vyukov's bounded MPSC) trade the producer spin for consumer or metadata
 * complexity; for short command messages drained once per update the spin
 * window is memcpy-sized and this design wins.
 *
 * @warning A producer that reserves and never commits freezes every producer
 * behind it. Only well-behaved producers should hold the queue.
 *
 * @remark Capacity is rounded up to a power of two so the index can be masked.
 * @remark Indexes are 64-bit and grow forever; wrap-around is beyond any
 * realistic runtime and all occupancy comparisons are wrap-safe unsigned
 * differences.
 *
 * @see https://github.com/bowtoyourlord/MPSCQueue
 */
typedef struct sc_ring_buffer
{
    ma_allocation_callbacks     allocationCallbacks;    //< Kept for uninit; zeroed if init got NULL
    sc_uint8*                   buffer;
    size_t                      capacity;
    size_t                      mask;

    // Padded to their own cache lines to avoid false sharing between
    // producer- and consumer-side writes.
    SC_ALIGN_TO(SC_CACHE_LINE_SIZE) sc_atomic_uint64 reserveWriteIndex;
    SC_ALIGN_TO(SC_CACHE_LINE_SIZE) sc_atomic_uint64 committedWriteIndex;    //< Consumer reads only up to this
    SC_ALIGN_TO(SC_CACHE_LINE_SIZE) sc_atomic_uint64 readIndex;
} sc_ring_buffer;

/**
 * @brief Initialise a ring buffer.
 *
 * @param requestedSize         Rounded up to the next power of two, minimum 4.
 * @param allocationCallbacks   May be NULL to use miniaudio defaults.
 */
sbk_status SC_API sc_ring_buffer_init(sc_ring_buffer* rb, size_t requestedSize, const ma_allocation_callbacks* allocationCallbacks);

/**
 * @brief Release the backing storage. Safe on an already-uninitialised buffer.
 *
 * Not thread-safe with pending readers or writers; the caller must quiesce first.
 */
void SC_API sc_ring_buffer_uninit(sc_ring_buffer* rb);

/**
 * @brief Reserve a contiguous region for a producer to fill.
 *
 * When the request straddles the end of the buffer, the reservation also
 * covers the padding at the end. The producer's message goes into @r outBuffer
 * (at the start of the buffer for a wrap); @r outPadding lets the caller write
 * a skip marker over the padding so the consumer knows to jump past it.
 *
 * @remark Callable from any producer thread.
 */
sbk_status SC_API sc_ring_buffer_reserve_write(sc_ring_buffer* rb,
                                               size_t          size,
                                               sc_uint8**      outBuffer,
                                               size_t*         outReserveIndex,
                                               sc_uint8**      outPadding,
                                               size_t*         outPaddingSize);

/**
 * @brief Publish a reservation returned by @ref sc_ring_buffer_reserve_write.
 *
 * @param size  Bytes to publish, including any padding returned by the reserve.
 *
 * @warning Every reserve MUST be paired with a commit.
 *
 * @remark Callable from any producer thread.
 */
sbk_status SC_API sc_ring_buffer_commit_write(sc_ring_buffer* rb, size_t reserveIndex, size_t size);

/**
 * @brief Reserve, memcpy, and commit in one call.
 *
 * Splits the message across the end/start of the buffer if it doesn't fit
 * contiguously. @ref sc_ring_buffer_read handles the straddle automatically;
 * @ref sc_ring_buffer_read_begin callers must expect a partial return.
 *
 * @remark Callable from any producer thread.
 */
sbk_status SC_API sc_ring_buffer_write(sc_ring_buffer* rb, const void* message, size_t size);

/**
 * @brief Zero-copy read. Returns a pointer into the buffer.
 *
 * If the request straddles the buffer end, only the bytes up to the end are
 * returned via @r outActualSize; the consumer must call again for the rest.
 *
 * @remark Consumer thread only.
 */
sbk_status SC_API sc_ring_buffer_read_begin(sc_ring_buffer* rb,
                                            size_t          size,
                                            sc_uint8**      outBuffer,
                                            size_t*         outReadIndex,
                                            size_t*         outActualSize);

/** @remark Consumer thread only. */
sbk_status SC_API sc_ring_buffer_read_end(sc_ring_buffer* rb, size_t size);

/**
 * @brief Copy @r size bytes out of the buffer, handling any straddle.
 *
 * @remark Consumer thread only.
 */
sbk_status SC_API sc_ring_buffer_read(sc_ring_buffer* rb, void* outBuffer, size_t size);

/** @remark Consumer thread only. */
sbk_status SC_API sc_ring_buffer_advance_read_index(sc_ring_buffer* rb, size_t size);

size_t SC_API sc_ring_buffer_get_capacity(const sc_ring_buffer* rb);

#ifdef __cplusplus
}
#endif

#endif  // SOUND_CHEF_RING_BUFFER
