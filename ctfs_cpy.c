#include "ctfs_runtime.h"

#define FLUSH_ALIGN (uint64_t)64
#define ALIGN_MASK	(FLUSH_ALIGN - 1)


inline void avx_cpy(void *dest, const void *src, size_t size)
{
	/*
			* Copy the range in the forward direction.
			*
			* This is the most common, most optimized case, used unless
			* the overlap specifically prevents it.
			*/
	/* copy up to FLUSH_ALIGN boundary */

	
	size_t cnt = (uint64_t)dest & ALIGN_MASK;
	if (unlikely(cnt > 0))
	{
		cnt = FLUSH_ALIGN - cnt;
		if(cnt > size){
			cnt = size;
			size = 0;
		}
		else{
			size -= cnt;
		}
		/* never try to copy more the len bytes */
		// register uint32_t d;
		register uint8_t d8;
		// while(cnt > 3){
		// 	d = *(uint32_t*)(src);
		// 	_mm_stream_si32(dest, d);
		// 	src += 4;
		// 	dest += 4;
		// 	cnt -= 4;
		// }
		// if(unlikely(cnt > 0)){
		while(cnt){
			d8 = *(uint8_t*)(src);
			*(uint8_t*)dest = d8;
			cnt --;
			src ++;
			dest ++;
		}
		cache_wb_one(dest);
		// }
		if(size == 0){
			return;
		}
	}
	assert((uint64_t)dest % 64 == 0);
	register __m512i xmm0;
	while(size >= 64){
		xmm0 = _mm512_loadu_si512(src);
		_mm512_stream_si512(dest, xmm0);
		dest += 64;
		src += 64;
		size -= 64;
	}
	
	/* copy the tail (<512 bit)  */
	size &= ALIGN_MASK;
	if (unlikely(size != 0))
	{
		while(size > 0){
			*(uint8_t*)dest = *(uint8_t*)src;
			size --;
			dest ++;
			src ++;
		}
		cache_wb_one(dest - 1);
	}
}


inline void avx_cpyt(void *dest, void *src, size_t size)
{
	/*
			* Copy the range in the forward direction.
			*
			* This is the most common, most optimized case, used unless
			* the overlap specifically prevents it.
			*/
	/* copy up to FLUSH_ALIGN boundary */
	
	register __m512i xmm0;
	while(size >= 512){
		xmm0 = _mm512_loadu_si512(src);
		_mm512_storeu_si512(dest, xmm0);
		xmm0 = _mm512_loadu_si512(src + 64);
		_mm512_storeu_si512(dest + 64, xmm0);
		xmm0 = _mm512_loadu_si512(src + 128);
		_mm512_storeu_si512(dest + 128, xmm0);
		xmm0 = _mm512_loadu_si512(src + 192);
		_mm512_storeu_si512(dest + 192, xmm0);
		xmm0 = _mm512_loadu_si512(src + 256);
		_mm512_storeu_si512(dest + 256, xmm0);
		xmm0 = _mm512_loadu_si512(src + 320);
		_mm512_storeu_si512(dest + 320, xmm0);
		xmm0 = _mm512_loadu_si512(src + 384);
		_mm512_storeu_si512(dest + 384, xmm0);
		xmm0 = _mm512_loadu_si512(src + 448);
		_mm512_storeu_si512(dest + 448, xmm0);
		dest += 512;
		src += 512;
		size -= 512;
	}
	while(size >= 64){
		xmm0 = _mm512_loadu_si512(src);
		_mm512_storeu_si512(dest, xmm0);
		dest += 64;
		src += 64;
		size -= 64;
	}
	
	/* copy the tail  */
	size &= ALIGN_MASK;
	if (unlikely(size != 0))
	{
		while(size > 0){
			*(uint8_t*)dest = *(uint8_t*)src;
			size --;
			dest ++;
			src ++;
		}
	}

}
