#include "sound_chef/sound_chef_ring_buffer.h"

#include <string.h>

// Occupancy is (write - read) as unsigned; wrap is well-defined as long as
// true occupancy never exceeds capacity, which sc_rb_can_reserve enforces.

static MA_INLINE sc_bool sc_rb_is_empty(size_t committedWrite, size_t read)
{
    return committedWrite == read;
}

static MA_INLINE sc_bool sc_rb_can_reserve(size_t reserveWrite, size_t read, size_t capacity, size_t size)
{
    return (reserveWrite - read) + size <= capacity;
}

static MA_INLINE sc_bool sc_rb_can_read(size_t committedWrite, size_t read, size_t size)
{
    return (committedWrite - read) >= size;
}

sbk_status SC_API sc_ring_buffer_init(sc_ring_buffer* rb, size_t requestedSize, const ma_allocation_callbacks* allocationCallbacks)
{
    SC_CHECK_ARG(rb != NULL);
    SC_CHECK_ARG(requestedSize > 2);
    SC_CHECK(rb->buffer == NULL, SBK_ERR_ALREADY_INITIALIZED);
    SC_CHECK(rb->capacity == 0, SBK_ERR_ALREADY_INITIALIZED);

    const size_t capacity = sc_next_pow2(requestedSize);
    SC_ASSERT(sc_is_pow2(capacity));

    rb->buffer = (sc_uint8*)ma_malloc(capacity, allocationCallbacks);
    SC_CHECK_MEM(rb->buffer);

    // Copy so uninit doesn't depend on the caller keeping the struct alive.
    if (allocationCallbacks != NULL)
    {
        rb->allocationCallbacks = *allocationCallbacks;
    }
    rb->capacity = capacity;
    rb->mask     = capacity - 1;

    c89atomic_store_explicit_64(&rb->reserveWriteIndex,   0, c89atomic_memory_order_relaxed);
    c89atomic_store_explicit_64(&rb->committedWriteIndex, 0, c89atomic_memory_order_relaxed);
    c89atomic_store_explicit_64(&rb->readIndex,           0, c89atomic_memory_order_relaxed);

    return SBK_SUCCESS;
}

void SC_API sc_ring_buffer_uninit(sc_ring_buffer* rb)
{
    if (rb == NULL)
    {
        return;
    }
    if (rb->buffer != NULL)
    {
        // onFree == NULL means init got NULL callbacks; pass NULL through so ma_free uses its defaults.
        const ma_allocation_callbacks* pFreeCallbacks = (rb->allocationCallbacks.onFree != NULL) ? &rb->allocationCallbacks : NULL;
        ma_free(rb->buffer, pFreeCallbacks);
    }
    MA_ZERO_OBJECT(rb);
}

sbk_status SC_API sc_ring_buffer_reserve_write(sc_ring_buffer* rb,
                                               size_t          size,
                                               sc_uint8**      outBuffer,
                                               size_t*         outReserveIndex,
                                               sc_uint8**      outPadding,
                                               size_t*         outPaddingSize)
{
    SC_CHECK_ARG(rb != NULL);
    SC_CHECK_ARG(outBuffer != NULL);
    SC_CHECK_ARG(outReserveIndex != NULL);
    SC_CHECK_ARG(outPadding != NULL);
    SC_CHECK_ARG(outPaddingSize != NULL);
    SC_CHECK_ARG(size != 0);
    SC_CHECK(rb->buffer != NULL && rb->capacity > 0, SBK_ERR_UNITIALIZED);
    SC_CHECK_ARG(size <= rb->capacity);

    *outPadding     = NULL;
    *outPaddingSize = 0;

    for (;;)
    {
        // Acquire pairs with the reader's release-store on readIndex.
        const size_t read         = (size_t)c89atomic_load_explicit_64(&rb->readIndex, c89atomic_memory_order_acquire);
        size_t       reserveWrite = (size_t)c89atomic_load_explicit_64(&rb->reserveWriteIndex, c89atomic_memory_order_relaxed);

        const size_t writeOffset           = reserveWrite & rb->mask;
        const size_t spaceFromWriteTillEnd = rb->capacity - writeOffset;
        const sc_bool needsWrapping        = size > spaceFromWriteTillEnd;

        // Wrap reservations pay for the end-of-buffer padding too.
        const size_t reserveSize = needsWrapping ? (size + spaceFromWriteTillEnd) : size;

        if (!sc_rb_can_reserve(reserveWrite, read, rb->capacity, reserveSize))
        {
            return SBK_ERR_FULL;
        }

        if (c89atomic_compare_exchange_weak_explicit_64(&rb->reserveWriteIndex,
                                                        &reserveWrite,
                                                        reserveWrite + reserveSize,
                                                        c89atomic_memory_order_relaxed,
                                                        c89atomic_memory_order_relaxed))
        {
            *outReserveIndex = reserveWrite;

            if (needsWrapping)
            {
                *outBuffer      = rb->buffer;
                *outPadding     = rb->buffer + writeOffset;
                *outPaddingSize = spaceFromWriteTillEnd;
            }
            else
            {
                *outBuffer = rb->buffer + writeOffset;
            }
            return SBK_SUCCESS;
        }
    }
}

