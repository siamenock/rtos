#ifdef __SSE_4_1__
#include <stdint.h>
#include <xmmintrin.h>
#include <smmintrin.h>

void* memmove(void *dst, void *src, size_t len) {
	uint8_t* a = (uint8_t*)dst;
	uint8_t* b = (uint8_t*)src;

	int aligned_a = 0, aligned_b = 0;
	int i = 0;

	if(src < dst && dst < src + len) {
		/* Destructive overlap...have to copy backwards */
		i = len;
		aligned_a = ((uintptr_t)&a[i] & (sizeof(__m128i)-1));
		aligned_b = ((uintptr_t)&b[i] & (sizeof(__m128i)-1));

		/* Not aligned */
		if(aligned_a != aligned_b) {
			while(i) {
				i--;
				a[i] = b[i];
			}

			return dst;
		}

		/* align */
		if(aligned_a) {
			while(i && ((uintptr_t) &a[i] & (sizeof(__m128i)-1))) {
				i--;
				a[i] = b[i];
			}
		}

		  while(i >= 128) {
			i -= 128;
			__m128i r1 = _mm_loadu_si128((__m128i*)&(b[i])); //16byte
			__m128i r2 = _mm_loadu_si128((__m128i*)&(b[i + 16])); //16byte
			__m128i r3 = _mm_loadu_si128((__m128i*)&(b[i + 32])); //16byte
			__m128i r4 = _mm_loadu_si128((__m128i*)&(b[i + 48])); //16byte
			__m128i r5 = _mm_loadu_si128((__m128i*)&(b[i + 64])); //16byte
			__m128i r6 = _mm_loadu_si128((__m128i*)&(b[i + 80])); //16byte
			__m128i r7 = _mm_loadu_si128((__m128i*)&(b[i + 96])); //16byte
			__m128i r8 = _mm_loadu_si128((__m128i*)&(b[i + 112])); //16byte
			_mm_store_si128((__m128i*)&a[i], r1);
			_mm_store_si128((__m128i*)&a[i + 16], r2);
			_mm_store_si128((__m128i*)&a[i + 32], r3);
			_mm_store_si128((__m128i*)&a[i + 48], r4);
			_mm_store_si128((__m128i*)&a[i + 64], r5);
			_mm_store_si128((__m128i*)&a[i + 80], r6);
			_mm_store_si128((__m128i*)&a[i + 96], r7);
			_mm_store_si128((__m128i*)&a[i + 112], r8);
		}

		if(i >= 64) {
			i -= 64;
			__m128i r1 = _mm_loadu_si128((__m128i*)&(b[i])); //16byte
			__m128i r2 = _mm_loadu_si128((__m128i*)&(b[i + 16])); //16byte
			__m128i r3 = _mm_loadu_si128((__m128i*)&(b[i + 32])); //16byte
			__m128i r4 = _mm_loadu_si128((__m128i*)&(b[i + 48])); //16byte
			_mm_store_si128((__m128i*)&a[i], r1);
			_mm_store_si128((__m128i*)&a[i + 16], r2);
			_mm_store_si128((__m128i*)&a[i + 32], r3);
			_mm_store_si128((__m128i*)&a[i + 48], r4);

		}

		if(i >= 32) {
			i -= 32;
			__m128i r1 = _mm_loadu_si128((__m128i*)&(b[i])); //16byte
			__m128i r2 = _mm_loadu_si128((__m128i*)&(b[i + 16])); //16byte
			_mm_store_si128((__m128i*)&a[i], r1);
			_mm_store_si128((__m128i*)&a[i + 16], r2);

		}

		if(i >= 16) {
			i -= 16;
			__m128i r1 = _mm_loadu_si128((__m128i*)&(b[i])); //16byte
			_mm_store_si128((__m128i*)&a[i], r1);
		}

		while(i) {
			--i;
			a[i] = b[i];
		}
	} else {
		aligned_a = ((uintptr_t)a & (sizeof(__m128i)-1) );
		aligned_b = ((uintptr_t)b & (sizeof(__m128i)-1) );

		/* Not aligned */
		if(aligned_a != aligned_b) {
			while(len) {
				a[i] = b[i];

				i++;
				len--;
			}

			return dst;
		}

		/* aligned */
		if(aligned_a) {
			while(len && ((uintptr_t) &a[i] & ( sizeof(__m128i)-1))) {
				a[i] = b[i];

				i++;
				len--;
			}
		}

		  while(len >= 128) {
			__m128i r1 = _mm_loadu_si128((__m128i*)&(b[i])); //16byte
			__m128i r2 = _mm_loadu_si128((__m128i*)&(b[i + 16])); //16byte
			__m128i r3 = _mm_loadu_si128((__m128i*)&(b[i + 32])); //16byte
			__m128i r4 = _mm_loadu_si128((__m128i*)&(b[i + 48])); //16byte
			__m128i r5 = _mm_loadu_si128((__m128i*)&(b[i + 64])); //16byte
			__m128i r6 = _mm_loadu_si128((__m128i*)&(b[i + 80])); //16byte
			__m128i r7 = _mm_loadu_si128((__m128i*)&(b[i + 96])); //16byte
			__m128i r8 = _mm_loadu_si128((__m128i*)&(b[i + 112])); //16byte
			_mm_store_si128((__m128i*)&a[i], r1);
			_mm_store_si128((__m128i*)&a[i + 16], r2);
			_mm_store_si128((__m128i*)&a[i + 32], r3);
			_mm_store_si128((__m128i*)&a[i + 48], r4);
			_mm_store_si128((__m128i*)&a[i + 64], r5);
			_mm_store_si128((__m128i*)&a[i + 80], r6);
			_mm_store_si128((__m128i*)&a[i + 96], r7);
			_mm_store_si128((__m128i*)&a[i + 112], r8);

			i += 128;
			len -= 128;
		}

		if(len >= 64) {
			__m128i r1 = _mm_loadu_si128((__m128i*)&(b[i])); //16byte
			__m128i r2 = _mm_loadu_si128((__m128i*)&(b[i + 16])); //16byte
			__m128i r3 = _mm_loadu_si128((__m128i*)&(b[i + 32])); //16byte
			__m128i r4 = _mm_loadu_si128((__m128i*)&(b[i + 48])); //16byte
			_mm_store_si128((__m128i*)&a[i], r1);
			_mm_store_si128((__m128i*)&a[i + 16], r2);
			_mm_store_si128((__m128i*)&a[i + 32], r3);
			_mm_store_si128((__m128i*)&a[i + 48], r4);

			i += 64;
			len -= 64;
		}

		if(len >= 32) {
			__m128i r1 = _mm_loadu_si128((__m128i*)&(b[i])); //16byte
			__m128i r2 = _mm_loadu_si128((__m128i*)&(b[i + 16])); //16byte
			_mm_store_si128((__m128i*)&a[i], r1);
			_mm_store_si128((__m128i*)&a[i + 16], r2);

			i += 32;
			len -= 32;
		}

		if(len >= 16) {
			__m128i r1 = _mm_loadu_si128((__m128i*)&(b[i])); //16byte
			_mm_store_si128((__m128i*)&a[i], r1);

			i += 16;
			len -= 16;
		}

		while(len >= 4) {
			*(uint32_t*)(&a[i]) = *(uint32_t*)(&b[i]);

			i += 4;
			len -= 4;
		}

		while(len) {
			a[i] = b[i];

			i++;
			len--;
		}
	}

	return dst;
}
#else
/*
FUNCTION
	<<memmove>>---move possibly overlapping memory

INDEX
	memmove

ANSI_SYNOPSIS
	#include <string.h>
	void *memmove(void *<[dst]>, const void *<[src]>, size_t <[length]>);

TRAD_SYNOPSIS
	#include <string.h>
	void *memmove(<[dst]>, <[src]>, <[length]>)
	void *<[dst]>;
	void *<[src]>;
	size_t <[length]>;

DESCRIPTION
	This function moves <[length]> characters from the block of
	memory starting at <<*<[src]>>> to the memory starting at
	<<*<[dst]>>>. <<memmove>> reproduces the characters correctly
	at <<*<[dst]>>> even if the two areas overlap.


RETURNS
	The function returns <[dst]> as passed.

PORTABILITY
<<memmove>> is ANSI C.

<<memmove>> requires no supporting OS subroutines.

QUICKREF
	memmove ansi pure
*/

