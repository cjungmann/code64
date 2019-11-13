// -*- compile-command: "cc -Wall -Werror -I . -L. -ggdb -o codetest codetest.c  -Wl,-R -Wl,. -lcode64" -*-

#include <stdio.h>
#include <string.h>
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
 * @brief Demonstration/example for direct use of the fundamental encoding function
 *        **c64_encode_to_pointer**.
 *
 */
void explicit_conversion_demo(const char *str)
{
   size_t len_input = strlen(str);
   size_t uint32s_needed = c64_encode_uint32s_needed(len_input);

   uint32_t *buffer = (uint32_t*)alloca(uint32s_needed * sizeof(uint32_t));

   if (((intptr_t)buffer % 4) == 0)
      printf("The array is 32-bit integer aligned.\n");
   else
      printf("The array IS NOT 32-bit integer aligned.\n");
      
   const char *ptr_in = str;
   const char *ptr_end = str + len_input;
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

   printf("[32;1m%s[m\n", (const char*)buffer);
}



void prediction_test(void)
{
   int len_quote = strlen(buff_quote);
   int len_encoded = strlen(buff_encoded);

   printf("\n[33;1mBeginning prediction_test.[m\n");
   printf("len_text %d, predicted %lu, actual %d.\n",
          len_quote,
          c64_encode_chars_needed(len_quote),
          len_encoded);
}

void explicit_conversion_test()
{
   printf("\n[33;1mBeginning explicit_conversion_test.[m\n");
   explicit_conversion_demo("Man");
   explicit_conversion_demo("Ma");
   explicit_conversion_demo("any carnal pleas");
   explicit_conversion_demo("any carnal pleasu");
   explicit_conversion_demo("any carnal pleasur");
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
   size_t ints_len = c64_encode_uint32s_needed(str_len);
   uint32_t *buffer = (uint32_t*)malloc(ints_len * sizeof(uint32_t));

   printf("\n[33;1mBeginning encode_to_buffer_test with %lu-length buffer for a %lu-length string..[m\n", ints_len, str_len);


   c64_encode_to_buffer(str, buffer, ints_len);

   printf ("[32;1m%s[m\n", (const char*)buffer);

   free(buffer);
}


int main(int argc, char **argv)
{
   /* prediction_test(); */
   explicit_conversion_test();
   c64_encode_to_callback(buff_quote, leviathan_quote_test);
   encode_to_buffer_test(buff_quote);

   return 0;
}
