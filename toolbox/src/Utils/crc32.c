/********************************************/
/* crc32.c 0.0.0 (1999-Oct-17-Thu)          */
/* Adam M. Costello <amc@cs.berkeley.edu>   */
/* mostly copied from the PNG specification */
/********************************************/

/* Implementation of the interface defined by crc32.h 0.0.*. */

/* This is ANSI C code. */


#include "crc32.h"


static crc32_t crc32_table[256];      /* CRCs of all 8-bit messages.    */
static int crc32_table_computed = 0;  /* Flag: Has table been computed? */


/* Make the table: */

static void make_crc32_table(void)
{
  crc32_t c;
  int i, k;

  for (i = 0;  i < 256;  ++i) {
    c = (crc32_t) i;

    for (k = 0;  k < 8;  ++k) {
      if (c & 1) c = 0xedb88320L ^ (c >> 1);
      else c >>= 1;
    }

    crc32_table[i] = c;
  }

  crc32_table_computed = 1;
}


crc32_t tb_crc32(const unsigned char *bytes, size_t n)
{
  return tb_crc32_update(0xffffffff, bytes, n) ^ 0xffffffff;
}


crc32_t tb_crc32_update(crc32_t crc, const unsigned char *bytes, size_t n)
{
  crc32_t c = crc;
  int i;

  if (!crc32_table_computed) make_crc32_table();

  for (i = 0;  i < n;  i++) {
    c = crc32_table[(c ^ bytes[i]) & 0xff] ^ (c >> 8);
  }

  return c;
}
