#include <stdio.h>    // for fprintf to report errors
#include <string.h>   // for strlen
#include <stdint.h>   // for uint64_t, etc.
#include <assert.h>
#include <alloca.h>

#include <ctype.h>    // for isspace

#include "code64.h"

char digits[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char digit62 = '+';
char digit63 = '/';
char padding_char = '=';

char byte_encode(unsigned int val)
{
   assert(val < 64);
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
   else if (digit == digit62)
      return 62;
   else if (digit == digit63)
      return 63;
   else if (digit == padding_char)
      return '\0';
   else
   {
      fprintf(stderr, "Unrecognized digit '%c'.\n", digit);
      return -1;
   }
}

/**
 * @brief Function to remap special encoding characters.
 *
 * The first 62 encoding (indexed 0-61) characters are obvious and easy
 * to assign.  The first 52 characters are two runs through the roman
 * alphabet, first in upper case, then in lower case.  The 52 are followed
 * by 10 decimal digits, 0 through 9, making 62 total obvious digits.
 *
 * The standard special characters are '+' and '/' for the two final
 * digits that brings the count to 64, the full capacity for a 6-bit
 * value.  When encodings include padding characters to indicate an
 * sparse final 3-byte set, the padding character is usually '='.
 * Some encoding standards change the padding character in addition
 * to the base64 digits.
 *
 * Some applications use different special characters.  A URL string
 * cannot include a slash and interprets a '+' as a space, so **base64url**
 * encoding replaces '+' with '-' and '/' with '_'.
 *
 * The padding character is usually '=' and required, but some encodings
 * change this.  A '=' in a URL may confuse some URL parsers, so the '='
 * is optional in **base64url**, as well as some others.  In that case,
 * set the **charpad** parameter to '\0'.
 */
void c64_set_special_chars(char char62, char char63, char charpad)
{
   assert(char62 && char63);

   digits[62] = digit62 = char62;
   digits[63] = digit63 = char63;
   padding_char = charpad;
}

/**
 * @brief Return length to last character of an encoded string.
 *
 * The returned value is a worst-case scenario, the actual encoded
 * characters may well be less than what this function returns.
 *
 * While whitespace in input strings are ignored and not decoded,
 * the whitespace characters are included in the count of characters
 * to the end of the input string to ensure all significant
 * characters are decoded.
 */
size_t c64_decoding_length(const char *input)
{
   size_t len = strlen(input);

   /* // Alternate implementation, trimming any non-base64 */
   /* // digit from the right of the string. */
   /* while ( !strchr(digits, input[len-1]) ) */
   /*    --len; */

   // Standard implementation, only trim padding character:
   while ( input[len-1] == padding_char )
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
const char *c64_encode_to_pointer(const char *input, int count, uint32_t *buff_var)
{
   const unsigned char *ptr = (const unsigned char*)input;
   const unsigned char *end = ptr + count;
   unsigned char working = 0;

   int shifts[3] = { 2, 4, 6 };
   int *shift = &shifts[0];

   char *output_buff = (char*)buff_var;

   /* // Preload buffer with "\0\0==" to in case of short input (less than 3 characters) */
   /* *buff_var = *(uint32_t*)"\0\0=="; */

   while (ptr < end)
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

   // If any characters where processed, the final encoded
   // character must be set after the loop exits.
   if (ptr > (const unsigned char *)input)
      *output_buff = byte_encode(working);

   while (count < 3)
   {
      *++output_buff = padding_char;
      ++count;
   }
   
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
      int count = ptr_end - ptr_in;
      if (count > 3)
         count = 3;

      const char *result = c64_encode_to_pointer(ptr_in, count, ptr_out);

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
      int count = in_end - ptr_in;
      if (count > 3)
         count = 3;
      const char *result = c64_encode_to_pointer(ptr_in, count, ptr_out);

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
      c64_encode_to_pointer((const char*)&reading, bytes_read, &working);

      bytes_written = fwrite((void*)&working, 1, 4, out);

      total_written += bytes_written;
      if (breaks && total_written && (total_written % 76)==0 )
         fwrite((void*)"\r\n", 1, 2, out);

      reading = 0;
   }
}

void c64_decode_to_callback(const char *input, Decode_User user)
{
   size_t in_len = c64_decoding_length(input);

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
   size_t in_len = c64_decoding_length(input);
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
      bytes_written = fwrite((void*)&working, 1, 3, out);
      total_written += bytes_written;

      reading = 0;
   }
}


