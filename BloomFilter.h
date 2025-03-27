#ifndef BLOOMFILTER_H
#define BLOOMFILTER_H

#include "../utils/vector.h"
#include <cmath>
#include "../utils/string.h"
#include <openssl/md5.h>

class Bloomfilter
   {
   public:
      Bloomfilter( int num_objects, double false_positive_rate )
         {
         // Determine the size of bits of our data vector, and resize.

         // Use the formula: m = - (n * log(p)) / (log(2)^2)

         const unsigned int optimized_size = (int)(- (num_objects * log(false_positive_rate)) / (log(2) * log(2)));

         // Determine number of hash functions to use.
         const unsigned int num_hashes = (int)( (optimized_size / num_objects) * log(2) );
         
         this->num_hashes = num_hashes;
         bits.resize( optimized_size, false ); 


         }

      void insert( const string &s)
         {
         // Hash the string into two unique hashes.
         const auto hashes = hash(s);
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

         const auto hashes = hash(s);
         for ( unsigned int i = 0; i < num_hashes; ++i )
            {
            const unsigned int index = ( (hashes.first + i * hashes.second) % bits.size() );
            if ( !bits[index] ) return false; 
            }
         }

   private:
      // Add any private member variables that may be neccessary.
      unsigned int num_hashes;
      // unsigned int optimized_size;

      vector<bool> bits; 

      std::pair<uint64_t, uint64_t> hash( const string &datum )
         {
         //Use MD5 to hash the string into one 128 bit hash, and split into 2 hashes.
        
         unsigned char hash_digest[MD5_DIGEST_LENGTH];
         
         MD5((unsigned char*)datum.c_str(), datum.length(), (unsigned char*)&hash_digest);


         uint64_t hash1 = 0;
         uint64_t hash2 = 0;
         memcpy(&hash1, &hash_digest[0], sizeof(uint64_t));
         memcpy(&hash2, &hash_digest[8], sizeof(uint64_t));

         return {hash1,hash2};
         }
   };

#endif