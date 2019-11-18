// -*- compile-command: "cc -Wall -Werror -O3 -I . -L. -o code64 code64.c  -Wl,-R -Wl,. -lcode64" -*-

#include <stdio.h>
#include <stdlib.h>   // for atoi();
#include <string.h>   // memset(), strerror()
#include <errno.h>    // make available the global errno variable

#include "libcode64.h"

void show_usage(void)
{
   printf("-b to encode file WITH breaks.\n");
   printf("-c set custom special characters.\n");
   printf("-d to decode file.\n");
   printf("-e to encode file without breaks.\n");
   printf("-h *help* to show usage (this display).\n");
   printf("-i filename Read filename instead of reading stdin for input.\n");
   printf("-o filename Write to filename instead to stdout.\n");
}

void close_FILEs(FILE *file1, FILE *file2)
{
   if (file1 != NULL)
      fclose(file1);
   if (file2 != NULL)
      fclose(file2);
}

/**
 * @brief Return a valid breaks value (divisble by 4).  Returns 0 if less than 3.
 */
int align_breaks_value(const char *val)
{
   int value = atoi(val);
   if (value > 3)
      return value / 4 * 4;
   else
      return 0;
}

int main(int argc, const char **argv)
{
   enum ops { None, Encode, Decode };

   // FILE stream pointers to be used for input and output.
   // Although they may point to different streams, they will
   // NOT be closed using these pointers.
   FILE *fin_using = stdin;
   FILE *fout_using = stdout;

   // These variables will point to command line arguments if
   // the -i and/or -o options are used.  Non-null values below
   // will trigger a call to fopen() when the option processing
   // is done.
   const char *in_filename = NULL;
   const char *out_filename = NULL;

   // These variable will be NULL unless -i or -o has set
   // the in_filename or out_filename values, in which case
   // these variables will hold the pointers to the opened
   // streams.  The fin_using and fout_using NULL values will
   // be replaced with these values when and it these pointers
   // are set.  These stream pointers WILL be closed at the
   // end if they are not NULL.
   FILE *fin = NULL;
   FILE *fout = NULL;

   enum ops operation = Encode;
   int breaks = 76;

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
            // If not an option, assume the argument is the infile name:
            if (**ptr != '-')
               in_filename = *ptr;
            else
            {
               switch((*ptr)[1])
               {
                  case 'b':
                     ++ptr;
                     ++count;
                     breaks = align_breaks_value(*ptr);
                     if (breaks)
                        fprintf(stderr, "Breaking lines at the %d character.\n", breaks);
                     else
                        fprintf(stderr, "Omitting line breaks in output.\n");
                     break;
                  case 'c':
                     ++ptr;
                     ++count;
                     if (strlen(*ptr) < 2)
                        fprintf(stderr, "Special characters string too short.\n");
                     else
                        c64_set_special_chars((*ptr)[0], (*ptr)[1], (*ptr)[2]);
                     break;
                  case 'd':
                     operation = Decode;
                     break;
                  case 'e':
                     operation = Encode;
                     break;
                  case 'h':
                     show_usage();
                     return 0;
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
                  default:
                     show_usage();
                     return 1;
               }
            }
         }

         ++count;
         ++ptr;
      }

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

      if (operation == Encode)
         c64_encode_stream_to_stream(fin_using, fout_using, breaks);
      else if (operation == Decode)
         c64_decode_stream_to_stream(fin_using, fout_using);

      close_FILEs(fin, fout);
   }

   return 0;
}
