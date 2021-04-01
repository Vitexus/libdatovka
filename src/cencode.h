/*
cencode.h - c header for a Base64 encoding algorithm

This is part of the libb64 project, and has been placed in the public domain.
For details, see http://sourceforge.net/projects/libb64
*/

#ifndef BASE64_CENCODE_H
#define BASE64_CENCODE_H
#include <stdint.h>
#include <stddef.h>

typedef enum {
	step_A, step_B, step_C
} base64_encodestep;

typedef struct {
	base64_encodestep step;
	int8_t result;
	int stepcount;
} base64_encodestate;

void _isds_base64_init_encodestate(base64_encodestate *state_in);

size_t _isds_base64_encode_block(const int8_t *plaintext_in, size_t length_in,
        int8_t *code_out, base64_encodestate *state_in);

size_t _isds_base64_encode_blockend(int8_t *code_out, base64_encodestate *state_in);

#endif /* BASE64_CENCODE_H */
