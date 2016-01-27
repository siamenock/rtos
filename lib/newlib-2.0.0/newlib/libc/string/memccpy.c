#ifdef __SSE_4_1__
#include <stdio.h>
#include <stdint.h>
#include <xmmintrin.h>
#include <smmintrin.h>

void* memccpy(void *dst, void *src, int c, size_t len) {
	uint8_t* a = dst;
	uint8_t* b = src;
	uint8_t endchar = c & 0xff;

	if(!len)
		return NULL;

	int aligned_a = 0, aligned_b = 0;
	int i = 0;

	aligned_a = ((uintptr_t)a & (sizeof(__m128i) - 1));
	aligned_b = ((uintptr_t)b & (sizeof(__m128i) - 1));

	/* Not aligned */
	if(aligned_a != aligned_b) {
		while(len) {
			if(b[i] == endchar) {
				a[i] = b[i];
				return a + i;
			}

			a[i] = b[i];

			i++;
			len--;
		}

		return NULL;
	}

	/* aligned */
	if(aligned_a) {
		while(len && ((uintptr_t) &a[i] & ( sizeof(__m128i)-1))) {
			if(b[i] == endchar) {
				a[i] = b[i];
				return a + i;
			}

			a[i] = b[i];

			i++;
			len--;
		}
	}

	if(len >= 16) {
		uint32_t buf_32 = endchar;
		buf_32 |= (buf_32 << 8);
		buf_32 |= (buf_32 << 16);

		__m128i r1 = _mm_set_epi32(buf_32, buf_32, buf_32, buf_32);

		while(len >= 16) {
			__m128i y = _mm_loadu_si128((__m128i*)&(b[i])); //16byte

			__m128i cmp = _mm_cmpeq_epi8(y, r1);

			uint16_t result = (uint16_t)_mm_movemask_epi8(cmp);

			if(result != 0x0) {
				//result = ~result;

				while(1) {
					if(result & 0x1) {
						a[i] = b[i];
						return a + i;
					}

					a[i] = b[i];
					result = result >> 1;
					i++;
				}
			}

			_mm_store_si128((__m128i*)&a[i], y);

			i += 16;
			len -= 16;
		}
	}

	while(len) {
		if(b[i] == endchar) {
			a[i] = b[i];
			return a + i;
		}

		a[i] = b[i];

		i++;
		len--;
	}

	return NULL;
}
#else
/*
FUNCTION
        <<memccpy>>---copy memory regions with end-token check

ANSI_SYNOPSIS
        #include <string.h>
        void* memccpy(void *<[out]>, const void *<[in]>, 
                      int <[endchar]>, size_t <[n]>);

TRAD_SYNOPSIS
        void *memccpy(<[out]>, <[in]>, <[endchar]>, <[n]>
        void *<[out]>;
        void *<[in]>;
	int <[endchar]>;
        size_t <[n]>;

DESCRIPTION
        This function copies up to <[n]> bytes from the memory region
        pointed to by <[in]> to the memory region pointed to by
        <[out]>.  If a byte matching the <[endchar]> is encountered,
	the byte is copied and copying stops.

        If the regions overlap, the behavior is undefined.

RETURNS
        <<memccpy>> returns a pointer to the first byte following the
	<[endchar]> in the <[out]> region.  If no byte matching
	<[endchar]> was copied, then <<NULL>> is returned.

PORTABILITY
<<memccpy>> is a GNU extension.

<<memccpy>> requires no supporting OS subroutines.

	*/

#include <_ansi.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>

/* Nonzero if either X or Y is not aligned on a "long" boundary.  */
#define UNALIGNED(X, Y) \
  (((long)X & (sizeof (long) - 1)) | ((long)Y & (sizeof (long) - 1)))

/* How many bytes are copied each iteration of the word copy loop.  */
#define LITTLEBLOCKSIZE (sizeof (long))

/* Threshhold for punting to the byte copier.  */
#define TOO_SMALL(LEN)  ((LEN) < LITTLEBLOCKSIZE)

/* Macros for detecting endchar */
#if LONG_MAX == 2147483647L
#define DETECTNULL(X) (((X) - 0x01010101) & ~(X) & 0x80808080)
#else
#if LONG_MAX == 9223372036854775807L
/* Nonzero if X (a long int) contains a NULL byte. */
#define DETECTNULL(X) (((X) - 0x0101010101010101) & ~(X) & 0x8080808080808080)
#else
#error long int is not a 32bit or 64bit type.
#endif
#endif


_PTR
_DEFUN (memccpy, (dst0, src0, endchar, len0),
	_PTR dst0 _AND
	_CONST _PTR src0 _AND
	int endchar0 _AND
	size_t len0)
{

#if defined(PREFER_SIZE_OVER_SPEED) || defined(__OPTIMIZE_SIZE__)
  _PTR ptr = NULL;
  char *dst = (char *) dst0;
  char *src = (char *) src0;
  char endchar = endchar0 & 0xff;

  while (len0--)
    {
      if ((*dst++ = *src++) == endchar)
        {
          ptr = dst;
          break;
        }
    }

  return ptr;
#else
  _PTR ptr = NULL;
  char *dst = dst0;
  _CONST char *src = src0;
  long *aligned_dst;
  _CONST long *aligned_src;
  char endchar = endchar0 & 0xff;

  /* If the size is small, or either SRC or DST is unaligned,
     then punt into the byte copy loop.  This should be rare.  */
  if (!TOO_SMALL(len0) && !UNALIGNED (src, dst))
    {
      int i;
      unsigned long mask = 0;

      aligned_dst = (long*)dst;
      aligned_src = (long*)src;

      /* The fast code reads the ASCII one word at a time and only
         performs the bytewise search on word-sized segments if they
         contain the search character, which is detected by XORing
         the word-sized segment with a word-sized block of the search
         character and then detecting for the presence of NULL in the
         result.  */
      for (i = 0; i < LITTLEBLOCKSIZE; i++)
        mask = (mask << 8) + endchar;


      /* Copy one long word at a time if possible.  */
      while (len0 >= LITTLEBLOCKSIZE)
        {
          unsigned long buffer = (unsigned long)(*aligned_src);
          buffer ^=  mask;
          if (DETECTNULL (buffer))
            break; /* endchar is found, go byte by byte from here */
          *aligned_dst++ = *aligned_src++;
          len0 -= LITTLEBLOCKSIZE;
        }

       /* Pick up any residual with a byte copier.  */
      dst = (char*)aligned_dst;
      src = (char*)aligned_src;
    }

  while (len0--)
    {
      if ((*dst++ = *src++) == endchar)
        {
          ptr = dst;
          break;
        }
    }

  return ptr;
#endif /* not PREFER_SIZE_OVER_SPEED */
}
#endif /*__SSE_4_1__*/
