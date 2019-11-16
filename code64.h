#include <stdint.h>   // for uint34_t definition

typedef void (*Encode_User)(const char *encoded_content);
typedef void (*Decode_User)(const void *decoded_content, size_t data_length);

/** Returns length of input if padding characters are removed. */
size_t c64_encoding_length(const char *input);

/** Functions to predict memory requirements of encoding and decoding. */
size_t c64_encode_chars_needed(size_t input_size);
size_t c64_decode_chars_needed(size_t input_size);

/** Predict number of uint32s needed to encode the input. */
size_t c64_encode_uint32s_needed(size_t input_size);

/** Functions to perform conversions of the smallest portion the input. */
const char *c64_encode_to_pointer(const char *input, uint32_t *buff_var);
int c64_decode_to_pointer(const char *input, uint32_t *buff_var);

/** Encoding functions that convert the entire input **/
void c64_encode_to_callback(const char *input, size_t len_input, Encode_User user);
void c64_encode_to_buffer(const char *input, size_t len_input, uint32_t *buffer, int bufflen);
void c64_encode_stream_to_stream(FILE *in, FILE *out, int breaks);

/** Decoding functions that convert the entire input **/
void c64_decode_to_callback(const char *input, Decode_User user);
void c64_decode_to_buffer(const char *input, char *buffer, size_t len);
void c64_decode_stream_to_stream(FILE *in, FILE *out);
