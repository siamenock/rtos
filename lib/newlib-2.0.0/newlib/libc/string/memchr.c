#ifdef __SSE_4_1__
#include <stdio.h>
#include <stdint.h>
#include <xmmintrin.h>
#include <smmintrin.h>

void* memchr(void *dst, int c, size_t len) {
	uint8_t* a = dst;

	if(!len)
		return NULL;

	int aligned_a = 0;
	int i = 0;

	aligned_a = ((uintptr_t)a & (sizeof(__m128i) - 1));
	/* aligned */
	if(aligned_a) {
		while(len && ((uintptr_t) &a[i] & (sizeof(__m128i) - 1))) {
			if(a[i] == (char)c) {
				return a + i;
			}

			i++;
			len--;
		}
	}

	if(len >= 16) {
		uint32_t buf_32 = c;
		buf_32 |= (buf_32 << 8);
		buf_32 |= (buf_32 << 16);

		__m128i r1 = _mm_set_epi32(buf_32, buf_32, buf_32, buf_32);
		
		while(len >= 16) {
			__m128i x = _mm_loadu_si128((__m128i*)&(a[i])); //16byte
			__m128i cmp = _mm_cmpeq_epi8(x, r1);

			uint16_t result = (uint16_t)_mm_movemask_epi8(cmp);

			if(result != 0x0000U) {
				while(!(result & 0x1)) {
					result = result >> 1;
					i++;
				}

				return a + i;
			}

			i += 16;
			len -= 16;
		}
	}

	while(len) {
		if(a[i] == (char)c) {
			return a + i;
		}

		i++;
		len--;
	}

	return NULL;
}
#else
/*
FUNCTION
	<<memchr>>---find character in memory

INDEX
	memchr

ANSI_SYNOPSIS
	#include <string.h>
	void *memchr(const void *<[src]>, int <[c]>, size_t <[length]>);

TRAD_SYNOPSIS
	#include <string.h>
	void *memchr(<[src]>, <[c]>, <[length]>)
	void *<[src]>;
	void *<[c]>;
	size_t <[length]>;

DESCRIPTION
	This function searches memory starting at <<*<[src]>>> for the
	character <[c]>.  The search only ends with the first
	occurrence of <[c]>, or after <[length]> characters; in
	particular, <<NUL>> does not terminate the search.

RETURNS
	If the character <[c]> is found within <[length]> characters
	of <<*<[src]>>>, a pointer to the character is returned. If
	<[c]> is not found, then <<NULL>> is returned.

PORTABILITY
<<memchr>> is ANSI C.

<<memchr>> requires no supporting OS subroutines.

QUICKREF
	memchr ansi pure
*/

#include <_ansi.h>
#include <string.h>
#include <limits.h>

/* Nonzero if either X or Y is not aligned on a "long" boundary.  */
#define UNALIGNED(X) ((long)X & (sizeof (long) - 1))

/* How many bytes are loaded each iteration of the word copy loop.  */
#define LBLOCKSIZE (sizeof (long))

/* Threshhold for punting to the bytewise iterator.  */
#define TOO_SMALL(LEN)  ((LEN) < LBLOCKSIZE)

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

#ifndef DETECTNULL
#error long int is not a 32bit or 64bit byte
#endif

/* DETECTCHAR returns nonzero if (long)X contains the byte used
   to fill (long)MASK. */
#define DETECTCHAR(X,MASK) (DETECTNULL(X ^ MASK))

_PTR
_DEFUN (memchr, (src_void, c, length),
	_CONST _PTR src_void _AND
	int c _AND
	size_t length)
{
  _CONST unsigned char *src = (_CONST unsigned char *) src_void;
  unsigned char d = c;

#if !defined(PREFER_SIZE_OVER_SPEED) && !defined(__OPTIMIZE_SIZE__)
  unsigned long *asrc;
  unsigned long  mask;
  int i;

  while (UNALIGNED (src))
    {
      if (!length--)
        return NULL;
      if (*src == d)
        return (void *) src;
      src++;
    }

  if (!TOO_SMALL (length))
    {
      /* If we get this far, we know that length is large and src is
         word-aligned. */
      /* The fast code reads the source one word at a time and only
         performs the bytewise search on word-sized segments if they
         contain the search character, which is detected by XORing
         the word-sized segment with a word-sized block of the search
         character and then detecting for the presence of NUL in the
         result.  */
      asrc = (unsigned long *) src;
      mask = d << 8 | d;
      mask = mask << 16 | mask;
      for (i = 32; i < LBLOCKSIZE * 8; i <<= 1)
        mask = (mask << i) | mask;

      while (length >= LBLOCKSIZE)
        {
          if (DETECTCHAR (*asrc, mask))
            break;
          length -= LBLOCKSIZE;
          asrc++;
        }

      /* If there are fewer than LBLOCKSIZE characters left,
         then we resort to the bytewise loop.  */

      src = (unsigned char *) asrc;
    }

#endif /* not PREFER_SIZE_OVER_SPEED */

  while (length--)
    {
      if (*src == d)
        return (void *) src;
      src++;
    }

  return NULL;
}
#endif /*__SSE_4_1__*/
