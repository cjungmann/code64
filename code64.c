#include <stdio.h>    // for fprintf to report errors
#include <string.h>   // for strlen
#include <stdint.h>   // for uint64_t, etc.
#include <assert.h>
#include <alloca.h>

#include <ctype.h>    // for isspace

#include "code64.h"

char byte_encode(unsigned int val)
{
   assert(val < 64);
   const static char digits[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
   return digits[val];
}

unsigned char digit_decode(char digit)
{
   if (digit >= 'A' && digit <= 'Z')
      return digit - 'A';
   else if (digit >= 'a' && digit <= 'z')
      return digit - 'a' + 26;
   else if (digit >= '0' && digit <= '9')
      return digit - '0' + 52;
   else if (digit == '+')
      return 62;
   else if (digit == '/')
      return 63;
   else if (digit == '=')
      return '\0';
   else
   {
      fprintf(stderr, "Unrecognized digit '%c'.\n", digit);
      return -1;
   }
}

/**
 * @brief Return length of encoding string with padding characters removed.
 *
 * The true length of a decoding string depends on the length of
 * the data portion of the encoded input string.
 * 
 */
size_t c64_encoding_length(const char *input)
{
   size_t len = strlen(input);
   while ( input[len-1] == '=' )
      --len;
   return len;
}

/**
 * @brief Returns number of bytes needed to encode **input_size** bytes.
 */
size_t c64_encode_chars_needed(size_t input_size)
{
   // Any overflow/remainder from x/3 requires another 4 chars if padding is included
   return input_size/3*4 + ( input_size % 3 ? 4 : 0 );
}

/**
 * @brief Returns the number of bytes guaranteed to be enough to contain
 *        the decoding.
 *
 * An accurate response requires that the input_size does not include
 * padding characters.  This function cannot determine that because it
 * only knows the length, not where the data is stored.  Look at function
 * **c64_decode_to_callback** for an example that shows how one can handle
 * the input length question.
 */

size_t c64_decode_chars_needed(size_t input_size)
{
   size_t extra = input_size % 4;
   return  input_size/4*3 + ( extra==0 ? 0 : ( extra==1 ? 1 : 2 ) );
}

/**
 * @brief Calculate number of uint32 elements needed for encoding buffer.
 *
 * The returned value includes a final element in which the terminating \0
 * will be written to make, when cast as _const char*_, a traditional string.
 */
size_t c64_encode_uint32s_needed(size_t input_size)
{
   // Add extra uint32 for any remainder.
   // Then, since the output is a string, and padding
   // will fill out the last data byte, let's add an
   // extra uint32 at the end that will be set to 0 to
   // terminate the string.
   return input_size/3 + ((input_size % 3)>0) + 1;
}

size_t c64_decode_uint32s_needed(size_t input_size)
{
   input_size *= 3;
   return input_size / 4 + ((input_size % 4)>0);
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
 *
 * This function consumers **input** 4 characters at a time.
 */
int c64_decode_to_pointer(const char *input, uint32_t *buff_val)
{
   // Abort without copying if first character end-of-string
   if (*input==0)
      return 0;

   uint32_t working = *buff_val = 0;
   char *buff_alias = (char*)buff_val;

   // pack the bits into 3 bytes of a uint32
   for (int i=26; i>6; i-=6)
   {
      if (*input)
      {
         working |= digit_decode(*input) << i;

         // Only increment pointer if !=\0 so after a \0,
         // subsequent loops get the same \0 value.
         ++input;
      }
   }

   for (int i=0, shift=24; i<3; ++i, shift-=8)
   {
      buff_alias[i] = 0xFF & (working >> shift);

      char cbyte = 0xFF & (working >> shift);
      if (cbyte == buff_alias[i])
         cbyte = 0;
   }

   return 1;
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
void c64_encode_to_callback(const char *input, size_t len_input, Encode_User user)
{
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

/**
 * @brief Convenience function, call c64_encode_to_pointer to fill an allocated buffer.
 *
 * Use this function to encode input without using callbacks.
 *
 * Use function **c64_encode_uint32s_needed()** to get the length needed
 * for the uint32 buffer.
 */
void c64_encode_to_buffer(const char *input, size_t len_input, uint32_t *buffer, int bufflen)
{
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

void c64_encode_stream_to_stream(FILE *in, FILE *out, int breaks)
{
   uint32_t reading = 0;
   uint32_t working;
   size_t total_read = 0, total_written = 0;

   size_t bytes_read, bytes_written;

   while ((bytes_read = fread((void*)&reading, 1, 3, in)) > 0)
   {
      total_read += bytes_read;
      c64_encode_to_pointer((const char*)&reading, &working);

      bytes_written = fwrite((void*)&working, 1, 4, out);

      total_written += bytes_written;
      if (breaks && total_written && (total_written % 76)==0 )
         fwrite((void*)"\r\n", 1, 2, out);

      reading = 0;
   }
}

void c64_decode_to_callback(const char *input, Decode_User user)
{
   size_t in_len = c64_encoding_length(input);

   const char *in_end = input + in_len;

   size_t out_len = c64_decode_chars_needed(in_len);
   char *out_buff = (char*)alloca(out_len);
   char *out_ptr = out_buff;
   uint32_t working;

   const char *ptr = input;
   while(ptr < in_end)
   {
      if (!c64_decode_to_pointer(ptr, &working))
         break;

      memcpy(out_ptr, (void*)&working, 3);

      ptr += 4;
      out_ptr += 3;
   }

   (*user)((const void*)out_buff, out_len);
}

void c64_decode_to_buffer(const char *input, char *buffer, size_t len)
{
   size_t in_len = c64_encoding_length(input);
   const char *in_end = input + in_len;

   assert(len >= c64_decode_chars_needed(in_len));

   char *out_ptr = buffer;
   uint32_t working;

   const char *ptr = input;
   while(ptr < in_end)
   {
      if (!c64_decode_to_pointer(ptr, &working))
         break;

      memcpy(out_ptr, (void*)&working, 3);

      ptr += 4;
      out_ptr += 3;
   }
}

int read_but_skip_whitespace(FILE *in, void* buffer, size_t len)
{
   size_t bread=0, tread=0;
   char *buff = (char*)buffer;
   char *cur;

   while (tread < len)
   {
      cur = &buff[tread];
      bread = fread(cur, 1, 1, in);
      if (bread)
      {
         if (!isspace(*cur))
         {
            ++cur;
            ++tread;
         }
      }
      else
         break;
   }

   return tread;
}

void c64_decode_stream_to_stream(FILE *in, FILE *out)
{
   uint32_t reading = 0;
   uint32_t working;
   size_t total_read = 0, total_written = 0;

   size_t bytes_read, bytes_written;

   while ((bytes_read = read_but_skip_whitespace(in, (void*)&reading, sizeof(reading))))
   /* while ((bytes_read = fread((void*)&reading, 1, 4, in)) > 0) */
   {
      total_read += bytes_read;
      c64_decode_to_pointer((const char*)&reading, &working);
      bytes_written = fwrite((void*)&working, 1, 4, out);
      total_written += bytes_written;

      reading = 0;
   }
}


