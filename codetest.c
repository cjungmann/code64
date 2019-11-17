// -*- compile-command: "cc -Wall -Werror -I . -L. -ggdb -o codetest codetest.c  -Wl,-R -Wl,. -lcode64" -*-

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
      int count = ptr_end - ptr_in;
      if (count > 3)
         count = 0;

      const char *result = c64_encode_to_pointer(ptr_in, count, ptr_out);

      ++ptr_out;
      ptr_in += 3;

      if (*result == 0)
         break;
   }

   *ptr_out = 0;

   printf("[32;1m%s[m\n", (const char*)buffer);
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
          c64_encode_chars_needed(len_quote),
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
   c64_decode_to_callback("YW55IGNhcm5hbCBwbGVhc3VyZS4=", show_decoded_string);
   c64_decode_to_callback("YW55IGNhcm5hbCBwbGVhc3VyZQ==", show_decoded_string);
   c64_decode_to_callback("YW55IGNhcm5hbCBwbGVhc3Vy", show_decoded_string);
   c64_decode_to_callback("YW55IGNhcm5hbCBwbGVhc3U=", show_decoded_string);
   c64_decode_to_callback("YW55IGNhcm5hbCBwbGVhcw==", show_decoded_string);

   printf("Testing strings WITHOUT end-padding for missing data.\n");
   c64_decode_to_callback("YW55IGNhcm5hbCBwbGVhcw", show_decoded_string);
   c64_decode_to_callback("YW55IGNhcm5hbCBwbGVhc3U", show_decoded_string);
   c64_decode_to_callback("YW55IGNhcm5hbCBwbGVhc3Vy", show_decoded_string);

   printf("Testing decode of entire encoded leviathan quote.\n");
   c64_decode_to_callback(buff_encoded, show_decoded_string);
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

      len_input = c64_encoding_length(*ptr);
      calc_encoded = c64_encode_chars_needed(len_input);
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
   size_t ints_len = c64_encode_uint32s_needed(str_len);
   uint32_t *buffer = (uint32_t*)malloc(ints_len * sizeof(uint32_t));

   printf("\n[33;1mBeginning encode_to_buffer_test with %lu-length buffer for a %lu-length string..[m\n", ints_len, str_len);


   c64_encode_to_buffer(str, str_len, buffer, ints_len);

   printf ("[32;1m%s[m\n", (const char*)buffer);

   free(buffer);
}

void run_tests(void)
{
   prediction_test();
   /* explicit_conversion_test(); */
   /* c64_encode_to_callback(buff_quote, strlen(buff_quote), leviathan_quote_test); */
   /* encode_to_buffer_test(buff_quote); */

   test_decode_chars_needed();
   /* test_decoding(); */
   /* c64_decode_to_callback("YW55IGNhcm5hbCBwbGVhc3Vy", show_decoded_string); */

}

void show_usage(void)
{
   printf("-b to encode file WITH breaks.\n");
   printf("-d to decode file.\n");
   printf("-e to encode file without breaks.\n");
   printf("-i filename Read filename instead of reading stdin for input.\n");
   printf("-o filename Write to filename instead to stdout.\n");
   printf("-t to run tests.\n");
}

void close_FILEs(FILE *file1, FILE *file2)
{
   if (file1 != NULL)
      fclose(file1);
   if (file2 != NULL)
      fclose(file2);
}


int main(int argc, const char **argv)
{
   enum ops { None, Encode, Decode };
   int breaks = 0;
   const char *in_filename = NULL;
   const char *out_filename = NULL;
   FILE *fin = NULL;
   FILE *fout = NULL;
   FILE *fin_using = stdin;
   FILE *fout_using = stdout;

   enum ops operation = None;

   if (argc == 1)
   {
      show_usage();
   }
   else
   {
      const char **ptr = argv;
      int count = 0;
      while (count < argc)
      {
         if (count)
         {
            if (**ptr == '-')
            {
               switch((*ptr)[1])
               {
                  case 'b':
                     breaks = 1;
                     operation = Encode;
                     break;
                  case 'd':
                     operation = Decode;
                     break;
                  case 'e':
                     breaks = 0;
                     operation = Encode;
                     break;
                  case 'i':
                     ++ptr;
                     ++count;
                     in_filename = *ptr;
                     break;
                  case 'o':
                     ++ptr;
                     ++count;
                     out_filename = *ptr;
                     break;
                  case 't':
                     run_tests();
                     return 0;
                  default:
                     show_usage();
                     return 1;
               }
            }
         }

         ++count;
         ++ptr;
      }

      if ( operation == None)
      {
         fprintf(stderr, "Don't know what to do.\n\n");
         show_usage();
         return 1;
      }
      else
      {
         if (in_filename)
         {
            fin = fopen(in_filename, "r");
            if (fin)
               fin_using = fin;
            else
            {
               fprintf(stderr, "Failed to open input file \"%s\" (%s).\n", in_filename, strerror(errno));
               close_FILEs(fin, fout);
               return 1;
            }
         }

         if (out_filename)
         {
            fout = fopen(out_filename, "w");
            if (fout)
               fout_using = fout;
            else
            {
               fprintf(stderr, "Failed to open out file \"%s\" (%s).\n", out_filename, strerror(errno));
               close_FILEs(fin, fout);
               return 1;
            }
         }
      }

      if (operation == Encode)
         c64_encode_stream_to_stream(fin_using, fout_using, breaks);
      else if (operation == Decode)
         c64_decode_stream_to_stream(fin_using, fout_using);

      close_FILEs(fin, fout);
   }

   return 0;
}