#include <string.h>
#include <_ansi.h>
#include <stddef.h>
#include <limits.h>

/* Nonzero if either X or Y is not aligned on a "long" boundary.  */
#define UNALIGNED(X, Y) \
  (((long)X & (sizeof (long) - 1)) | ((long)Y & (sizeof (long) - 1)))

/* How many bytes are copied each iteration of the 4X unrolled loop.  */
#define BIGBLOCKSIZE    (sizeof (long) << 2)

/* How many bytes are copied each iteration of the word copy loop.  */
#define LITTLEBLOCKSIZE (sizeof (long))

/* Threshhold for punting to the byte copier.  */
#define TOO_SMALL(LEN)  ((LEN) < BIGBLOCKSIZE)

/*SUPPRESS 20*/
_PTR
_DEFUN (memmove, (dst_void, src_void, length),
	_PTR dst_void _AND
	_CONST _PTR src_void _AND
	size_t length)
{
#if defined(PREFER_SIZE_OVER_SPEED) || defined(__OPTIMIZE_SIZE__)
  char *dst = dst_void;
  _CONST char *src = src_void;

  if (src < dst && dst < src + length)
    {
      /* Have to copy backwards */
      src += length;
      dst += length;
      while (length--)
	{
	  *--dst = *--src;
	}
    }
  else
    {
      while (length--)
	{
	  *dst++ = *src++;
	}
    }

  return dst_void;
#else
  char *dst = dst_void;
  _CONST char *src = src_void;
  long *aligned_dst;
  _CONST long *aligned_src;

  if (src < dst && dst < src + length)
    {
      /* Destructive overlap...have to copy backwards */
      src += length;
      dst += length;
      while (length--)
	{
	  *--dst = *--src;
	}
    }
  else
    {
      /* Use optimizing algorithm for a non-destructive copy to closely 
         match memcpy. If the size is small or either SRC or DST is unaligned,
         then punt into the byte copy loop.  This should be rare.  */
      if (!TOO_SMALL(length) && !UNALIGNED (src, dst))
        {
          aligned_dst = (long*)dst;
          aligned_src = (long*)src;

          /* Copy 4X long words at a time if possible.  */
          while (length >= BIGBLOCKSIZE)
            {
              *aligned_dst++ = *aligned_src++;
              *aligned_dst++ = *aligned_src++;
              *aligned_dst++ = *aligned_src++;
              *aligned_dst++ = *aligned_src++;
              length -= BIGBLOCKSIZE;
            }

          /* Copy one long word at a time if possible.  */
          while (length >= LITTLEBLOCKSIZE)
            {
              *aligned_dst++ = *aligned_src++;
              length -= LITTLEBLOCKSIZE;
            }

          /* Pick up any residual with a byte copier.  */
          dst = (char*)aligned_dst;
          src = (char*)aligned_src;
        }

      while (length--)
        {
          *dst++ = *src++;
        }
    }

  return dst_void;
#endif /* not PREFER_SIZE_OVER_SPEED */
}
#endif /* __SSE_4_1__ */
