// -*- compile-command: "cc -Wall -Werror -ggdb -o base64 base64.c" -*-


#include <string.h>   // for strlen
#include <stdint.h>   // for uint64_t, etc.
#include <assert.h>
#include <alloca.h>

#include "code64.h"

char byte_encode(unsigned int val)
{
   assert(val < 64);
   const static char digits[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
   return digits[val];
}


size_t c64_encode_chars_needed(size_t input_size)
{
   size_t required = input_size / 3 * 4;
   if (input_size % 3)
      required += 4;

   return required;
}

size_t c64_decode_chars_needed(size_t input_size)
{
   return input_size * 3 / 4;
}

/**
 * @brief Calculate number of uint32 elements needed for encoding buffer.
 *
 * The returned value includes a final element in which the terminating \0
 * will be written to make, when cast as _const char*_, a traditional string.
 */
size_t c64_encode_uint32s_needed(size_t input_size)
{
   // Add 1 for final \0, and one extra if remainer > 0;
   return input_size/3 + ((input_size % 3)>0) + 1;
}

/**
 * @brief The function that does the actual encoding.
 *
 * @param input Pointer to chars
 * @param buff_var Pointer to an integer variable
 * @return Cast the **buff_var** parameter to char* for return.
 *
 * This function reads up to 3 characters from **input**, writing the
 * conversion to the specific **buff_var** integer.
 *
 * I am insisting on using uint32 for the output buffer to take
 * advantage, however small, of using integer-aligned variables.
 */
const char *c64_encode_to_pointer(const char *input, uint32_t *buff_var)
{
   const unsigned char *ptr = (const unsigned char*)input;
   const unsigned char *end = ptr + 3;
   unsigned char working = 0;

   int shifts[3] = { 2, 4, 6 };
   int *shift = &shifts[0];

   char *output_buff = (char*)buff_var;

   // Preload buffer with "\0\0==" to in case of short input (less than 3 characters)
   *buff_var = *(uint32_t*)"\0\0==";

   while (*ptr && ptr < end)
   {
      // Append truncated part of current byte to left-over bits from last pass:
      working |= (*ptr >> *shift);
      // Convert 6-bit value to base64 digit and save to the current position of output buffer:
      *output_buff = byte_encode(working);

      // Prepare **working** by putting unused bits from current byte
      // to left of 
      working = ( (*ptr
                   & ((1 << *shift)-1)) // shift and subtract to make mask for unused bits
                  << (6 - *shift)       // shift bits to make room for portion of next byte
         );

      ++ptr;
      ++shift;
      ++output_buff;
   }

   if (*(ptr-1))
      *output_buff = byte_encode(working);
   
   return (const char *)buff_var;
}

/**
 * @brief Convenience function, call c64_encode_to_pointer to fill an allocated buffer.
 *
 * Use this function to encode input without using callbacks.
 *
 * Use function **c64_encode_uint32s_needed()** to get the length needed
 * for the uint32 buffer.
 */
void c64_encode_to_buffer(const char *input, uint32_t *buffer, int bufflen)
{
   size_t len_input = strlen(input);

   const char *ptr_in = input;
   const char *in_end = input + len_input;

   uint32_t *ptr_out = buffer;
   uint32_t *end_out = ptr_out + bufflen;

   while(ptr_in < in_end && ptr_out < end_out )
   {
      const char *result = c64_encode_to_pointer(ptr_in, ptr_out);

      ++ptr_out;
      ptr_in += 3;

      if (*result == 0)
         break;
   }

   *ptr_out = 0;
}


/**
 * @brief Call the **user** callback function with encoded content.
 *
 * This function allocates stack memory for the encoding buffer,
 * calling the **user** callback function when the encoding is
 * complete.  When the callback function returns, the encoding
 * is discarded along with the rest of the stack memory, when
 * this function returns.
 */
void c64_encode_to_callback(const char *input, Encode_User user)
{
   size_t len_input = strlen(input);
   size_t uint32_elements_needed = len_input / 3 + 2;

   uint32_t *buffer = (uint32_t*)alloca(uint32_elements_needed * sizeof(uint32_t));

   const char *ptr_in = input;
   const char *ptr_end = input + len_input;
   uint32_t *ptr_out = buffer;

   while(ptr_in < ptr_end)
   {
      const char *result = c64_encode_to_pointer(ptr_in, ptr_out);

      ++ptr_out;
      ptr_in += 3;

      if (*result == 0)
         break;
   }

   *ptr_out = 0;

   (*user)((const char*)buffer);
};
