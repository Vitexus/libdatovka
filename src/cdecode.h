/*
cdecode.h - c header for a Base64 decoding algorithm

This is part of the libb64 project, and has been placed in the public domain.
For details, see http://sourceforge.net/projects/libb64
*/

#ifndef BASE64_CDECODE_H
#define BASE64_CDECODE_H
#include <stdint.h>
#include <stddef.h>

typedef enum {
	step_a, step_b, step_c, step_d
} base64_decodestep;

typedef struct {
	base64_decodestep step;
	int8_t plainchar;
} base64_decodestate;

void _isds_base64_init_decodestate(base64_decodestate *state_in);

size_t _isds_base64_decode_block(const int8_t *code_in,
        const size_t length_in, int8_t *plaintext_out,
        base64_decodestate *state_in);

#endif /* BASE64_CDECODE_H */
