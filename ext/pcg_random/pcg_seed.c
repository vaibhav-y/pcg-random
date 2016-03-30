#include <ruby.h>
#include <pcg_variants.h>
#include <stdbool.h>
#include <stdint.h>

#include "entropy.h"
#include "pcg_seed.h"

/*
 * Returns a 16-byte integer that stores the seed value used to seed the
 * initial state and sequence for the PCG generator.
 */
VALUE
pcg_func_new_seed(VALUE self)
{
    return DEFAULT_SEED_VALUE;
}

/*
 * Generates a random seed string represented as a sequence of bytes
 *
 * @param byte_size Size of the bytestring to generate
 */
VALUE
pcg_func_raw_seed(VALUE self, VALUE byte_size)
{
    unsigned long n = NUM2ULONG(byte_size);
    if(n == 0)
    {
        return rb_str_new2("\0");
    }
    return pcg_raw_seed_bytestr(n);
}

/*
 * Gets `size` number of random bytes from a platform source
 * and returns those as a ruby string
 */
VALUE
pcg_raw_seed_bytestr(size_t size)
{
    VALUE result;
    char *bytestr = ALLOCA_N(char, size + 1);
    if(bytestr == NULL)
    {
        rb_raise(rb_eNoMemError,
            "Could not allocate enough space for %lu-byte seed!", size);
    }
    MEMZERO(bytestr, char, size + 1);
    pcg_func_entropy_getbytes((void *)bytestr, size);

    result = rb_str_new2(bytestr);

    return result;
}

/*
 * Generates a sequence of random bytes using device entropy
 * or a fallback mechanism based on pcg32_random_r
 */
bool
pcg_func_entropy_getbytes(void *dest, size_t size)
{
    // Get random bytes from /dev/random or a fallback source
    if(!entropy_getbytes(dest, size))
    {
        fallback_entropy_getbytes(dest, size);
        return true;
    }
    return true;
}

/*
 * Random integers are sourced from /dev/random or a fallback
 * source
 * Seed generated by manipulating a 16byte string & unpacking it to Q*
 */
VALUE
pcg_new_seed_bytestr(unsigned long seed_size)
{
    uint8_t *bytes =  ALLOCA_N(uint8_t, seed_size);

    if(bytes == NULL)
    {
        rb_raise(rb_eNoMemError, "Could not allocate enough memory!");
    }

    MEMZERO(bytes, uint8_t, seed_size);

    if(!pcg_func_entropy_getbytes((void *)bytes, seed_size))
    {
        rb_raise(rb_eRuntimeError, "Unable to generate seed!");
    }

    return rb_integer_unpack((void *) bytes, seed_size, sizeof(uint8_t), 0,
        INTEGER_PACK_MSWORD_FIRST|INTEGER_PACK_NATIVE_BYTE_ORDER);
}