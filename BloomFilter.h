#ifndef BLOOMFILTER_H
#define BLOOMFILTER_H

#include <cmath>
// #include <string.h>
// #include <openssl/md5.h>Go to your project folder and make sure there's a file at:


#include <fcntl.h>
#include <sys/stat.h>

#include <cf/vec.h>
#include <cf/crypto.h>

#include "../frontier/ReaderWriterLock.h"

#include <utility>
#include <cassert>




class Bloomfilter
   {
   public:
    
      Bloomfilter()
         {
            this->num_hashes = 0;
         }

      Bloomfilter(bool construct) {
         if (!construct) {
            this->num_hashes = 0;
            return;
         } 

         // Determine the size of bits of our data vector, and resize.

         // Use the formula: m = - (n * log(p)) / (log(2)^2)

         const unsigned int optimized_size = (int)(- (NUM_OBJECTS * log(FALSE_POSITIVE_RATE)) / (log(2) * log(2)));

         // Determine number of hash functions to use.
         const unsigned int n = (int)( (optimized_size / NUM_OBJECTS) * log(2) );
         
         this->num_hashes = n;
         bits.resize( optimized_size, false ); 
         
      }

         ~Bloomfilter() {
         }

         int buildBloomFilter( const char * path ) {


            int fd = open(path, O_RDONLY );
            if (fd == -1) {
               std::cerr << "Error opening bloom filter";
               return 1;
            }
            struct stat sb;
            if (fstat(fd, &sb) == -1) {
               perror("Error getting file size");
               close(fd);
               return 1;
            }
            int fsize = sb.st_size;
            int pos = 0;
            while (pos < bits.size() / 8 && pos < fsize) {
               read(fd, bits.data() + pos, 1);
               pos += 1;
            } 

            close(fd);
            return 0;
         }

         int writeBloomFilter() {
            WithWriteLock wl(bloom_lock); 
            int fd = open("./log/frontier/bloomfilter.bin", O_TRUNC | O_RDWR );
            if (fd == -1) {
               std::cerr << "Error opening bloom filter";
               return 1;
            }

            struct stat sb;
            if (fstat(fd, &sb) == -1) {
               perror("Error getting file size");
               close(fd);
               return 1;
            }
            int fsize = sb.st_size;

            int pos = 0;
            while (pos < bits.size() - 8) {
               uint8_t byte_value = 0;
               for (int i = 0; i < 8; ++i) {
                  if (bits[pos + i]) {
                     byte_value |= (1 << i); // Set the i-th bit if bool_array[i] is true
                  }
               }
               write(fd, &byte_value, 1);
               pos += 8;
            }

            close(fd);
            return 0;
         }


      inline void insert( const string& s)
         {
         // Hash the string into two unique hashes.
         // const auto s_new = std::string(s.c_str());
         // Use double hashing to get unique bit, and repeat for each hash function.

         WithWriteLock wl(bloom_lock);
         if (s.size() == 0)
            return;
         const auto hashes = crypto.doubleHash(s);
         for ( unsigned int i = 0; i < num_hashes; ++i )
            {
            const unsigned int index = ( (hashes.first + i * hashes.second) % bits.size() );
            bits[index] = true; 
            }
         }



      inline bool contains( const string& s ) 
         {
            // Hash the string into two unqiue hashes.

            // Use double hashing to get unique bit, and repeat for each hash function.
            // If bit is false, we know for certain this unique string has not been inserted.

            // If all bits were true, the string is likely inserted, but false positive is possible.

            WithWriteLock wl(bloom_lock);
            if (s.size() == 0)
               return false;
            const auto hashes = crypto.doubleHash(s);
            for ( unsigned int i = 0; i < num_hashes; ++i ) 
               {
                  const unsigned int index = ( (hashes.first + i * hashes.second) % bits.size() );
                  if ( !bits[index] ) return false; 
               }
            return true;
         }


   private:

         static constexpr size_t NUM_OBJECTS = 10000000; // 10 million
         static constexpr double FALSE_POSITIVE_RATE = 0.005; // .5%
      

      unsigned int num_hashes;
      // unsigned int optimized_size;

      vector<bool> bits; 

      Crypto crypto;

      ReaderWriterLock bloom_lock;


   
   };

#endif