#pragma once

#include <cstdint>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

namespace satdump
{
    /*
    Simple generic FIFO implementation using the pipe Unix / Linux and Windows implementation.
    */
    class Pipe
    {
    private:
        // File handles
        int fifo_handles[2];
        // Util values
        int read_cnt;
        int read_tries;
        bool detroyed = false;

    public:
        // The buffer size is only used on Windows currently
        Pipe(int buffer_size = 4096)
        {
#ifdef _WIN32
            _pipe(fifo_handles, buffer_size * 10, 'b');
#else
            pipe(fifo_handles);
            buffer_size = buffer_size; // Remove an annoying warning
#endif
        }
        // Close everything
        ~Pipe()
        {
            if (!detroyed)
            {
                if (fifo_handles[1])
                    close(fifo_handles[1]);
                if (fifo_handles[0])
                    close(fifo_handles[0]);
            }

            detroyed = true;
        }
        // Push x samples into the fifo
        void push(uint8_t *buffer, int size, int type_size)
        {
            write(fifo_handles[1], buffer, size * type_size);
        }
        // Read x samples from the fifo, trying to fill the request
        int pop(uint8_t *buffer, int size, int type_size, int tries = 100)
        {
            read_cnt = 0;
            read_tries = 0;
            while (read_cnt < size && read_tries < tries)
            {
                read_cnt += read(fifo_handles[0], &buffer[read_cnt], (size - read_cnt) * type_size) / type_size;
                read_tries++;
            }
            return read_cnt;
        }
        // Read x samples from the fifo, with what's in the buffer
        int pop_eager(uint8_t *buffer, int type_size, int size)
        {
            return read(fifo_handles[0], buffer, size * type_size) / type_size;
        }
    };
}; // namespace satdump

// Ring buffer
#include <mutex>
#include <condition_variable>
#include <string.h>

#define RING_BUF_SZ 1000000

class RingBuffer
{
public:
    RingBuffer()
    {
    }

    RingBuffer(int maxLatency) { init(maxLatency); }

    ~RingBuffer() { delete _buffer; }

    void init(int maxLatency)
    {
        size = RING_BUF_SZ;
        _buffer = new uint8_t[size];
        _stopReader = false;
        _stopWriter = false;
        this->maxLatency = maxLatency;
        writec = 0;
        readc = 0;
        readable = 0;
        writable = size;
        memset(_buffer, 0, size /** sizeof(T)*/);
    }

    int read(uint8_t *data, int tlen, int len)
    {
        int dataRead = 0;
        int toRead = 0;
        while (dataRead < len)
        {
            toRead = std::min<int>(waitUntilReadable(), len - dataRead);
            if (toRead < 0)
            {
                return -1;
            };

            if ((toRead + readc) > size)
            {
                memcpy(&data[dataRead], &_buffer[readc], (size - readc) * tlen);
                memcpy(&data[dataRead + (size - readc)], &_buffer[0], (toRead - (size - readc)) * tlen);
            }
            else
            {
                memcpy(&data[dataRead], &_buffer[readc], toRead * tlen);
            }

            dataRead += toRead;

            _readable_mtx.lock();
            readable -= toRead;
            _readable_mtx.unlock();
            _writable_mtx.lock();
            writable += toRead;
            _writable_mtx.unlock();
            readc = (readc + toRead) % size;
            canWriteVar.notify_one();
        }
        return len;
    }

    int waitUntilReadable()
    {
        if (_stopReader)
        {
            return -1;
        }
        int _r = getReadable();
        if (_r != 0)
        {
            return _r;
        }
        std::unique_lock<std::mutex> lck(_readable_mtx);
        canReadVar.wait(lck, [=]() { return ((this->getReadable(false) > 0) || this->getReadStop()); });
        if (_stopReader)
        {
            return -1;
        }
        return getReadable(false);
    }

    int getReadable(bool lock = true)
    {
        if (lock)
        {
            _readable_mtx.lock();
        };
        int _r = readable;
        if (lock)
        {
            _readable_mtx.unlock();
        };
        return _r;
    }

    int write(uint8_t *data, int tlen, int len)
    {
        int dataWritten = 0;
        int toWrite = 0;
        while (dataWritten < len)
        {
            toWrite = std::min<int>(waitUntilwritable(), len - dataWritten);
            if (toWrite < 0)
            {
                return -1;
            };

            if ((toWrite + writec) > size)
            {
                memcpy(&_buffer[writec], &data[dataWritten], (size - writec) * tlen);
                memcpy(&_buffer[0], &data[dataWritten + (size - writec)], (toWrite - (size - writec)) * tlen);
            }
            else
            {
                memcpy(&_buffer[writec], &data[dataWritten], toWrite * tlen);
            }

            dataWritten += toWrite;

            _readable_mtx.lock();
            readable += toWrite;
            _readable_mtx.unlock();
            _writable_mtx.lock();
            writable -= toWrite;
            _writable_mtx.unlock();
            writec = (writec + toWrite) % size;

            canReadVar.notify_one();
        }
        return len;
    }

    int waitUntilwritable()
    {
        if (_stopWriter)
        {
            return -1;
        }
        int _w = getWritable();
        if (_w != 0)
        {
            return _w;
        }
        std::unique_lock<std::mutex> lck(_writable_mtx);
        canWriteVar.wait(lck, [=]() { return ((this->getWritable(false) > 0) || this->getWriteStop()); });
        if (_stopWriter)
        {
            return -1;
        }
        return getWritable(false);
    }

    int getWritable(bool lock = true)
    {
        if (lock)
        {
            _writable_mtx.lock();
        };
        int _w = writable;
        if (lock)
        {
            _writable_mtx.unlock();
            _readable_mtx.lock();
        };
        int _r = readable;
        if (lock)
        {
            _readable_mtx.unlock();
        };
        return std::max<int>(std::min<int>(_w, maxLatency - _r), 0);
    }

    void stopReader()
    {
        _stopReader = true;
        canReadVar.notify_one();
    }

    void stopWriter()
    {
        _stopWriter = true;
        canWriteVar.notify_one();
    }

    bool getReadStop()
    {
        return _stopReader;
    }

    bool getWriteStop()
    {
        return _stopWriter;
    }

    void clearReadStop()
    {
        _stopReader = false;
    }

    void clearWriteStop()
    {
        _stopWriter = false;
    }

    void setMaxLatency(int maxLatency)
    {
        this->maxLatency = maxLatency;
    }

private:
    uint8_t *_buffer;
    int size;
    int readc;
    int writec;
    int readable;
    int writable;
    int maxLatency;
    bool _stopReader;
    bool _stopWriter;
    std::mutex _readable_mtx;
    std::mutex _writable_mtx;
    std::condition_variable canReadVar;
    std::condition_variable canWriteVar;
};
