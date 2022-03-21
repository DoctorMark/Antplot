/******************************************************************************
** File:	Alu.cpp
**
** Notes:	Very long data word ALU functions		
*/

#include "stdafx.h"
#include "Alu.h"

ALU_type ALU;

/******************************************************************************
** Function:	Set all bytes of register to value
**
** Notes:
*/
void ALU_type::set(ALU_reg dest, uint8 byte_value)
{
	memset(dest, byte_value, ALU_LENGTH_BYTES);
}

/******************************************************************************
** Function:	Copy one register to another
**
** Notes:
*/
void ALU_type::mov(ALU_reg dest, ALU_reg src)
{
	memcpy(dest, src, ALU_LENGTH_BYTES);
}

/******************************************************************************
** Function:	Shift register right (+ve) or left (-ve), putting result in dest
**
** Notes:		Shift count is in 64-bit words, not bits. Src register unaffected
*/
void ALU_type::shr(ALU_reg dest, ALU_reg src, int word_count)
{
	set(dest, 0x00);
	if (word_count >= 0)
		memcpy(&dest[word_count], src, ALU_LENGTH_BYTES - (word_count * sizeof(uint64)));
	else
		memcpy(dest, &src[-word_count], ALU_LENGTH_BYTES + (word_count * sizeof(uint64)));
}

/******************************************************************************
** Function:	Add with carry
**
** Notes:
*/
uint8 ALU_type::adc(ALU_reg dest, ALU_reg src, int length_words)
{
	uint8 c = 0;
	for (int i = length_words - 1; i >= 0; i--)
		c = _addcarry_u64(c, src[i], dest[i], &dest[i]);

	return c;
}

/******************************************************************************
** Function:	Compare registers
**
** Notes:		Returns true if a >= b, else false
*/
bool ALU_type::greater_or_equal(ALU_reg a, ALU_reg b)
{
	for (int i = 0; i < ALU_LENGTH_U64; i++)
	{
		if (a[i] > b[i])
			return true;

		if (a[i] < b[i])
			return false;
	}

	return true;
}

/******************************************************************************
** Function:	dest -= src
**
** Notes:
*/
uint8 ALU_type::sub(ALU_reg dest, ALU_reg src, int length_words)
{
	uint8 b = 0;
	for (int i = length_words - 1; i >= 0; i--)
		b = _subborrow_u64(b, dest[i], src[i], &dest[i]);

	return b;
}

/******************************************************************************
** Function:	Add U64 value v into register pair at index i
**
** Notes:		Propagates carry up the register (or pair) as required
*/
void ALU_type::add_u64(ALU_reg dest, int i, uint64 v)
{
	uint8 c = _addcarry_u64(0, v, dest[i], &dest[i]);

	while ((c != 0) && (--i >= 0))
		c = _addcarry_u64(c, 0UI64, dest[i], &dest[i]);
}

/******************************************************************************
** Function:	Multiply register by a uint64 & return carry
**
** Notes:
*/
uint64 ALU_type::mul_u64(ALU_reg r, uint64 v, int length_words)
{
	uint64 carry;
	uint64 c2;
	int i = length_words - 1;
	r[i] = _umul128(r[i], v, &carry);

	while (--i >= 0)
	{
		r[i] = _umul128(r[i], v, &c2);
		add_u64(r, i, carry);
		carry = c2;
	}

	return carry;
}

/******************************************************************************
** Function:
**
** Notes:
*/
void ALU_type::mul(ALU_reg_pair dest, ALU_reg x, ALU_reg y)
{
	uint64 high64;

	memset(dest, 0, sizeof(ALU_reg_pair));

	// For each U64 of multiplier:
	for (int i = ALU_LENGTH_U64 - 1; i >= 0; i--)
	{
		int k = ALU_LENGTH_U64 + i;						// index into result

		// for each byte of multiplicand:
		for (int j = ALU_LENGTH_U64 - 1; j >= 0; j--)
		{
			uint64 low64 = _umul128(x[i], y[j], &high64);
			add_u64(dest, k--, low64);					// note result index decremented
			add_u64(dest, k, high64);
		}
	}
}

