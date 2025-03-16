#pragma once

#include <pthread.h>

class ReaderWriterLock {
    private:
        unsigned int readers;                // Number of active readers

    public:
        pthread_mutex_t read_lock;  // Mutex for read access
        pthread_mutex_t write_lock; // Mutex for write access

        ReaderWriterLock() : readers(0) {
            pthread_mutex_init(&read_lock, nullptr);
            pthread_mutex_init(&write_lock, nullptr);
        }

        void readLock() {
            pthread_mutex_lock(&read_lock);
            readers++;
            if (readers == 1) {
                pthread_mutex_lock(&write_lock); // First reader locks the write lock
            }
            pthread_mutex_unlock(&read_lock);

        }

        void readUnlock() {
            pthread_mutex_lock(&read_lock);
            readers--;
            if (readers == 0) {
                pthread_mutex_unlock(&write_lock); // Last reader unlocks the write lock
            }
            pthread_mutex_unlock(&read_lock);
        }

        void writeLock() {
            pthread_mutex_lock(&write_lock); // Lock for writing
        }

        void writeUnlock() {
            pthread_mutex_unlock(&write_lock); // Unlock for writing
        }


        ~ReaderWriterLock() {
            pthread_mutex_destroy(&read_lock);
            pthread_mutex_destroy(&write_lock);
        }

};


class WithWriteLock {
    private:
        ReaderWriterLock &rw_lock;

    public:
        WithWriteLock(ReaderWriterLock &lock) : rw_lock(lock) {
            rw_lock.writeLock();
        }

        ~WithWriteLock() {
            rw_lock.writeUnlock();
        }
};

class WithReadLock {
    private:
        ReaderWriterLock &rw_lock;

    public:
        WithReadLock(ReaderWriterLock &lock) : rw_lock(lock) {
            rw_lock.readLock();
        }

        ~WithReadLock() {
            rw_lock.readUnlock();
        }
};


