#ifndef BLOOMFILTER_H
#define BLOOMFILTER_H

#include <cmath>
// #include <string.h>
// #include <openssl/md5.h>Go to your project folder and make sure there's a file at:


#include <fcntl.h>
#include <sys/stat.h>

#include "../utils/vector.h"
#include <utility>
#include <cassert>


#include "../utils/crypto.h"


class Bloomfilter
   {
   public:
      Bloomfilter( int num_objects, double false_positive_rate )
         {

         // Determine the size of bits of our data vector, and resize.

         // Use the formula: m = - (n * log(p)) / (log(2)^2)

         const unsigned int optimized_size = (int)(- (num_objects * log(false_positive_rate)) / (log(2) * log(2)));

         // Determine number of hash functions to use.
         const unsigned int n = (int)( (optimized_size / num_objects) * log(2) );
         
         this->num_hashes = n;
         bits.resize( optimized_size, false ); 

         }

         ~Bloomfilter() {
         }

         int buildBloomFilter( const char * path ) {

            int fd = open("./log/frontier/bloomfilter.bin", O_RDONLY );
            if (fd == -1) {
               std::cerr << "Error opening bloom filter";
               exit(1);
            }

            struct stat sb;
            if (fstat(fd, &sb) == -1) {
               perror("Error getting file size");
               close(fd);
               exit(1);
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

            int fd = open("./log/frontier/bloomfilter.bin", O_TRUNC | O_RDWR );
            if (fd == -1) {
               std::cerr << "Error opening bloom filter";
               exit(1);
            }

            struct stat sb;
            if (fstat(fd, &sb) == -1) {
               perror("Error getting file size");
               close(fd);
               exit(1);
            }
            int fsize = sb.st_size;

            int pos = 0;
            while (pos < bits.size()) {
               char *c = reinterpret_cast<char*>(bits.data() + pos);
               write(fd, c, 1);
               pos += 8;
            }

            close(fd);

         }


      void insert( const string &s)
         {
         // Hash the string into two unique hashes.
         // const auto s_new = std::string(s.c_str());
         const auto hashes = crypto.doubleHash(s);
         // Use double hashing to get unique bit, and repeat for each hash function.

         for ( unsigned int i = 0; i < num_hashes; ++i )
            {
            const unsigned int index = ( (hashes.first + i * hashes.second) % bits.size() );
            bits[index] = true; 
            }
         }



      bool contains( const string &s )
         {
            // Hash the string into two unqiue hashes.

            // Use double hashing to get unique bit, and repeat for each hash function.
            // If bit is false, we know for certain this unique string has not been inserted.

            // If all bits were true, the string is likely inserted, but false positive is possible.

            const auto hashes = crypto.doubleHash(s);
            for ( unsigned int i = 0; i < num_hashes; ++i ) 
               {
                  const unsigned int index = ( (hashes.first + i * hashes.second) % bits.size() );
                  if ( !bits[index] ) return false; 
               }
            return true;
         }


   private:
      // Add any private member variables that may be neccessary.
      unsigned int num_hashes;
      // unsigned int optimized_size;

      vector<bool> bits; 

      Crypto crypto;
   
   };

#endif