/******************************************************************************
** Function:	Divide register by a uint64 & set remainder
**
** Notes:		Returns true when result is zero
*/
bool ALU_type::div_u64(ALU_reg n, uint64 d, int length_words, uint64* p_remainder)
{
	*p_remainder = 0UI64;
	bool z = true;								// assume result is 0

	if (d == 0UI64)								// trap divide by zero
	{
		memset(n, 0xFF, length_words * sizeof(uint64));
		return false;
	}
	// else:

	for (int i = 0; i < length_words; i++)
	{
		n[i] = _udiv128(*p_remainder, n[i], d, p_remainder);
		if ((n[i] | *p_remainder) != 0UI64)
			z = false;
	}

	return z;
}

/******************************************************************************
** Function:	Divide n by d
**
** Notes:
** Q := 0                  -- Initialize quotient and remainder to zero
** R := 0                     
** for i := n - 1 .. 0 do  -- Where n is number of bits in N
**   R := R << 1           -- Left-shift R by 1 bit
**   R(0) := N(i)          -- Set the least-significant bit of R equal to bit i of the numerator
**   if R >= D then
**     R := R - D
**     Q(i) := 1
**   end
** end
*/
void ALU_type::div(ALU_reg_pair dest, ALU_reg n, ALU_reg d)
{
	int i;
	uint64 c, mask;

	memset(dest, 0, sizeof(ALU_reg_pair));	// Q := 0
	set(w1, 0x00);							// R := 0

	// loop MS down to LS for integer part:
	for (i = 0; i < ALU_LENGTH_U64; i++)
	{
		c = n[i];
		for (mask = 1UI64 << 63; mask != 0UI64; mask >>= 1)
		{
			adc(w1, w1, ALU_LENGTH_U64);			// R <<= 1
			if ((c & mask) != 0UI64)
				w1[ALU_LENGTH_U64 - 1] |= 0x01;		// R(0) := N(i)

			if (greater_or_equal(w1, d))			// if R >= D then
			{
				sub(w1, d, ALU_LENGTH_U64);			// R := R - D
				dest[i] |= mask;					// Q(i) := 1
			}
		}
	}

	// continue loop for fractional part:
	for (i = ALU_LENGTH_U64; i < 2 * ALU_LENGTH_U64; i++)
	{
		for (mask = 1UI64 << 63; mask != 0UI64; mask >>= 1)
		{
			adc(w1, w1, ALU_LENGTH_U64);			// R <<= 1
													// R(0) := N(i), but N(i) = 0

			if (greater_or_equal(w1, d))			// if R >= D then
			{
				sub(w1, d, ALU_LENGTH_U64);			// R := R - D
				dest[i] |= mask;					// Q(i) := 1
			}
		}
	}
}

/******************************************************************************
** Function:	Get byte from array of u64s
**
** Notes:		Array of uint64 values stored big-endian, but each uint64 is little-endian
**				indices go 7,6,5,4,3,2,1,0,15,14,13,12,11,10,9,8 etc.
*/
uint8 ALU_type::get_byte(uint8* p, int i)
{
	return p[i ^ 0x07];
}

/******************************************************************************
** Function:	Set byte in array of u64s
**
** Notes:		Array of uint64 values stored big-endian, but each uint64 is little-endian
**				indices go 7,6,5,4,3,2,1,0,15,14,13,12,11,10,9,8 etc.
*/
void ALU_type::set_byte(uint8 *p, int i, uint8 v)
{
	p[i ^ 0x07] = v;
}

/******************************************************************************
** Function:
**
** Notes:
*/
void ALU_type::print_hex(uint8* r, int length_bytes, bool skip_leading_zeroes)
{
	int i = 0;

	if (skip_leading_zeroes)
	{
		while ((get_byte(r, i) == 0x00) && (i < length_bytes - 1))
			i++;
	}

	int c = 0;
	while (i < length_bytes)
	{
		if ((c & 31) == 0)
			printf("\n%04X: ", i);
		c++;

		printf("%02X ", get_byte(r, i++));
	}

	printf("\n\n");
}

