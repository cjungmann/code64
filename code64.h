#ifndef CODE64_H
#define CODE64_H

#include <stdint.h>    // for uint34_t definition
#include <stdio.h>     // for FILE*
#include <sys/types.h> // for size_t

typedef void (*Encode_User)(const char *encoded_content);
typedef void (*Decode_User)(const void *decoded_content, size_t data_length);

/** Replace special encoding characters '+', '/', and '=' with alternates. */
void c64_set_special_chars(const char *special_chars);

/** Returns length of right-trimmed input string. */
size_t c64_decoding_length(const char *input);

/** Functions to predict memory requirements of encoding and decoding. */
size_t c64_encode_chars_needed(size_t input_size);
size_t c64_decode_chars_needed(size_t input_size);

size_t c64_encode_required_buffer_length(size_t input_size) { return c64_encode_chars_needed(input_size); }

/** Functions to perform conversions of the smallest portion the input. */
const char *c64_encode_to_pointer(const char *input, int count, uint32_t *buff_var);
int c64_decode_to_pointer(const char *input, uint32_t *buff_var);

/** Encoding functions that convert the entire input **/
void c64_encode_to_buffer(const char *input, size_t len_input, uint32_t *buffer, int bufflen);
void c64_encode_stream_to_stream(FILE *in, FILE *out, unsigned int breaks);

/** Decoding functions that convert the entire input **/
void c64_decode_to_buffer(const char *input, char *buffer, size_t len);
void c64_decode_stream_to_stream(FILE *in, FILE *out);


#endif
