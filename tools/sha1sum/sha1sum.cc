/*
    sha1.cpp - source code of

    ============
    SHA-1 in C++
    ============

    100% Public Domain.

    Original C Code
        -- Steve Reid <steve@edmweb.com>
    Small changes to fit into bglibs
        -- Bruce Guenter <bruce@untroubled.org>
    Translation to simpler C++ Code
        -- Volker Grabsch <vog@notjusthosting.com>
*/

#include "sha1sum.h"
#include <fstream>
#include <iomanip>
#include <sstream>

sha1sum::sha1sum() { reset(); }

void sha1sum::update(const std::string& s) {
  std::istringstream is(s);
  update(is);
}

void sha1sum::update(std::istream& is) {
  std::string rest_of_buffer;
  read(is, rest_of_buffer);
  buffer += rest_of_buffer;

  while (is) {
    uint32 block[BLOCK_INTS];
    buffer_to_block(buffer, block);
    transform(block);
    read(is, buffer);
  }
}

/*
 * Add padding and return the message digest.
 */

std::string sha1sum::final() {
  /* Total number of hashed bits */
  uint64 total_bits = (transforms * BLOCK_BYTES + buffer.size()) * 8;

  /* Padding */
  buffer += 0x80;
  auto const orig_size = buffer.size();
  while (buffer.size() < BLOCK_BYTES) {
    buffer += (char)0x00;
  }

  uint32 block[BLOCK_INTS];
  buffer_to_block(buffer, block);

  if (orig_size > BLOCK_BYTES - 8) {
    transform(block);
    for (unsigned int i = 0; i < BLOCK_INTS - 2; i++) {
      block[i] = 0;
    }
  }

  /* Append total_bits, split this uint64 into two uint32 */
  block[BLOCK_INTS - 1] = total_bits;
  block[BLOCK_INTS - 2] = (total_bits >> 32);
  transform(block);

  /* Hex std::string */
  std::ostringstream result;
  for (unsigned int i = 0; i < DIGEST_INTS; i++) {
    result << std::hex << std::setfill('0') << std::setw(8);
    result << (digest[i] & 0xffffffff);
  }

  /* Reset for next run */
  reset();

  return result.str();
}

std::string sha1sum::from_file(const std::string& filename) {
  std::ifstream stream(filename.c_str(), std::ios::binary);
  sha1sum checksum;
  checksum.update(stream);
  return checksum.final();
}

std::string sha1sum::from_buf(const std::vector<unsigned char>& buf) {
  sha1sum checksum;
  checksum.update(
      std::string{reinterpret_cast<char const*>(&buf[0]), buf.size()});
  return checksum.final();
}

void sha1sum::reset() {
  /* sha1sum initialization constants */
  digest[0] = 0x67452301;
  digest[1] = 0xefcdab89;
  digest[2] = 0x98badcfe;
  digest[3] = 0x10325476;
  digest[4] = 0xc3d2e1f0;

  /* Reset counters */
  transforms = 0;
  buffer = "";
}

/*
 * Hash a single 512-bit block. This is the core of the algorithm.
 */

