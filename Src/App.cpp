/******************************************************************************
** File:	App.cpp	
**
** Notes:	Antplot app for Langton's Ant, reciprocals, factorials, pi and e
**
*/

#include "stdafx.h"

#include "Alu.h"
#include "Fav.h"

#define extern
#include "App.h"
#undef extern

#define LANGTONS_ANT_ITERATIONS		12000

HACCEL app_hAccelTable;

bool app_prompt;
bool app_animate;
bool app_mode_reciprocal;

uint8 app_direction;			// 0..3
uint8 app_zoom_level;

int app_favourite_index;
uint64 app_reciprocal_int;
uint64 app_factorial_int;

ALU_reg_pair app_f_reg;

ALU_reg ant_trail;
ALU_reg app_trail_integer;			// first 9977 steps
ALU_reg app_trail_fraction;			// recurring 13-byte pattern
ALU_reg app_a;						// Value of A, where L = A / B

ALU_reg r1, r2;
ALU_reg_pair hl, bc;

uint8 app_board[300][300];

typedef struct
{
	int pos_x, pos_y;
	int dir_x, dir_y;
} app_ant_type;

typedef struct
{
	char command_character;
	const char *help_string;
	ptr_to_function p_function;
} app_command_table_entry_type;

// Command table appears at the end of this file
extern const app_command_table_entry_type app_command_table[];
extern const int app_n_commands;