sbk_status SC_API sc_ring_buffer_commit_write(sc_ring_buffer* rb, size_t reserveIndex, size_t size)
{
    SC_CHECK_ARG(rb != NULL);
    SC_CHECK(rb->buffer != NULL && rb->capacity > 0, SBK_ERR_UNITIALIZED);

    // Publish in reserve order — see the commit-ordering note in sc_ring_buffer.h.
    size_t expected = reserveIndex;
    while (!c89atomic_compare_exchange_weak_explicit_64(&rb->committedWriteIndex,
                                                        &expected,
                                                        reserveIndex + size,
                                                        c89atomic_memory_order_release,
                                                        c89atomic_memory_order_relaxed))
    {
        expected = reserveIndex;
        SC_PAUSE();
    }
    return SBK_SUCCESS;
}

sbk_status SC_API sc_ring_buffer_write(sc_ring_buffer* rb, const void* message, size_t size)
{
    SC_CHECK_ARG(rb != NULL);
    SC_CHECK_ARG(message != NULL);
    SC_CHECK_ARG(size != 0);
    SC_CHECK(rb->buffer != NULL && rb->capacity > 0, SBK_ERR_UNITIALIZED);
    SC_CHECK_ARG(size <= rb->capacity);

    for (;;)
    {
        const size_t read         = (size_t)c89atomic_load_explicit_64(&rb->readIndex, c89atomic_memory_order_acquire);
        size_t       reserveWrite = (size_t)c89atomic_load_explicit_64(&rb->reserveWriteIndex, c89atomic_memory_order_relaxed);

        if (!sc_rb_can_reserve(reserveWrite, read, rb->capacity, size))
        {
            return SBK_ERR_FULL;
        }

        if (c89atomic_compare_exchange_weak_explicit_64(&rb->reserveWriteIndex,
                                                        &reserveWrite,
                                                        reserveWrite + size,
                                                        c89atomic_memory_order_relaxed,
                                                        c89atomic_memory_order_relaxed))
        {
            const size_t writeOffset    = reserveWrite & rb->mask;
            const size_t firstChunkSize = (size <= rb->capacity - writeOffset) ? size : (rb->capacity - writeOffset);
            const sc_bool needsWrapping = firstChunkSize < size;

            memcpy(rb->buffer + writeOffset, message, firstChunkSize);
            if (needsWrapping)
            {
                memcpy(rb->buffer, (const sc_uint8*)message + firstChunkSize, size - firstChunkSize);
            }

            size_t expected = reserveWrite;
            while (!c89atomic_compare_exchange_weak_explicit_64(&rb->committedWriteIndex,
                                                                &expected,
                                                                reserveWrite + size,
                                                                c89atomic_memory_order_release,
                                                                c89atomic_memory_order_relaxed))
            {
                expected = reserveWrite;
                SC_PAUSE();
            }
            return SBK_SUCCESS;
        }
    }
}

