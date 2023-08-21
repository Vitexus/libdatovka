/*
cencoder.c - c source to a Base64 encoding algorithm implementation

This is part of the libb64 project, and has been placed in the public domain.
For details, see http://sourceforge.net/projects/libb64
*/

#include "cencode.h"
#include "compiler.h" /* _hidden */

static const int CHARS_PER_LINE = 72;


_hidden void _isds_base64_init_encodestate(base64_encodestate* state_in) {
	state_in->step = step_A;
	state_in->result = 0;
	state_in->stepcount = 0;
}


static int8_t base64_encode_value(int8_t value_in) {
    /* XXX: CHAR_BIT == 8 because of <stdint.h> */
	static const int8_t encoding[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	if (value_in > 63) return '=';
	return encoding[value_in];
}


_hidden size_t _isds_base64_encode_block(const int8_t* plaintext_in,
        size_t length_in, int8_t *code_out, base64_encodestate* state_in) {
	const int8_t *plainchar = plaintext_in;
	const int8_t* const plaintextend = plaintext_in + length_in;
	int8_t *codechar = code_out;
	int8_t result;
	int8_t fragment;

	result = state_in->result;

	switch (state_in->step) {
		while (1) {
	case step_A:
			if (plainchar == plaintextend) {
				state_in->result = result;
				state_in->step = step_A;
				return codechar - code_out;
			}
			fragment = *plainchar++;
			result = (fragment & 0x0fc) >> 2;
			*codechar++ = base64_encode_value(result);
			result = (fragment & 0x003) << 4;
	case step_B:
			if (plainchar == plaintextend) {
				state_in->result = result;
				state_in->step = step_B;
				return codechar - code_out;
			}
			fragment = *plainchar++;
			result |= (fragment & 0x0f0) >> 4;
			*codechar++ = base64_encode_value(result);
			result = (fragment & 0x00f) << 2;
	case step_C:
			if (plainchar == plaintextend) {
				state_in->result = result;
				state_in->step = step_C;
				return codechar - code_out;
			}
			fragment = *plainchar++;
			result |= (fragment & 0x0c0) >> 6;
			*codechar++ = base64_encode_value(result);
			result  = (fragment & 0x03f) >> 0;
			*codechar++ = base64_encode_value(result);

			++(state_in->stepcount);
			if (state_in->stepcount == CHARS_PER_LINE/4) {
				*codechar++ = '\n';
				state_in->stepcount = 0;
			}
		} /* while */
	} /* switch */

	/* control should not reach here */
	return codechar - code_out;
}


_hidden size_t _isds_base64_encode_blockend(int8_t *code_out,
        base64_encodestate* state_in) {
	int8_t *codechar = code_out;

	switch (state_in->step) {
	case step_B:
		*codechar++ = base64_encode_value(state_in->result);
		*codechar++ = '=';
		*codechar++ = '=';
		break;
	case step_C:
		*codechar++ = base64_encode_value(state_in->result);
		*codechar++ = '=';
		break;
	case step_A:
		break;
	}
	*codechar++ = '\n';

	return codechar - code_out;
}
