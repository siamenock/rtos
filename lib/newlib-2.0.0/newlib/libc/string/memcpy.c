#ifdef __SSE_4_1__
#include <stdint.h>
#include <xmmintrin.h>
#include <smmintrin.h>

_PTR
_DEFUN (memcpy, (dst, src, len),
	_PTR dst _AND
	_CONST _PTR src _AND
	size_t len)
{
	uint8_t* a = (uint8_t*)dst;
	uint8_t* b = (uint8_t*)src;

	int aligned_a = 0, aligned_b = 0;
	int i = 0;

	aligned_a = ((uintptr_t)a & (sizeof(__m128i)-1));
	aligned_b = ((uintptr_t)b & (sizeof(__m128i)-1));

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
		while(len && ((uintptr_t) &a[i] & (sizeof(__m128i)-1))) {
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

	return dst;
}
#else
/*
FUNCTION
        <<memcpy>>---copy memory regions

ANSI_SYNOPSIS
        #include <string.h>
        void* memcpy(void *<[out]>, const void *<[in]>, size_t <[n]>);

TRAD_SYNOPSIS
        #include <string.h>
        void *memcpy(<[out]>, <[in]>, <[n]>
        void *<[out]>;
        void *<[in]>;
        size_t <[n]>;

DESCRIPTION
        This function copies <[n]> bytes from the memory region
        pointed to by <[in]> to the memory region pointed to by
        <[out]>.

        If the regions overlap, the behavior is undefined.

RETURNS
        <<memcpy>> returns a pointer to the first byte of the <[out]>
        region.

PORTABILITY
<<memcpy>> is ANSI C.

<<memcpy>> requires no supporting OS subroutines.

QUICKREF
        memcpy ansi pure
	*/

#include <_ansi.h>
#include <string.h>

/* Nonzero if either X or Y is not aligned on a "long" boundary.  */
#define UNALIGNED(X, Y) \
  (((long)X & (sizeof (long) - 1)) | ((long)Y & (sizeof (long) - 1)))

/* How many bytes are copied each iteration of the 4X unrolled loop.  */
#define BIGBLOCKSIZE    (sizeof (long) << 2)

/* How many bytes are copied each iteration of the word copy loop.  */
#define LITTLEBLOCKSIZE (sizeof (long))

/* Threshhold for punting to the byte copier.  */
#define TOO_SMALL(LEN)  ((LEN) < BIGBLOCKSIZE)

_PTR
_DEFUN (memcpy, (dst0, src0, len0),
	_PTR dst0 _AND
	_CONST _PTR src0 _AND
	size_t len0)
{
#if defined(PREFER_SIZE_OVER_SPEED) || defined(__OPTIMIZE_SIZE__)
  char *dst = (char *) dst0;
  char *src = (char *) src0;

  _PTR save = dst0;

  while (len0--)
    {
      *dst++ = *src++;
    }

  return save;
#else
  char *dst = dst0;
  _CONST char *src = src0;
  long *aligned_dst;
  _CONST long *aligned_src;

  /* If the size is small, or either SRC or DST is unaligned,
     then punt into the byte copy loop.  This should be rare.  */
  if (!TOO_SMALL(len0) && !UNALIGNED (src, dst))
    {
      aligned_dst = (long*)dst;
      aligned_src = (long*)src;

      /* Copy 4X long words at a time if possible.  */
      while (len0 >= BIGBLOCKSIZE)
        {
          *aligned_dst++ = *aligned_src++;
          *aligned_dst++ = *aligned_src++;
          *aligned_dst++ = *aligned_src++;
          *aligned_dst++ = *aligned_src++;
          len0 -= BIGBLOCKSIZE;
        }

      /* Copy one long word at a time if possible.  */
      while (len0 >= LITTLEBLOCKSIZE)
        {
          *aligned_dst++ = *aligned_src++;
          len0 -= LITTLEBLOCKSIZE;
        }

       /* Pick up any residual with a byte copier.  */
      dst = (char*)aligned_dst;
      src = (char*)aligned_src;
    }

  while (len0--)
    *dst++ = *src++;

  return dst0;
#endif /* not PREFER_SIZE_OVER_SPEED */
}
#endif /*__SSE_4_1__*/