/******************************************************************************
** Function:
**
** Notes:
*/
void ALU_type::int_to_bcd(ALU_reg_pair dest, ALU_reg src)
{
	memset(dest, 0x00, sizeof(ALU_reg_pair));

	bool ms_digit = false;
	mov(w1, src);								// don't destroy src
	int dest_index = 2 * ALU_LENGTH_BYTES - 1;	// NB byte index
	uint8 digit_pair = 0;
	uint64 remainder;
	do
	{
		// Get next 4-bit BCD digit in remainder
		if (div_u64(w1, 10UI64, ALU_LENGTH_U64, &remainder))	// returns true when finished
			break;												// finished (w1 == 0)

		// Put digit in output
		if (ms_digit)
		{
			digit_pair |= (uint8)remainder << 4;
			set_byte((uint8*)dest, dest_index--, digit_pair);
		}
		else
		{
			digit_pair = (uint8)remainder;
			set_byte((uint8*)dest, dest_index, digit_pair);
		}

		ms_digit = !ms_digit;

	} while (true);
}

/******************************************************************************
** Function:	Fraction to BCD
**
** Notes:
** Two digits per dest_index. Number of hex digits in src = ALU_LENGTH_BYTES * 2.
** Number of decimal digits = log10(16) = 1.2041
** So generate 1.2 times 2 * ALU_LENGTH_BYTES of output,
** i.e. stop when dest_index > ALU_LENGTH_BYTES + ((10 * ALU_LENGTH_BYTES) / 49)
*/
void ALU_type::frac_to_bcd(ALU_reg_pair dest, ALU_reg src)
{
	memset(dest, 0x00, sizeof(ALU_reg_pair));

	bool ms_digit = true;
	mov(w1, src);								// don't destroy src
	int dest_index = 0;							// NB byte index
	uint8 digit_pair;
	do
	{
		uint8 c = (uint8)mul_u64(w1, 10UI64, ALU_LENGTH_U64);

		// Put digit in output
		if (ms_digit)
		{
			digit_pair = c << 4;
			set_byte((uint8 *)dest, dest_index, digit_pair);
		}
		else
		{
			digit_pair |= c;
			set_byte((uint8*)dest, dest_index++, digit_pair);
		}

		ms_digit = !ms_digit;

	} while (dest_index < (ALU_LENGTH_BYTES + ((10 * ALU_LENGTH_BYTES) / 49)));
}

/******************************************************************************
** Function:
**
** Notes:
*/
void ALU_type::print_digit(uint8 digit)
{
	printf("%1X", digit);
	if (++col > 59)
	{
		printf("\n");
		col = 0;
	}
}

/******************************************************************************
** Function:	Print integer or fractional value supplied in BCD
**
** Notes:		Initialise col according to number of characters already printed
**				before caqlling
*/
void ALU_type::print_bcd(ALU_reg_pair v, bool fraction)
{
	uint8 digit_pair;
	int i = 0;
	int end_index = 2 * ALU_LENGTH_BYTES;		// index of final pair of digits + 1

	if (!fraction)								// suppress leading zeroes
	{
		while ((get_byte((uint8 *)v, i) == 0x00) && (i < end_index))
			i++;

		digit_pair = get_byte((uint8 *)v, i);
		if ((digit_pair & 0xF0) == 0x00)		// suppress MS digit of first value
		{
			print_digit(digit_pair);
			i++;
		}
	}
	else										// suppress trailing zeroes
	{
		do
		{
			if (get_byte((uint8 *)v, --end_index) != 0x00)		// end_index references final non-zero digit pair
				break;
		} while (end_index > 1);

		end_index++;							// last non-zero pair + 1
	}

	// Print pairs of digits:
	while (i < end_index)
	{
		digit_pair = get_byte((uint8 *)v, i++);
		print_digit(digit_pair >> 4);
		print_digit(digit_pair & 0x0F);
	}
}