sbk_status SC_API sc_ring_buffer_read_begin(sc_ring_buffer* rb,
                                            size_t          size,
                                            sc_uint8**      outBuffer,
                                            size_t*         outReadIndex,
                                            size_t*         outActualSize)
{
    SC_CHECK_ARG(rb != NULL);
    SC_CHECK_ARG(outBuffer != NULL);
    SC_CHECK_ARG(outReadIndex != NULL);
    SC_CHECK_ARG(outActualSize != NULL);
    SC_CHECK_ARG(size != 0);
    SC_CHECK(rb->buffer != NULL && rb->capacity > 0, SBK_ERR_UNITIALIZED);
    SC_CHECK_ARG(size <= rb->capacity);

    const size_t read           = (size_t)c89atomic_load_explicit_64(&rb->readIndex, c89atomic_memory_order_relaxed);
    // Acquire pairs with the producer's release-store on committedWriteIndex.
    const size_t committedWrite = (size_t)c89atomic_load_explicit_64(&rb->committedWriteIndex, c89atomic_memory_order_acquire);

    SC_CHECK(!sc_rb_is_empty(committedWrite, read), SBK_ERR_EMPTY);
    SC_CHECK(sc_rb_can_read(committedWrite, read, size), SBK_ERR_TOO_LARGE);

    const size_t readOffset     = read & rb->mask;
    const size_t firstChunkSize = (size <= rb->capacity - readOffset) ? size : (rb->capacity - readOffset);

    *outBuffer     = rb->buffer + readOffset;
    *outReadIndex  = read;
    *outActualSize = firstChunkSize;
    return SBK_SUCCESS;
}

sbk_status SC_API sc_ring_buffer_read_end(sc_ring_buffer* rb, size_t size)
{
    SC_CHECK_ARG(rb != NULL);
    SC_CHECK(rb->buffer != NULL && rb->capacity > 0, SBK_ERR_UNITIALIZED);
    // Release so producers that acquire-load readIndex see the freed space.
    c89atomic_fetch_add_explicit_64(&rb->readIndex, size, c89atomic_memory_order_release);
    return SBK_SUCCESS;
}

sbk_status SC_API sc_ring_buffer_read(sc_ring_buffer* rb, void* outBuffer, size_t size)
{
    SC_CHECK_ARG(rb != NULL);
    SC_CHECK_ARG(outBuffer != NULL);
    SC_CHECK_ARG(size != 0);
    SC_CHECK(rb->buffer != NULL && rb->capacity > 0, SBK_ERR_UNITIALIZED);
    SC_CHECK_ARG(size <= rb->capacity);

    const size_t read           = (size_t)c89atomic_load_explicit_64(&rb->readIndex, c89atomic_memory_order_relaxed);
    const size_t committedWrite = (size_t)c89atomic_load_explicit_64(&rb->committedWriteIndex, c89atomic_memory_order_acquire);

    SC_CHECK(!sc_rb_is_empty(committedWrite, read), SBK_ERR_EMPTY);
    SC_CHECK(sc_rb_can_read(committedWrite, read, size), SBK_ERR_TOO_LARGE);

    const size_t readOffset     = read & rb->mask;
    const size_t firstChunkSize = (size <= rb->capacity - readOffset) ? size : (rb->capacity - readOffset);
    const sc_bool needsWrapping = firstChunkSize < size;

    memcpy(outBuffer, rb->buffer + readOffset, firstChunkSize);
    if (needsWrapping)
    {
        memcpy((sc_uint8*)outBuffer + firstChunkSize, rb->buffer, size - firstChunkSize);
    }

    c89atomic_store_explicit_64(&rb->readIndex, read + size, c89atomic_memory_order_release);
    return SBK_SUCCESS;
}

sbk_status SC_API sc_ring_buffer_advance_read_index(sc_ring_buffer* rb, size_t size)
{
    SC_CHECK_ARG(rb != NULL);
    SC_CHECK_ARG(size != 0);
    SC_CHECK(rb->buffer != NULL && rb->capacity > 0, SBK_ERR_UNITIALIZED);
    SC_CHECK_ARG(size <= rb->capacity);

    const size_t read           = (size_t)c89atomic_load_explicit_64(&rb->readIndex, c89atomic_memory_order_relaxed);
    const size_t committedWrite = (size_t)c89atomic_load_explicit_64(&rb->committedWriteIndex, c89atomic_memory_order_acquire);

    SC_CHECK(!sc_rb_is_empty(committedWrite, read), SBK_ERR_EMPTY);
    SC_CHECK(sc_rb_can_read(committedWrite, read, size), SBK_ERR_TOO_LARGE);

    c89atomic_store_explicit_64(&rb->readIndex, read + size, c89atomic_memory_order_release);
    return SBK_SUCCESS;
}

size_t SC_API sc_ring_buffer_get_capacity(const sc_ring_buffer* rb)
{
    if (rb == NULL)
    {
        return 0;
    }
    return rb->capacity;
}