/******************************************************************************
** Function:
**
** Notes:
*/
char app_wait_for_keypress(void)
{
	MSG msg;

	if (app_prompt)
	{
		printf("\nPress command key: > ");
		fflush(stdin);
		app_prompt = false;
	}

	do
	{
		// Run the message loop to allow plot window to be moved etc
		if (PeekMessage(&msg, APP_hWnd, 0, 0, PM_REMOVE))
		{
			if (!TranslateAccelerator(msg.hwnd, app_hAccelTable, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else
			Sleep(50);
	} while (!_kbhit());

	return _getch();
}

/******************************************************************************
** Function:	Paint antplot display to screen
**
** Notes:
*/
void APP_paint(void)
{
	PAINTSTRUCT ps;
	HDC hdc;

	hdc = BeginPaint(APP_hWnd, &ps);

	StretchBlt(hdc, 0, 0, APP_SCREEN_WIDTH, APP_SCREEN_HEIGHT, APP_hdc, 0, 0,
		APP_SCREEN_WIDTH / (1 << app_zoom_level), APP_SCREEN_HEIGHT / (1 << app_zoom_level), SRCCOPY);

	EndPaint(APP_hWnd, &ps);
}

//*****************************************************************************
// Function:	Plot words supplied in big-endian order
//
// Notes: Provide pointer to start of register data
// Pause after plot if prompt != NULL
//
void app_plot(uint64* v, int length_words, const char* prompt, bool wait)
{
	int i;
	app_ant_type ant;

	// Clear screen:
	RECT r = { 0, 0, APP_SCREEN_WIDTH, APP_SCREEN_HEIGHT };
	HBRUSH brush = CreateSolidBrush(RGB(50, 85, 100));
	//HBRUSH brush = CreateSolidBrush(RGB(255, 255, 255));		// (RGB(0, 0, 0));
	FillRect(APP_hdc, &r, brush);				//(HBRUSH)GetStockObject(DKGRAY_BRUSH));
	DeleteObject(brush);

	// Set initial position: use centre of screen
	ant.pos_x = APP_SCREEN_WIDTH / (1 << (app_zoom_level + 1));
	ant.pos_y = APP_SCREEN_HEIGHT / (1 << (app_zoom_level + 1));

	// Set initial direction: 0 = (-1, 0), 1 = (0, 1), 2 = (1, 0), 3 = (0, -1)
	ant.dir_x = (~app_direction & 0x01) * (app_direction - 1);
	ant.dir_y = (app_direction & 0x01) * ((app_direction ^ 0x03) - 1);
	//printf("\nInitial direction: %d, %d\n\n", ant.dir_x, ant.dir_y);

	// suppress leading zeroes:
	for (i = 0; i < length_words; i++)			// Find first non-zero word
	{
		if (v[i] != 0UI64)
			break;
	}

	uint64 c = v[i];
	uint64 m = 1UI64 << 63;
	while (((c & m) == 0UI64) && (m != 0UI64))		// find first non-zero bit
		m >>= 1;

	uint32 step = 0;
	while (i < length_words)
	{
		int s = 1;									// default direction
		if ((c & m) == 0x00)						// set pixel white
		{
			SetPixel(APP_hdc, ant.pos_x, ant.pos_y, RGB(0xFF, 0xFF, 0xFF));
			if (ant.dir_x != 0)
				s = -1;
		}
		else										// set pixel black
		{
			SetPixel(APP_hdc, ant.pos_x, ant.pos_y, RGB(0x00, 0x00, 0x00));
			if (ant.dir_x == 0)
				s = -1;
		}

		int x = ant.dir_x;
		ant.dir_x = s * ant.dir_y;
		ant.dir_y = s * x;

		ant.pos_x += ant.dir_x;
		ant.pos_y += ant.dir_y;

		m >>= 1;
		if (m == 0UI64)
		{
			m = 1UI64 << 63;
			c = v[++i];
		}

		if (app_animate)
		{
			ShowWindow(APP_hWnd, SW_SHOWNORMAL);
			InvalidateRect(APP_hWnd, NULL, TRUE);
			UpdateWindow(APP_hWnd);

			printf("** Step %u. Next step or 'a' >>\n", ++step);
			if (app_wait_for_keypress() == 'a')
			{
				app_animate = false;
				printf("\n*** Animate mode OFF\n");
			}
		}
	}

	ShowWindow(APP_hWnd, SW_SHOWNORMAL);
	BringWindowToTop(APP_hWnd);
	InvalidateRect(APP_hWnd, NULL, TRUE);
	UpdateWindow(APP_hWnd);

	if (prompt != NULL)
		printf("\n%s\n\n", prompt);

	if (wait)
	{
		printf("Press any key to continue >>> ");
		app_wait_for_keypress();
	}
}

/******************************************************************************
** Function:	Generate Langton's Ant into ant_trail in big-endian order
**
** Notes:
*/
void app_generate_ant(int n_steps)
{
	app_ant_type ant;

	memset(app_board, 0xFF, sizeof(app_board));			// all white

	ant.pos_x = 150;
	ant.pos_y = 150;
	ant.dir_x = 0;
	ant.dir_y = 1;

	// Get Langton's Ant pattern in the destination register
	// Recurrent pattern starts after 9977 steps, which is (156 * 64) - 7.
	// For convenience with 64-bit integer words, set mask so ant starts 7 steps into the first word,
	// so the non-recurrent part of the ant trail is 156 words long.
	uint64 mask = 1UI64 << (63 - 7);					// start 7 steps into first word
	ALU.set(ant_trail, 0);								// initialise 1-dimensional ant trail
	int i = 0;											// index into ant_trail (uint64 values)
	for (int step = 0; step < n_steps; step++)
	{
		int s = 1;
		if (app_board[ant.pos_x][ant.pos_y] != 0)		// Pixel is white
		{
			app_board[ant.pos_x][ant.pos_y] = 0;		// turn it black
			if (ant.dir_x != 0)
				s = -1;

			ant_trail[i] |= mask;						// set 1 wherever we turn pixel black
		}
		else											// pixel is black
		{
			app_board[ant.pos_x][ant.pos_y] = 1;		// turn it white
			if (ant.dir_x == 0)
				s = -1;

			// leave the corresponding bit in ant_trail as 0 (white)
		}

		int x = ant.dir_x;
		ant.dir_x = s * ant.dir_y;
		ant.dir_y = s * x;

		ant.pos_x += ant.dir_x;
		ant.pos_y += ant.dir_y;

		mask >>= 1;
		if (mask == 0UI64)								// start the next word in ant_trail
		{
			mask = 1UI64 << 63;
			i++;
		}
	}
}

/******************************************************************************
** Function:	Plot reciprocal of a 64-bit integer value
**
** Notes:
*/
void app_plot_reciprocal(uint64 v)
{
	// Compute 0.5 / v:
	uint64 remainder = 0UI64;
	memset(hl, 0, sizeof(hl));
	hl[0] = 1UI64 << 63;
	ALU.div_u64(hl, v, 2 * ALU_LENGTH_U64, &remainder);

	// correct to 1 / v:
	ALU.adc(hl, hl, 2 * ALU_LENGTH_U64);

	printf("\n1 / %I64d (first 1024 hex digits):\n", v);
	ALU.col = 0;
	ALU.print_hex((uint8*)hl, 512, false);
	printf("\n\n");

	// Print value, and lowest integer which would give the identical pattern:
	printf("\n1 / %I64d ", v);
	int power;
	for (power = 0; power < 64; power++)
	{
		if ((v & 1UI64) != 0UI64)
			break;
		v >>= 1;
	}
	if (power > 0)
		printf(" (= 1 / (%llu * 2^%d))", v, power);
	printf(":\n");

	app_plot(hl, 2 * ALU_LENGTH_U64, NULL, false);
}

/******************************************************************************
** Function:
**
** Notes:
*/
void app_plot_factorial(bool calculate_from_scratch)
{
	uint64 i = app_factorial_int;									// assume we already have (n - 1)! in app_f_reg
	
	if ((app_factorial_int < 2) || calculate_from_scratch)			// redo full calculation
	{
		memset(app_f_reg, 0, sizeof(app_f_reg));
		app_f_reg[N_ELEMENTS(app_f_reg) - 1] = 1UI64;
		i = 2UI64;
	}

	for (; i <= app_factorial_int; i++)
	{
		if (ALU.mul_u64(app_f_reg, i, N_ELEMENTS(app_f_reg)) != 0UI64)
		{
			printf("\n*** Numeric overflow for factorial of %llu\n", app_factorial_int);
			break;
		}
	}

	printf("%llu!:\n", app_factorial_int);
	ALU.col = 0;
	
	ALU.print_hex((uint8 *)app_f_reg, sizeof(app_f_reg), true);
	// ALU.int_to_bcd(hl, &app_f_reg[ALU_LENGTH_U64]);
	// ALU.print_bcd(hl, false);
	
	printf("\n\n");

	printf("Plotting %llu!\n", app_factorial_int);
	app_plot(app_f_reg, N_ELEMENTS(app_f_reg), NULL, false);
}

/******************************************************************************
** Function:
**
** Notes:
*/
void app_toggle_animation(void)
{
	app_animate = !app_animate;
	printf("\n*** Animate mode %s\n", app_animate ? "ON" : "OFF");
}

/******************************************************************************
** Function: go back 2 values from current factorial or reciprocal
**
** Notes:
*/
void app_go_back(void)
{
	if (app_mode_reciprocal)
	{
		app_reciprocal_int = (app_reciprocal_int < 3) ? 0UI64 : app_reciprocal_int - 2;
		app_plot_reciprocal(app_reciprocal_int);
	}
	else
	{
		app_factorial_int = (app_factorial_int < 3) ? 0UI64 : app_factorial_int - 2;
		app_plot_factorial(true);
	}
}

/******************************************************************************
** Function: Calculate A and B, where L = A / B
**
** Notes:
*/
void app_calculate(void)
{
	uint64 remainder;

	app_generate_ant(LANGTONS_ANT_ITERATIONS);

	// Get just the integer & fractional parts
	// Recurrent pattern starts after 9977 steps, which is (156 * 64) - 7.
	// Number of 64-bit words = 156
	// Integer part goes at the LS end of the trail_integer register
	ALU.shr(app_trail_integer, ant_trail, ALU_LENGTH_U64 - 156);
	printf("\nAnt trail, Integer part:\n");
	ALU.print_hex((uint8*)app_trail_integer, ALU_LENGTH_BYTES, true);
	app_plot(app_trail_integer, ALU_LENGTH_U64, "Integer part", true);

	// Fractional part goes at the MS end of the trail_fraction register
	ALU.shr(app_trail_fraction, ant_trail, -156);
	printf("\nAnt trail, Fractional part, first 128 bytes:\n");
	ALU.print_hex((uint8*)app_trail_fraction, 128, true);
	app_plot(app_trail_fraction, ALU_LENGTH_U64, "Fractional part", true);

	printf("\nComputing values of A and B, where L = A / B....\n");
	// Langton's Ant recurrence pattern: 4F 27 9E 5E 87 B7 85 EF 0B D3 0C F3 49
	// The HCF of the above pattern and 0xFFFFFFFFFFFFFFFFFFFFFFFFFF is 5.
	// Compute value of C from recurrence pattern divided by 5:
	ALU.set(r1, 0);
	r1[ALU_LENGTH_U64 - 2] = app_trail_fraction[0];
	r1[ALU_LENGTH_U64 - 1] = app_trail_fraction[1] & 0xFFFFFFFFFF000000UI64;

	// r1 now contains recurrent pattern << 24, so divide it by 5 << 24 to get C
	ALU.div_u64(r1, 5UI64 << 24, ALU_LENGTH_U64, &remainder);	// r1 = C

	// Get value of B in r2, where B = 0xFFFFFFFFFFFFFFFFFFFFFFFFFF / 5:
	ALU.set(r2, 0);
	r2[ALU_LENGTH_U64 - 2] = 0x3333333333UI64;
	r2[ALU_LENGTH_U64 - 1] = 0x3333333333333333UI64;

	ALU.mul(hl, r2, app_trail_integer);								// hl = B.Li
	ALU.mov(app_a, &hl[ALU_LENGTH_U64]);						// app_a = l
	ALU.adc(app_a, r1, ALU_LENGTH_U64);							// app_a = B.Li + C

	ALU.div(bc, app_a, r2);										// bc = A/B

	app_plot(bc, 2 * ALU_LENGTH_U64, "Plotting L = A / B", true);

	// Print & Plot A:
	printf("\nValue of A:\n");
	ALU.col = 0;
	ALU.print_hex((uint8*)app_a, ALU_LENGTH_BYTES, true);
	printf("\n\n");
	app_plot(app_a, ALU_LENGTH_U64, "Plotting value of A", true);

	// Print & Plot B:
	printf("\nValue of B:\n");
	ALU.col = 0;
	ALU.print_hex((uint8*)r2, ALU_LENGTH_BYTES, true);
	printf("\n\n");
	app_plot(r2, ALU_LENGTH_U64, "\nPlotting value of B", true);

	// A as a decimal integer:
	printf("\n\nA as a decimal integer:\n\n");
	ALU.int_to_bcd(bc, app_a);
	ALU.col = 0;
	ALU.print_bcd(bc, false);
	printf("\n\n");

	// B as a decimal integer:
	printf("\n\nB as a decimal integer:\n\n");
	ALU.int_to_bcd(bc, r2);
	ALU.col = 0;
	ALU.print_bcd(bc, false);
	printf("\n\n");
}

/******************************************************************************
** Function:
**
** Notes:
*/
void app_set_direction(void)
{
	int i;

	printf("\nCurrent initial direction = %d\nEnter new initial direction (0..3): ", app_direction);
	scanf("%d", &i);
	app_direction = i & 0x03;
}

/******************************************************************************
** Function:
**
** Notes:
**
*/
void app_calc_e(void)
{
	memset(bc, 0, sizeof(bc));		// next term
	bc[0] = 1UI64 << 63;
	memset(hl, 0, sizeof(hl));		// value of e
	hl[0] = 1UI64 << 63;

	uint64 i = 2UI64;
	uint64 remainder;
	do
	{
		if (ALU.div_u64(bc, i++, 2 * ALU_LENGTH_U64, &remainder))
			break;

		ALU.adc(hl, bc, 2 * ALU_LENGTH_U64);

		if ((i & 0xFF) == 0UI64)
			printf("%lld\n", i);
	} while (true);

	// double it:
	ALU.adc(hl, hl, 2 * ALU_LENGTH_U64);

	// e as a decimal fraction:
	printf("e as a decimal fraction:\n");
	ALU.frac_to_bcd(bc, hl);
	printf(" 2.");
	ALU.col = 3;
	ALU.print_bcd(bc, true);
	printf("\n\n");

	// Plot it:
	ALU.div_u64(hl, 4, ALU_LENGTH_U64, &remainder);
	hl[0] |= 2UI64 << 62;
	app_plot(hl, ALU_LENGTH_U64, "Plotting e", false);
}

/******************************************************************************
** Function:
**
** Notes:
*/
void app_favourite()
{
	if (app_favourite_index >= N_ELEMENTS(app_favourites))
	{
		printf("\n\nStart of favourites list\n\n");
		app_favourite_index = 0;
	}

	app_plot_reciprocal(app_favourites[app_favourite_index++]);
}

/******************************************************************************
** Function:	Help command
**
** Notes:
*/
void app_help(void)
{
	printf("\n*** Antplot App V" APP_VERSION_STRING "\n");
	printf("\n*** Commands:\n");

	for (int i = 0; i < app_n_commands; i++)
		printf("*** %c - %s\n", app_command_table[i].command_character, app_command_table[i].help_string);
}

/******************************************************************************
** Function:
**
** Notes:
** Langton's Ant recurrence pattern: 4F 27 9E 5E 87 B7 85 EF 0B D3 0C F3 49
** Starts after 9977 steps (1248 bytes minus 7 steps)
*/
void app_langton(void)
{
	app_generate_ant(LANGTONS_ANT_ITERATIONS);

	// Plot whole ant trail:
	printf("\nPlotting %d steps of ant trail\n", LANGTONS_ANT_ITERATIONS);
	app_plot(ant_trail, (LANGTONS_ANT_ITERATIONS + 63) / 64, NULL, false);
}

/******************************************************************************
** Function:
**
** Notes:
*/
void app_toggle_mode(void)
{
	app_mode_reciprocal = !app_mode_reciprocal;
	printf("\n*** Reciprocal/Factorial mode: %s\n", app_mode_reciprocal ? "RECIPROCAL" : "FACTORIAL");
}

/******************************************************************************
** Function:	Next plot of reciprocal or factorial of a 64-bit integer value
**
** Notes:
*/
void app_next_plot(void)
{
	if (app_mode_reciprocal)
		app_plot_reciprocal(++app_reciprocal_int);
	else
	{
		app_factorial_int++;
		app_plot_factorial(false);
	}
}

/******************************************************************************
** Function:	Calculate pi using the BBP algorithm
**
** Notes:
*  total = 0;
*  for (i = 0; i < infinity; i++)
*  {
*      n = 8 * i;
*      r = (4/(n + 1)) - (2/(n + 4)) - (1/(n + 5)) - (1/(n + 6))
*      total += r >> (4 * i);
*  }
*/
void app_calc_pi(void)
{
	memset(bc, 0, sizeof(bc));		// value of pi

	uint64 i = 0;
	uint64 remainder;
	bool z;
	do
	{
		uint64 n = i << 3;
		int index = (int)i >> 4;											// word index
		if (index >= 2 * ALU_LENGTH_U64)
			break;

		int shift_count = 61 - (4 * (i & 0x0F));

		memset(hl, 0, sizeof(hl));
		hl[index] = 4UI64 << shift_count;
		z = ALU.div_u64(hl, n + 1, 2 * ALU_LENGTH_U64, &remainder);	// hl = (1 / (2 * (n + 1)))
		ALU.adc(bc, hl, 2 * ALU_LENGTH_U64);						// bc += hl
		
		memset(hl, 0, sizeof(hl));
		hl[index] = 2UI64 << shift_count;
		ALU.div_u64(hl, n + 4, 2 * ALU_LENGTH_U64, &remainder);		// hl = (1 / (4 * (n + 4)))
		ALU.sub(bc, hl, 2 * ALU_LENGTH_U64);						// bc -= (1 / (4(n + 4)))

		memset(hl, 0, sizeof(hl));
		hl[index] = 1UI64 << shift_count;
		ALU.div_u64(hl, n + 5, 2 * ALU_LENGTH_U64, &remainder);		// hl = (1 / (8 * (n + 5)))
		ALU.sub(bc, hl, 2 * ALU_LENGTH_U64);						// bc -= 1/(8(n+5))

		memset(hl, 0, sizeof(hl));
		hl[index] = 1UI64 << shift_count;
		ALU.div_u64(hl, n + 6, 2 * ALU_LENGTH_U64, &remainder);		// hl = (1 / (8 * (n + 6)))
		ALU.sub(bc, hl, 2 * ALU_LENGTH_U64);						// bc -= 1/(8(n+6))

		if ((++i & 0xFF) == 0UI64)
			printf("%lld\n", i);

	} while (!z);

	memcpy(r1, bc, sizeof(r1));
	ALU.mul_u64(r1, 8, ALU_LENGTH_U64);

	// e as a decimal fraction:
	printf("pi as a decimal fraction:\n");
	ALU.frac_to_bcd(bc, r1);
	printf(" 3.");
	ALU.col = 3;
	ALU.print_bcd(bc, true);
	printf("\n\n");

	// Plot it:
	app_plot(r1, ALU_LENGTH_U64, "Plotting pi", false);
}

/******************************************************************************
** Function:
**
** Notes:
*/
void app_quit(void)
{
	exit(0);
}

/******************************************************************************
** Function: Plot a random bit pattern
**
** Notes:
*/
void app_plot_random(void)
{
	for (int i = 0; i < ALU_LENGTH_U64; i++)
	{
		r1[i] = 0UI64;
		for (int j = 0; j < 64; j += 8)
			r1[i] |= (uint64)(rand() & 0xFF) << j;
	}

	printf("\nFirst 1024 hex digits of random sequence:\n");
	ALU.col = 0;
	ALU.print_hex((uint8*)r1, 512, false);
	app_plot(r1, ALU_LENGTH_U64, "Plotting random bit sequence", false);
}

/******************************************************************************
** Function:
**
** Notes:
*/
void app_test(void)
{
}

/******************************************************************************
** Function:
**
** Notes:
*/
void app_set_start_value(void)
{
	printf("\n*** Enter start value for %s: ", app_mode_reciprocal ? "reciprocal" : "factorial");

	if (app_mode_reciprocal)
	{
		scanf("%" SCNu64, &app_reciprocal_int);
		app_plot_reciprocal(app_reciprocal_int);
	}
	else		// factorial
	{
		scanf("%" SCNu64, &app_factorial_int);
		app_plot_factorial(true);
	}
}

/******************************************************************************
** Function:
**
** Notes:
*/
void app_set_zoom(void)
{
	int i;

	printf("Current zoom level is %d\n\nSet new zoom level (0..255) : ", app_zoom_level);
	scanf("%d", &i);
	if ((i < 0) || (i > 255))
		printf("*** Value out of range. Zoom level unchanged.\n");
	else
		app_zoom_level = (uint8)i;
}

/******************************************************************************
** Function:	Execute single character command
**
** Notes:
*/
void app_execute_command(char c)
{
	printf("%c\n", c);

	for (int i = 0; i < app_n_commands; i++)
	{
		// ensure lower case comparison:
		if ((app_command_table[i].command_character | 0x20) == c)
		{
			app_command_table[i].p_function();
			return;
		}
	}

	printf("*** Unrecognised command key.\n");
}

/******************************************************************************
** Function:	Task
**
** Notes:
*/
void APP_task(void)
{
	char keycode = 0;

	app_prompt = true;
	app_zoom_level = 1;
	app_help();					// Display help at start
	app_toggle_mode();			// sets app_mode_reciprocal = true & reports mode

	srand(GetTickCount());		// seed randomizer

	do
	{
		if (keycode != 0)
		{
			app_execute_command(keycode);
			keycode = 0;
			app_prompt = true;
		}

		keycode = app_wait_for_keypress();
	} while (true);
}

/******************************************************************************
** Command table:
*/
const app_command_table_entry_type app_command_table[] =
{
	{ 'a', "Toggle animation mode on/off", app_toggle_animation },
	{ 'b', "Go back 2 values", app_go_back },
	{ 'c', "Calculate integer expression for Langton's ant", app_calculate },
	{ 'd', "Set initial ant direction", app_set_direction },
	{ 'e', "Calculate & plot e", app_calc_e },
	{ 'f', "Next favourite", app_favourite },
	{ 'h', "Help", app_help },
	{ 'l', "Langton's Ant", app_langton },
	{ 'm', "Mode (factorial or reciprocal)", app_toggle_mode },
	{ 'n', "Next factorial or reciprocal plot", app_next_plot },
	{ 'p', "Calculate & plot pi", app_calc_pi },
	{ 'q', "Quit", app_quit },
	{ 'r', "Plot a random bit sequence", app_plot_random },
	// { 't', "Test", app_test },
	{ 'v', "Set value for reciprocal or factorial", app_set_start_value },
	{ 'z', "Set zoom factor", app_set_zoom }
};

const int app_n_commands = N_ELEMENTS(app_command_table);