void sha1sum::transform(uint32 block[BLOCK_BYTES]) {
  /* Copy digest[] to working vars */
  uint32 a = digest[0];
  uint32 b = digest[1];
  uint32 c = digest[2];
  uint32 d = digest[3];
  uint32 e = digest[4];

  /* Help macros */
#define rol(value, bits) \
  (((value) << (bits)) | (((value)&0xffffffff) >> (32 - (bits))))
#define blk(i)                                                      \
  (block[i & 15] = rol(block[(i + 13) & 15] ^ block[(i + 8) & 15] ^ \
                           block[(i + 2) & 15] ^ block[i & 15],     \
                       1))

  /* (R0+R1), R2, R3, R4 are the different operations used in sha1sum */
#define R0(v, w, x, y, z, i)                                    \
  z += ((w & (x ^ y)) ^ y) + block[i] + 0x5a827999 + rol(v, 5); \
  w = rol(w, 30);
#define R1(v, w, x, y, z, i)                                  \
  z += ((w & (x ^ y)) ^ y) + blk(i) + 0x5a827999 + rol(v, 5); \
  w = rol(w, 30);
#define R2(v, w, x, y, z, i)                          \
  z += (w ^ x ^ y) + blk(i) + 0x6ed9eba1 + rol(v, 5); \
  w = rol(w, 30);
#define R3(v, w, x, y, z, i)                                        \
  z += (((w | x) & y) | (w & x)) + blk(i) + 0x8f1bbcdc + rol(v, 5); \
  w = rol(w, 30);
#define R4(v, w, x, y, z, i)                          \
  z += (w ^ x ^ y) + blk(i) + 0xca62c1d6 + rol(v, 5); \
  w = rol(w, 30);

  /* 4 rounds of 20 operations each. Loop unrolled. */
  R0(a, b, c, d, e, 0);
  R0(e, a, b, c, d, 1);
  R0(d, e, a, b, c, 2);
  R0(c, d, e, a, b, 3);
  R0(b, c, d, e, a, 4);
  R0(a, b, c, d, e, 5);
  R0(e, a, b, c, d, 6);
  R0(d, e, a, b, c, 7);
  R0(c, d, e, a, b, 8);
  R0(b, c, d, e, a, 9);
  R0(a, b, c, d, e, 10);
  R0(e, a, b, c, d, 11);
  R0(d, e, a, b, c, 12);
  R0(c, d, e, a, b, 13);
  R0(b, c, d, e, a, 14);
  R0(a, b, c, d, e, 15);
  R1(e, a, b, c, d, 16);
  R1(d, e, a, b, c, 17);
  R1(c, d, e, a, b, 18);
  R1(b, c, d, e, a, 19);
  R2(a, b, c, d, e, 20);
  R2(e, a, b, c, d, 21);
  R2(d, e, a, b, c, 22);
  R2(c, d, e, a, b, 23);
  R2(b, c, d, e, a, 24);
  R2(a, b, c, d, e, 25);
  R2(e, a, b, c, d, 26);
  R2(d, e, a, b, c, 27);
  R2(c, d, e, a, b, 28);
  R2(b, c, d, e, a, 29);
  R2(a, b, c, d, e, 30);
  R2(e, a, b, c, d, 31);
  R2(d, e, a, b, c, 32);
  R2(c, d, e, a, b, 33);
  R2(b, c, d, e, a, 34);
  R2(a, b, c, d, e, 35);
  R2(e, a, b, c, d, 36);
  R2(d, e, a, b, c, 37);
  R2(c, d, e, a, b, 38);
  R2(b, c, d, e, a, 39);
  R3(a, b, c, d, e, 40);
  R3(e, a, b, c, d, 41);
  R3(d, e, a, b, c, 42);
  R3(c, d, e, a, b, 43);
  R3(b, c, d, e, a, 44);
  R3(a, b, c, d, e, 45);
  R3(e, a, b, c, d, 46);
  R3(d, e, a, b, c, 47);
  R3(c, d, e, a, b, 48);
  R3(b, c, d, e, a, 49);
  R3(a, b, c, d, e, 50);
  R3(e, a, b, c, d, 51);
  R3(d, e, a, b, c, 52);
  R3(c, d, e, a, b, 53);
  R3(b, c, d, e, a, 54);
  R3(a, b, c, d, e, 55);
  R3(e, a, b, c, d, 56);
  R3(d, e, a, b, c, 57);
  R3(c, d, e, a, b, 58);
  R3(b, c, d, e, a, 59);
  R4(a, b, c, d, e, 60);
  R4(e, a, b, c, d, 61);
  R4(d, e, a, b, c, 62);
  R4(c, d, e, a, b, 63);
  R4(b, c, d, e, a, 64);
  R4(a, b, c, d, e, 65);
  R4(e, a, b, c, d, 66);
  R4(d, e, a, b, c, 67);
  R4(c, d, e, a, b, 68);
  R4(b, c, d, e, a, 69);
  R4(a, b, c, d, e, 70);
  R4(e, a, b, c, d, 71);
  R4(d, e, a, b, c, 72);
  R4(c, d, e, a, b, 73);
  R4(b, c, d, e, a, 74);
  R4(a, b, c, d, e, 75);
  R4(e, a, b, c, d, 76);
  R4(d, e, a, b, c, 77);
  R4(c, d, e, a, b, 78);
  R4(b, c, d, e, a, 79);

  /* Add the working vars back into digest[] */
  digest[0] += a;
  digest[1] += b;
  digest[2] += c;
  digest[3] += d;
  digest[4] += e;

  /* Count the number of transformations */
  transforms++;
}

void sha1sum::buffer_to_block(const std::string& buffer,
                              uint32 block[BLOCK_BYTES]) {
  /* Convert the std::string (byte buffer) to a uint32 array (MSB) */
  for (unsigned int i = 0; i < BLOCK_INTS; i++) {
    block[i] = (buffer[4 * i + 3] & 0xff) | (buffer[4 * i + 2] & 0xff) << 8 |
               (buffer[4 * i + 1] & 0xff) << 16 |
               (buffer[4 * i + 0] & 0xff) << 24;
  }
}

void sha1sum::read(std::istream& is, std::string& s) {
  char sbuf[BLOCK_BYTES];
  is.read(sbuf, BLOCK_BYTES);
  s.assign(sbuf, is.gcount());
}