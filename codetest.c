// -*- compile-command: "cc -Wall -Werror -I . -L. -ggdb -o codetest codetest.c  -Wl,-R -Wl,. -lcode64d" -*-

#include <stdio.h>
#include <string.h>   // memset(), strerror()
#include <errno.h>    // make available the global errno variable
#include <alloca.h>
#include <stdlib.h>   // for malloc()

#include "code64.h"

/**
 * The two following strings were copied from https://en.wikipedia.org/wiki/Base64.
 * I used them to test my code for correct output, and they remain in this code in
 * case I need further testing.
 */

const char buff_quote[] = "Man is distinguished, not only by his reason, but by this singular passion from other animals,"
   " which is a lust of the mind, that by a perseverance of delight in the continued and indefatigable"
   " generation of knowledge, exceeds the short vehemence of any carnal pleasure.";

const char buff_encoded[] = 
   "TWFuIGlzIGRpc3Rpbmd1aXNoZWQsIG5vdCBvbmx5IGJ5IGhpcyByZWFzb24sIGJ1dCBieSB0aGlz"
   "IHNpbmd1bGFyIHBhc3Npb24gZnJvbSBvdGhlciBhbmltYWxzLCB3aGljaCBpcyBhIGx1c3Qgb2Yg"
   "dGhlIG1pbmQsIHRoYXQgYnkgYSBwZXJzZXZlcmFuY2Ugb2YgZGVsaWdodCBpbiB0aGUgY29udGlu"
   "dWVkIGFuZCBpbmRlZmF0aWdhYmxlIGdlbmVyYXRpb24gb2Yga25vd2xlZGdlLCBleGNlZWRzIHRo"
   "ZSBzaG9ydCB2ZWhlbWVuY2Ugb2YgYW55IGNhcm5hbCBwbGVhc3VyZS4=";



/**
 * @brief Often-used callback function for display decoding results
 */
void show_decoded_string(const void* result, size_t data_length)
{
   const char *quote = (const char*)result;
   printf("The decoded result is [44;1m%.*s[m (%lu).\n\n", (int)data_length, quote, data_length);
}

/**
 * @brief Demonstrate stack-based result WITHOUT using c64_encode_to_callback().
 *
 * This function demonstrates the heap-saving technique that can be
 * used without the deprecated c64_encode_to_callback().
 */
void encode_to_callback(const char *input, size_t len_input, Encode_User callback)
{
   size_t len_encoded = c64_encode_required_buffer_length(len_input);
   char *buffer = (char*)alloca(len_encoded);

   c64_encode_to_buffer(input, len_input, (void*)buffer, len_encoded);
   (*callback)(buffer);
}

/**
 * @brief Demonstrate stack-based result WITHOUT using c64_decode_to_callback().
 *
 * This function demonstrates the heap-saving technique that can be
 * used without the deprecated c64_decode_to_callback().
 */
void decode_to_callback(const char *input, Decode_User callback)
{
   size_t len_decoded = c64_decode_chars_needed(c64_decoding_length(input));
   char *buffer = (char*)alloca(len_decoded);

   c64_decode_to_buffer(input, buffer, len_decoded);
   (*callback)(buffer, len_decoded);
}

/**
 * @brief Demonstration/example for direct use of the fundamental encoding function
 *        **c64_encode_to_pointer**.
 *
 */
void explicit_conversion_demo(const char *str)
{
   size_t len_input = strlen(str);
   size_t encoding_len = c64_encode_required_buffer_length(len_input);

   uint32_t *buffer = (uint32_t*)alloca(encoding_len);

   if (((intptr_t)buffer % 4) == 0)
      printf("The array is 32-bit integer aligned.\n");
   else
      printf("The array IS NOT 32-bit integer aligned.\n");
      
   const char *ptr_in = str;
   const char *ptr_end = str + len_input;
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

   printf("Converted [32;1m%s[m to [32;1m%s[m\n", str, (const char*)buffer);
}


void report_decode_len(int len)
{
   size_t out = c64_decode_chars_needed(len);
   printf("For length of %d, we need %lu bytes.\n", len, out);
}


void prediction_test(void)
{
   int len_quote = strlen(buff_quote);

   int len_encoded = strlen(buff_encoded);

   printf("\n[33;1mBeginning prediction_test.[m\n");
   printf("encoding prediction: len_text %d, predicted %lu, actual %d.\n",
          len_quote,
          c64_encode_required_buffer_length(len_quote),
          len_encoded);

   printf("[33;1mBEFORE[m adjustment: %d,  ", len_encoded);
   while (buff_encoded[len_encoded-1] == '=')
      --len_encoded;
   printf("[33;1mAFTER[m adjustment: %d\n", len_encoded);

   printf("decoding prediction: len_text %d, predicted %lu, actual %d.\n",
          len_encoded,
          c64_decode_chars_needed(len_encoded),
          len_quote);
}

/**
 * @brief Test a variety of known encodings to confirm my routine.
 */
