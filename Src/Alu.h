/******************************************************************************
** File:	Alu.h
**
** Notes:	Very long integer ALU functions, big-endian format
** NB array registers go MS to LS, but each array element (uint64) is little-endian.
*/

#define ALU_LENGTH_U64		2048
#define ALU_LENGTH_BYTES	(ALU_LENGTH_U64 * sizeof(uint64))

typedef uint64 ALU_reg[ALU_LENGTH_U64];
typedef uint64 ALU_reg_pair[2 * ALU_LENGTH_U64];

class ALU_type
{
private:
	ALU_reg w1;

	void print_digit(uint8 digit);

public:
	int col;

	void set(ALU_reg dest, uint8 byte_value);
	void mov(ALU_reg dest, ALU_reg src);
	void shr(ALU_reg dest, ALU_reg src, int word_count);
	uint8 adc(ALU_reg dest, ALU_reg src, int length_words);
	void add_u64(ALU_reg dest, int i, uint64 v);
	uint8 sub(ALU_reg dest, ALU_reg src, int length_words);
	bool greater_or_equal(ALU_reg a, ALU_reg b);

	uint64 mul_u64(ALU_reg r, uint64 v, int length_words);
	void mul(ALU_reg_pair dest, ALU_reg x, ALU_reg y);
	bool div_u64(ALU_reg n, uint64 d, int length_words, uint64 *p_remainder);
	void div(ALU_reg_pair dest, ALU_reg n, ALU_reg d);

	uint8 get_byte(uint8* p, int i);
	void set_byte(uint8* p, int i, uint8 v);
	void print_hex(uint8 *r, int length_bytes, bool skip_leading_zeroes);

	void int_to_bcd(ALU_reg_pair dest, ALU_reg src);
	void frac_to_bcd(ALU_reg_pair dest, ALU_reg src);
	void print_bcd(ALU_reg_pair v, bool fraction);
};

extern ALU_type ALU;

