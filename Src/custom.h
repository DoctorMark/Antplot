/********************************************************************************
* File:		custom.h
* Author:	Dr Mark Agate
*
* NOTE:		assumes sizeof(short) = 2, sizeof(long) = 4,
*			little-endian storage, LSB first in bitfields.
*/

#ifndef custom_h
#define custom_h

/********************************************************************************
* Typedefs
*/

typedef unsigned char		uint8;
typedef unsigned short		uint16;
typedef unsigned long		uint32;
typedef unsigned long long	uint64;

typedef signed char			int8;
typedef signed short		int16;
typedef signed long			int32;
typedef signed long long	int64;

typedef struct
{
	unsigned b0 : 1;
	unsigned b1 : 1;
	unsigned b2 : 1;
	unsigned b3 : 1;
	unsigned b4 : 1;
	unsigned b5 : 1;
	unsigned b6 : 1;
	unsigned b7 : 1;
} BITFIELD;

typedef struct
{
	uint8 c_low;
	uint8 c_high;
} U16;

typedef struct
{
	uint16 i_low;
	uint16 i_high;
} U32;

// Pointers to functions not advisable when using overlayed auto variables (embedded CPUs)
typedef void (*ptr_to_function)(void); /* pointer to function with no return value */

/********************************************************************************
* Defines
*/

#ifndef FALSE
#define FALSE	0
#define TRUE	(!FALSE)
#endif

// Array size macro
#define N_ELEMENTS(X) (sizeof(X) / sizeof(X[0]))

typedef unsigned char bit;

// Bit access
#define BIT0(X)	(((BITFIELD *)&(X))->b0)
#define BIT1(X)	(((BITFIELD *)&(X))->b1)
#define BIT2(X)	(((BITFIELD *)&(X))->b2)
#define BIT3(X)	(((BITFIELD *)&(X))->b3)
#define BIT4(X)	(((BITFIELD *)&(X))->b4)
#define BIT5(X)	(((BITFIELD *)&(X))->b5)
#define BIT6(X)	(((BITFIELD *)&(X))->b6)
#define BIT7(X)	(((BITFIELD *)&(X))->b7)

#define LOWBYTE(X)	(((U16 *)&(X))->c_low)
#define HIGHBYTE(X)	(((U16 *)&(X))->c_high)

#define LOW16(X)	(((U32 *)&(X))->i_low)
#define HIGH16(X)	(((U32 *)&(X))->i_high)

#define BITS0TO7(X)		(LOWBYTE(LOW16(X)))
#define BITS8TO15(X)	(HIGHBYTE(LOW16(X)))
#define BITS16TO23(X)	(LOWBYTE(HIGH16(X)))
#define BITS24TO31(X)	(HIGHBYTE(HIGH16(X)))

//*****************************************************************************
// Debugging timer
//
// Notes: 
//
#define DBG_START_TIMER()	\
	FILETIME ft; GetSystemTimeAsFileTime(&ft); \
	__int64 t0 = static_cast<__int64>(ft.dwHighDateTime) << 32 | ft.dwLowDateTime;

#define DBG_STOP_TIMER(X)	\
	GetSystemTimeAsFileTime(&ft); \
	__int64 t_x100ns = (static_cast<__int64>(ft.dwHighDateTime) << 32 | ft.dwLowDateTime) - t0; \
	int t = (int)t_x100ns; if (X > t) X = t;

#endif

/********************************************************************************
* EOF
*/