void test_decoding(void)
{
   printf("Testing strings with end-padding for missing data.\n");
   decode_to_callback("YW55IGNhcm5hbCBwbGVhc3VyZS4=", show_decoded_string);
   decode_to_callback("YW55IGNhcm5hbCBwbGVhc3VyZQ==", show_decoded_string);
   decode_to_callback("YW55IGNhcm5hbCBwbGVhc3Vy", show_decoded_string);
   decode_to_callback("YW55IGNhcm5hbCBwbGVhc3U=", show_decoded_string);
   decode_to_callback("YW55IGNhcm5hbCBwbGVhcw==", show_decoded_string);

   printf("Testing strings WITHOUT end-padding for missing data.\n");
   decode_to_callback("YW55IGNhcm5hbCBwbGVhcw", show_decoded_string);
   decode_to_callback("YW55IGNhcm5hbCBwbGVhc3U", show_decoded_string);
   decode_to_callback("YW55IGNhcm5hbCBwbGVhc3Vy", show_decoded_string);

   printf("Testing decode of entire encoded leviathan quote.\n");
   decode_to_callback(buff_encoded, show_decoded_string);
}

void explicit_conversion_test(void)
{
   printf("\n[33;1mBeginning explicit_conversion_test.[m\n");
   explicit_conversion_demo("Man");
   explicit_conversion_demo("Ma");
   explicit_conversion_demo("any carnal pleas");
   explicit_conversion_demo("any carnal pleasu");
   explicit_conversion_demo("any carnal pleasur");
}

void test_allowing_invalid_encode_chars(void)
{
   const char *strings[] = {
   "YW55IGNhcm5hbCB@#w  bG&*Vhc3VyZS4=",
   "YW55IGNhc\tm5hbCBwbGVhc3VyZQ==",
   "YW55IGNhcm5hbCBwbGVhc3Vy",
   NULL
   };

   const char **ptr = strings;
   char buff[128];    // Much too long buffer, but efficiency isn't important here.
   
   printf("\n\nSome valid encoded strings may include invalid characters.\n");
   printf("This test confirms that the decoder appropriately ignored them.\n");

   while (*ptr)
   {
      // Zero memory to effectively set the \0 string terminator character:
      memset((void*)&buff, 0, sizeof(buff));

      c64_decode_to_buffer( *ptr, buff, sizeof(buff) );
      printf("[44;1m%s[m converts to [44;1m%s[m.\n", *ptr, buff);

      ++ptr;
   }
}

void test_decode_chars_needed(void)
{
   const char *strings[]  = {
      "Now is the time for all good men.",
      "Now is the time for all good men",
      "Now is the time for all good me",
      "Now is the time for all good m",
      "Now is the time for all good ",
      "Now is the time for all good",
      "Now is the time for all goo",
      "Now is the time for all go",
      "Now is the time for all g",
      "Now is the time for all ",
      "Now is the time for all",
      "Now is the time for al",
      "Now is the time for a",
      "Now is the time for ",
      "Now is the time for",
      "Now is the time fo",
      "Now is the time f",
      NULL
   };

   const char **ptr = strings;
   uint32_t encode_buffer[45];
   char decode_buffer[60];
   int len_input, len_encoded, calc_encoded, calc_decoded, len_decoded;

   while (*ptr)
   {
      memset((void*)encode_buffer, 0, sizeof(encode_buffer));
      memset((void*)decode_buffer, 0, sizeof(decode_buffer));

      len_input = c64_decoding_length(*ptr);
      calc_encoded = c64_encode_required_buffer_length(len_input);
      c64_encode_to_buffer(*ptr, len_input, encode_buffer, sizeof(encode_buffer));

      len_encoded = strlen((const char*)encode_buffer);

      calc_decoded = c64_decode_chars_needed(len_encoded);

      c64_decode_to_buffer((const char*)encode_buffer, decode_buffer, sizeof(decode_buffer));

      len_decoded = strlen(decode_buffer);

      if (len_decoded != len_input)
         printf("[41m");

      printf("%02d : %02d : %02d : %02d : %02d [4m%s[24m -> [4m%s[24m -> [4m%s[m\n",
             len_input,
             calc_encoded,
             len_encoded,
             calc_decoded,
             len_decoded,
             *ptr,
             (const char*)encode_buffer,
             decode_buffer);

      if (len_decoded != len_input)
         printf("len input : %d, len of encoded output : %d, len of decoded encoding : %d\n\n",
                len_input,
                len_encoded,
                len_decoded);

      ++ptr;
   }

}


/**
 * @brief Run test using quote found on [base 64 Wiki Page](https://en.wikipedia.org/wiki/Base64)
 *
 * Compare output for quote on Wiki page.
 */
void leviathan_quote_test(const char *result)
{
   printf("\n[33;1mBeginning leviathan_quote_test.[m\n");

   printf("[32;1m%s[m\n", result);

   if ( strcmp(result, buff_encoded) )
      printf("The results DO NOT match.\n");
   else
      printf("The results match.\n");
}

void encode_to_buffer_test(const char *str)
{
   size_t str_len = strlen(str);
   size_t encoded_len = c64_encode_required_buffer_length(str_len);
   uint32_t *buffer = (uint32_t*)malloc(encoded_len);

   printf("\n[33;1mBeginning encode_to_buffer_test with "
          "%lu-length buffer for a %lu-length string..[m\n",
          encoded_len, str_len);

   c64_encode_to_buffer(str, str_len, buffer, encoded_len);

   printf ("[32;1m%s[m\n", (const char*)buffer);

   free(buffer);
}

void run_tests(void)
{
   prediction_test();
   explicit_conversion_test();
   encode_to_callback(buff_quote, strlen(buff_quote), leviathan_quote_test);
   encode_to_buffer_test(buff_quote);

   test_decode_chars_needed();
   test_decoding();
   decode_to_callback("YW55IGNhcm5hbCBwbGVhc3Vy", show_decoded_string);
   test_allowing_invalid_encode_chars();

}

int main(int argc, const char **argv)
{
   run_tests();
   return 0;
}
