#include <stdint.h>   // for uint34_t definition

typedef void (*Encode_User)(const char *encoded_content);

size_t c64_encode_chars_needed(size_t input_size);
size_t c64_decode_chars_needed(size_t input_size);

/** decoding output buffer size needed, in uint32. */
size_t c64_encode_uint32s_needed(size_t input_size);

const char *c64_encode_to_pointer(const char *input, uint32_t *buff_var);

void c64_encode_to_buffer(const char *input, uint32_t *buffer, int bufflen);
void c64_encode_to_callback(const char *input, Encode_User user);

