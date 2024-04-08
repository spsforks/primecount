# We use GCC/Clang's function multi-versioning for AVX512
# support. This code will automatically dispatch to the
# AVX512 algorithm if the CPU supports AVX512 and use the
# default (portable) algorithm otherwise.

include(CheckCXXSourceCompiles)

check_cxx_source_compiles("
    #include <immintrin.h>
    #include <stdint.h>

    // GCC/Clang function multiversioning for AVX512 is not needed if
    // the user compiles with -mavx512f -mavx512vbmi -mavx512vbmi2.
    // GCC/Clang function multiversioning generally causes a minor
    // overhead, hence we disable it if it is not needed.
    #if defined(__AVX512F__) && \
        defined(__AVX512VBMI__) && \
        defined(__AVX512VBMI2__)
      Error: AVX512VBMI2 multiarch not needed!
    #endif

    class PrimeGenerator {
        public:
        __attribute__ ((target (\"default\")))
        void fillNextPrimes(uint64_t* primes64);
        __attribute__ ((target (\"avx512f,avx512vbmi,avx512vbmi2\")))
        void fillNextPrimes(uint64_t* primes64);
    };

    __attribute__ ((target (\"default\")))
    void PrimeGenerator::fillNextPrimes(uint64_t* primes64)
    {
        primes64[0] = 2;
    }

    __attribute__ ((target (\"avx512f,avx512vbmi,avx512vbmi2\")))
    void PrimeGenerator::fillNextPrimes(uint64_t* primes64)
    {
        __m512i bytes_0_to_7 = _mm512_setr_epi64(0, 1, 2, 3, 4, 5, 6, 7);
        __m512i base = _mm512_set1_epi64(123);
        __m512i bitValues = _mm512_maskz_compress_epi8(0xffff, base);
        __m512i vprimes = _mm512_maskz_permutexvar_epi8(0x0101010101010101ull, bytes_0_to_7, bitValues);
        vprimes = _mm512_add_epi64(base, vprimes);
        _mm512_storeu_si512(primes64, vprimes);
    }

    int main()
    {
        uint64_t primes[8];
        PrimeGenerator p;
        p.fillNextPrimes(primes);
        return 0;
    }
" multiarch_avx512_vbmi2)

if(multiarch_avx512_vbmi2)
    set(ENABLE_MULTIARCH_AVX512 "ENABLE_MULTIARCH_AVX512")
endif()
