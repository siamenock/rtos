#ifdef __SSE_4_1__
#include <stdint.h>
#include <xmmintrin.h>
#include <smmintrin.h>

void* memset(void *dst, int value, size_t len) {
	uint8_t* a = dst;

	int aligned_a = 0;
	int i = 0;

	aligned_a = ((uintptr_t)a & (sizeof(__m128i)-1));

	/* aligned */
	if(aligned_a) {
		while(len && ((uintptr_t) &a[i] & (sizeof(__m128i)-1))) {
			a[i] = (char)value;

			i++;
			len--;
		}
	}

	if(len >= 4) {
		uint32_t buf_32 = value;
		buf_32 |= (buf_32 << 8);
		buf_32 |= (buf_32 << 16);

		if(len >= 16) {
			__m128i r1 = _mm_set_epi32(buf_32, buf_32, buf_32, buf_32);

			while(len >= 128) {
				_mm_store_si128((__m128i*)&a[i], r1);
				_mm_store_si128((__m128i*)&a[i + 16], r1);
				_mm_store_si128((__m128i*)&a[i + 32], r1);
				_mm_store_si128((__m128i*)&a[i + 48], r1);
				_mm_store_si128((__m128i*)&a[i + 64], r1);
				_mm_store_si128((__m128i*)&a[i + 80], r1);
				_mm_store_si128((__m128i*)&a[i + 96], r1);
				_mm_store_si128((__m128i*)&a[i + 112], r1);

				i += 128;
				len -= 128;
			}

			if(len >= 64) {
				_mm_store_si128((__m128i*)&a[i], r1);
				_mm_store_si128((__m128i*)&a[i + 16], r1);
				_mm_store_si128((__m128i*)&a[i + 32], r1);
				_mm_store_si128((__m128i*)&a[i + 48], r1);

				i += 64;
				len -= 64;
			}

			if(len >= 32) {
				_mm_store_si128((__m128i*)&a[i], r1);
				_mm_store_si128((__m128i*)&a[i + 16], r1);

				i += 32;
				len -= 32;
			}

			if(len >= 16) {
				_mm_store_si128((__m128i*)&a[i], r1);

				i += 16;
				len -= 16;
			}

			if(len >= 8) {
				*(uint64_t*)(&a[i]) = buf_32;

				i += 8;
				len -= 8;
			}
		}

		while(len >= 4) {
			*(uint32_t*)(&a[i]) = buf_32;

			i += 4;
			len -= 4;
		}
	}

	while(len) {
		a[i] = (char)value;

		i++;
		len--;
	}

	return dst;
}
#else
/*
FUNCTION
	<<memset>>---set an area of memory

INDEX
	memset

ANSI_SYNOPSIS
	#include <string.h>
	void *memset(void *<[dst]>, int <[c]>, size_t <[length]>);

TRAD_SYNOPSIS
	#include <string.h>
	void *memset(<[dst]>, <[c]>, <[length]>)
	void *<[dst]>;
	int <[c]>;
	size_t <[length]>;

DESCRIPTION
	This function converts the argument <[c]> into an unsigned
	char and fills the first <[length]> characters of the array
	pointed to by <[dst]> to the value.

RETURNS
	<<memset>> returns the value of <[dst]>.

PORTABILITY
<<memset>> is ANSI C.

    <<memset>> requires no supporting OS subroutines.

QUICKREF
	memset ansi pure
*/

#include <string.h>

#define LBLOCKSIZE (sizeof(long))
#define UNALIGNED(X)   ((long)X & (LBLOCKSIZE - 1))
#define TOO_SMALL(LEN) ((LEN) < LBLOCKSIZE)

_PTR
_DEFUN (memset, (m, c, n),
	_PTR m _AND
	int c _AND
	size_t n)
{
  char *s = (char *) m;

#if !defined(PREFER_SIZE_OVER_SPEED) && !defined(__OPTIMIZE_SIZE__)
  int i;
  unsigned long buffer;
  unsigned long *aligned_addr;
  unsigned int d = c & 0xff;	/* To avoid sign extension, copy C to an
				   unsigned variable.  */

  while (UNALIGNED (s))
    {
      if (n--)
        *s++ = (char) c;
      else
        return m;
    }

  if (!TOO_SMALL (n))
    {
      /* If we get this far, we know that n is large and s is word-aligned. */
      aligned_addr = (unsigned long *) s;

      /* Store D into each char sized location in BUFFER so that
         we can set large blocks quickly.  */
      buffer = (d << 8) | d;
      buffer |= (buffer << 16);
      for (i = 32; i < LBLOCKSIZE * 8; i <<= 1)
        buffer = (buffer << i) | buffer;

      /* Unroll the loop.  */
      while (n >= LBLOCKSIZE*4)
        {
          *aligned_addr++ = buffer;
          *aligned_addr++ = buffer;
          *aligned_addr++ = buffer;
          *aligned_addr++ = buffer;
          n -= 4*LBLOCKSIZE;
        }

      while (n >= LBLOCKSIZE)
        {
          *aligned_addr++ = buffer;
          n -= LBLOCKSIZE;
        }
      /* Pick up the remainder with a bytewise loop.  */
      s = (char*)aligned_addr;
    }

#endif /* not PREFER_SIZE_OVER_SPEED */

  while (n--)
    *s++ = (char) c;

  return m;
}
#endif /*__SSE_4_1__*/
