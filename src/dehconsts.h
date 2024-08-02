//------------------------------------------------------------------------
//  DEHACKED
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2016 Andrew Apted
//  Copyright (C) 1997-2003 André Majorel et al
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//------------------------------------------------------------------------
//
//  Based on Yadex which incorporated code from DEU 5.21 that was put
//  in the public domain in 1994 by Raphaël Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------

// THANKS TO Isaac Colón (https://github.com/iccolon818) for this contribution, and compiling all
// these data lists, as based on the data from the DOOM executable. I did reformatting.

#ifndef __EUREKA_DEHCONSTS_H__
#define __EUREKA_DEHCONSTS_H__

#include "m_strings.h"
#include "w_dehacked.h"

namespace dehacked
{
static const int THING_NUM_TO_TYPE[] =
{
	  -1,   -1, 3004,    9,   64,   -1,   66,   -1,
	  -1,   67,   -1,   65, 3001, 3002,   58, 3005,
	3003,   -1,   69, 3006,    7,   68,   16,   71,
	  84,   72,   88,   89,   87,   -1,   -1, 2035,	// 32
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	  -1,   -1,   14,   -1, 2018, 2019, 2014, 2015,
	   5,   13,    6,   39,   38,   40, 2011, 2012,
	2013, 2022, 2023, 2024, 2025, 2026, 2045,   83,	// 64
	2007, 2048, 2010, 2046, 2047,   17, 2008, 2049,
	   8, 2006, 2002, 2005, 2003, 2004, 2001,   82,
	  85,   86, 2028,   30,   31,   32,   33,   37,
	  36,   41,   42,   43,   44,   45,   46,   55,	// 96
	  56,   57,   47,   48,   34,   35,   49,   50,
	  51,   52,   53,   59,   60,   61,   62,   63,
	  22,   15,   18,   21,   23,   20,   19,   10,
	  12,   28,   24,   27,   29,   25,   26,   54,	// 128
	  70,   73,   74,   75,   76,   77,   78,   79,	// 136
	  80,   81, 5001, 5002,  888,   -1,   -1, 2016,	// 144
	2017	// 145
};

static const int THING_NUM_TO_SPRITE[] =
{
	0, 149, 174, 207, 241, 281, 321, 316, 	// 8
	311, 362, 357, 406, 442, 475, 475, 502,	// 16
	527, 522, 556, 585, 601, 632, 674, 701,	// 24
	726, 763, 778, 784, 0, 787, 791, 806,	// 32
	97, 102, 114, 107, 115, 667, 93, 90,	// 40
	130, 142, 0, 123, 802, 804, 816, 822,	// 48
	828, 830, 832, 838, 836, 834, 840, 841,	// 56
	842, 848, 852, 853, 861, 862, 868, 857,	// 64
	870, 871, 872, 873, 874, 875, 876, 877,	// 72
	878, 879, 880, 881, 882, 883, 884, 885,	// 80
	959, 963, 886, 907, 908, 909, 910, 913,	// 88
	924, 917, 921, 914, 926, 930, 934, 938,	// 96
	942, 946, 906, 916, 911, 912, 888, 902,	// 104
	903, 904, 905, 902, 904, 903, 905, 888,	// 112
	515, 164, 193, 495, 600, 461, 226, 173,	// 120
	173, 894, 895, 896, 897, 899, 900, 915,	// 128
	813, 950, 951, 952, 953, 954, 955, 956,	// 136
	957, 958, 967, 967, 972, 1042, 1049, 1054,	// 144
	1055	// 145
};


static const frame_t FRAMES[] =
{
	{0, 0, false}, {1, 4, false}, {2, 0, false}, {2, 0, false}, {2, 0, false}, {2, 1, false},
	{2, 2, false}, {2, 3, false}, {2, 2, false}, {2, 1, false}, {3, 0, false}, {3, 0, false},
	{3, 0, false}, {3, 0, false}, {3, 1, false}, {3, 2, false}, {3, 1, false}, {4, 0, true},
	{1, 0, false}, {1, 0, false}, {1, 0, false}, {1, 0, false}, {1, 0, false}, {1, 1, false},
	{1, 2, false}, {1, 3, false}, {1, 2, false}, {1, 1, false}, {1, 0, false}, {1, 0, false},
	// 30
	{5, 0, true}, {5, 1, true}, {6, 0, false}, {6, 0, false}, {6, 0, false}, {6, 0, false},
	{6, 0, false}, {6, 1, false}, {6, 2, false}, {6, 3, false}, {6, 4, false}, {6, 5, false},
	{6, 6, false}, {6, 7, false}, {6, 0, false}, {6, 1, false}, {6, 0, false}, {6, 8, true},
	{6, 9, true}, {7, 0, false}, {7, 0, false}, {7, 0, false}, {7, 0, false}, {7, 1, false},
	{7, 1, false}, {8, 0, true}, {8, 1, true}, {9, 0, false}, {9, 0, false}, {9, 0, false},
	 // 60
	{9, 1, false}, {9, 1, false}, {9, 1, false}, {10, 0, true}, {10, 1, true}, {10, 2, true},
	{10, 3, true}, {11, 2, false}, {11, 3, false}, {11, 2, false}, {11, 2, false}, {11, 0, false},
	{11, 1, false}, {11, 1, false}, {12, 0, false}, {12, 0, false}, {12, 0, false}, {12, 0, false},
	{12, 1, false}, {13, 0, true}, {13, 1, true}, {14, 0, false}, {14, 0, false}, {14, 0, false},
	{14, 0, false}, {14, 1, false}, {14, 1, false}, {14, 1, false}, {15, 0, true}, {15, 1, true},
	// 90
	{16, 2, false}, {16, 1, false}, {16, 0, false}, {17, 0, true}, {17, 1, false}, {17, 2, false},
	{17, 3, false}, {18, 0, true}, {18, 1, true}, {18, 2, true}, {18, 3, true}, {18, 4, true},
	{19, 0, true}, {19, 1, true}, {19, 2, true}, {19, 3, true}, {19, 4, true}, {20, 0, true},
	{20, 1, true}, {21, 0, true}, {21, 1, true}, {21, 2, true}, {21, 3, true}, {21, 4, true},
	{22, 0, true}, {23, 0, true}, {23, 1, true}, {24, 0, true}, {24, 1, true}, {24, 2, true},
	// 120
	{24, 3, true}, {24, 4, true}, {24, 5, true}, {25, 0, true}, {25, 1, true}, {25, 2, true},
	{25, 3, true}, {22, 1, true}, {22, 2, true}, {22, 3, true}, {26, 0, true}, {26, 1, true},
	{26, 0, true}, {26, 1, true}, {26, 2, true}, {26, 3, true}, {26, 4, true}, {26, 5, true},
	{26, 6, true}, {26, 7, true}, {26, 8, true}, {26, 9, true}, {27, 0, true}, {27, 1, true},
	{27, 0, true}, {27, 1, true}, {27, 2, true}, {27, 3, true}, {27, 4, true}, {28, 0, false},
	// 150
	{28, 0, false}, {28, 1, false}, {28, 2, false}, {28, 3, false}, {28, 4, false}, {28, 5, true},
	{28, 6, false}, {28, 6, false}, {28, 7, false}, {28, 8, false}, {28, 9, false}, {28, 10, false},
	{28, 11, false}, {28, 12, false}, {28, 13, false}, {28, 14, false}, {28, 15, false}, {28, 16, false},
	{28, 17, false}, {28, 18, false}, {28, 19, false}, {28, 20, false}, {28, 21, false}, {28, 22, false},
	{29, 0, false}, {29, 1, false}, {29, 0, false}, {29, 0, false}, {29, 1, false}, {29, 1, false},
	// 180
	{29, 2, false}, {29, 2, false}, {29, 3, false}, {29, 3, false}, {29, 4, false}, {29, 5, false},
	{29, 4, false}, {29, 6, false}, {29, 6, false}, {29, 7, false}, {29, 8, false}, {29, 9, false},
	{29, 10, false}, {29, 11, false}, {29, 12, false}, {29, 13, false}, {29, 14, false}, {29, 15, false},
	{29, 16, false}, {29, 17, false}, {29, 18, false}, {29, 19, false}, {29, 20, false}, {29, 10, false},
	{29, 9, false}, {29, 8, false}, {29, 7, false}, {30, 0, false}, {30, 1, false}, {30, 0, false},
	// 210
	{30, 0, false}, {30, 1, false}, {30, 1, false}, {30, 2, false}, {30, 2, false}, {30, 3, false},
	{30, 3, false}, {30, 4, false}, {30, 5, true}, {30, 4, false}, {30, 6, false}, {30, 6, false},
	{30, 7, false}, {30, 8, false}, {30, 9, false}, {30, 10, false}, {30, 11, false}, {30, 12, false},
	{30, 13, false}, {30, 14, false}, {30, 15, false}, {30, 16, false}, {30, 17, false}, {30, 18, false},
	{30, 19, false}, {30, 20, false}, {30, 11, false}, {30, 10, false}, {30, 9, false}, {30, 8, false},
	// 240
	{30, 7, false}, {31, 0, false}, {31, 1, false}, {31, 0, false}, {31, 0, false}, {31, 1, false},
	{31, 1, false}, {31, 2, false}, {31, 2, false}, {31, 3, false}, {31, 3, false}, {31, 4, false},
	{31, 4, false}, {31, 5, false}, {31, 5, false}, {31, 6, true}, {31, 6, true}, {31, 7, true},
	{31, 8, true}, {31, 9, true}, {31, 10, true}, {31, 11, true}, {31, 12, true}, {31, 13, true},
	{31, 14, true}, {31, 15, true}, {31, 26, true}, {31, 27, true}, {31, 28, true}, {31, 16, false},
	// 270
	{31, 16, false}, {31, 16, false}, {31, 17, false}, {31, 18, false}, {31, 19, false}, {31, 20, false},
	{31, 21, false}, {31, 22, false}, {31, 23, false}, {31, 24, false}, {31, 25, false}, {32, 0, true},
	{32, 1, true}, {32, 0, true}, {32, 1, true}, {32, 2, true}, {32, 1, true}, {32, 2, true},
	{32, 1, true}, {32, 2, true}, {32, 3, true}, {32, 2, true}, {32, 3, true}, {32, 2, true},
	{32, 3, true}, {32, 4, true}, {32, 3, true}, {32, 4, true}, {32, 3, true}, {32, 4, true},
	// 300
	{32, 5, true}, {32, 4, true}, {32, 5, true}, {32, 4, true}, {32, 5, true}, {32, 6, true},
	{32, 7, true}, {32, 6, true}, {32, 7, true}, {32, 6, true}, {32, 7, true}, {17, 1, false},
	{17, 2, false}, {17, 1, false}, {17, 2, false}, {17, 3, false}, {33, 0, true}, {33, 1, true},
	{34, 0, true}, {34, 1, true}, {34, 2, true}, {35, 0, false}, {35, 1, false}, {35, 0, false},
	{35, 0, false}, {35, 1, false}, {35, 1, false}, {35, 2, false}, {35, 2, false}, {35, 3, false},
	// 330
	{35, 3, false}, {35, 4, false}, {35, 4, false}, {35, 5, false}, {35, 5, false}, {35, 6, false},
	{35, 6, false}, {35, 7, false}, {35, 8, false}, {35, 9, true}, {35, 9, true}, {35, 10, false},
	{35, 10, false}, {35, 11, false}, {35, 11, false}, {35, 11, false}, {35, 12, false}, {35, 13, false},
	{35, 14, false}, {35, 15, false}, {35, 16, false}, {35, 16, false}, {35, 15, false}, {35, 14, false},
	{35, 13, false}, {35, 12, false}, {35, 11, false}, {36, 0, true}, {36, 1, true}, {22, 1, true},
	// 360
	{22, 2, true}, {22, 3, true}, {37, 0, false}, {37, 1, false}, {37, 0, false}, {37, 0, false},
	{37, 1, false}, {37, 1, false}, {37, 2, false}, {37, 2, false}, {37, 3, false}, {37, 3, false},
	{37, 4, false}, {37, 4, false}, {37, 5, false}, {37, 5, false}, {37, 6, false}, {37, 7, true},
	{37, 8, false}, {37, 6, false}, {37, 7, true}, {37, 8, false}, {37, 6, false}, {37, 7, true},
	{37, 8, false}, {37, 6, false}, {37, 9, false}, {37, 9, false}, {37, 10, false}, {37, 11, false},
	// 390
	{37, 12, false}, {37, 13, false}, {37, 14, false}, {37, 15, false}, {37, 16, false}, {37, 17, false},
	{37, 18, false}, {37, 19, false}, {37, 17, false}, {37, 16, false}, {37, 15, false}, {37, 14, false},
	{37, 13, false}, {37, 12, false}, {37, 11, false}, {37, 10, false}, {38, 0, false}, {38, 1, false},
	{38, 0, false}, {38, 0, false}, {38, 1, false}, {38, 1, false}, {38, 2, false}, {38, 2, false},
	{38, 3, false}, {38, 3, false}, {38, 4, false}, {38, 5, true}, {38, 4, true}, {38, 5, false},
	// 420
	{38, 6, false}, {38, 6, false}, {38, 7, false}, {38, 8, false}, {38, 9, false}, {38, 10, false},
	{38, 11, false}, {38, 12, false}, {38, 13, false}, {38, 14, false}, {38, 15, false}, {38, 16, false},
	{38, 17, false}, {38, 18, false}, {38, 19, false}, {38, 13, false}, {38, 12, false}, {38, 11, false},
	{38, 10, false}, {38, 9, false}, {38, 8, false}, {38, 7, false}, {0, 0, false}, {0, 1, false},
	{0, 0, false}, {0, 0, false}, {0, 1, false}, {0, 1, false}, {0, 2, false}, {0, 2, false},
	// 450
	{0, 3, false}, {0, 3, false}, {0, 4, false}, {0, 5, false}, {0, 6, false}, {0, 7, false},
	{0, 7, false}, {0, 8, false}, {0, 9, false}, {0, 10, false}, {0, 11, false}, {0, 12, false},
	{0, 13, false}, {0, 14, false}, {0, 15, false}, {0, 16, false}, {0, 17, false}, {0, 18, false},
	{0, 19, false}, {0, 20, false}, {0, 12, false}, {0, 11, false}, {0, 10, false}, {0, 9, false},
	{0, 8, false}, {39, 0, false}, {39, 1, false}, {39, 0, false}, {39, 0, false}, {39, 1, false},
	// 480
	{39, 1, false}, {39, 2, false}, {39, 2, false}, {39, 3, false}, {39, 3, false}, {39, 4, false},
	{39, 5, false}, {39, 6, false}, {39, 7, false}, {39, 7, false}, {39, 8, false}, {39, 9, false},
	{39, 10, false}, {39, 11, false}, {39, 12, false}, {39, 13, false}, {39, 13, false}, {39, 12, false},
	{39, 11, false}, {39, 10, false}, {39, 9, false}, {39, 8, false}, {40, 0, false}, {40, 0, false},
	{40, 1, false}, {40, 2, false}, {40, 3, true}, {40, 4, false}, {40, 4, false}, {40, 5, false},
	// 510
	{40, 6, false}, {40, 7, false}, {40, 8, false}, {40, 9, false}, {40, 10, false}, {40, 11, false},
	{40, 11, false}, {40, 10, false}, {40, 9, false}, {40, 8, false}, {40, 7, false}, {40, 6, false},
	{41, 0, true}, {41, 1, true}, {41, 2, true}, {41, 3, true}, {41, 4, true}, {42, 0, false},
	{42, 1, false}, {42, 0, false}, {42, 0, false}, {42, 1, false}, {42, 1, false}, {42, 2, false},
	{42, 2, false}, {42, 3, false}, {42, 3, false}, {42, 4, false}, {42, 5, false}, {42, 6, false},
	// 540
	{42, 7, false}, {42, 7, false}, {42, 8, false}, {42, 9, false}, {42, 10, false}, {42, 11, false},
	{42, 12, false}, {42, 13, false}, {42, 14, false}, {42, 14, false}, {42, 13, false}, {42, 12, false},
	{42, 11, false}, {42, 10, false}, {42, 9, false}, {42, 8, false}, {43, 0, false}, {43, 1, false},
	{43, 0, false}, {43, 0, false}, {43, 1, false}, {43, 1, false}, {43, 2, false}, {43, 2, false},
	{43, 3, false}, {43, 3, false}, {43, 4, false}, {43, 5, false}, {43, 6, false}, {43, 7, false},
	// 570
	{43, 7, false}, {43, 8, false}, {43, 9, false}, {43, 10, false}, {43, 11, false}, {43, 12, false},
	{43, 13, false}, {43, 14, false}, {43, 14, false}, {43, 13, false}, {43, 12, false}, {43, 11, false},
	{43, 10, false}, {43, 9, false}, {43, 8, false}, {44, 0, true}, {44, 1, true}, {44, 0, true},
	{44, 1, true}, {44, 2, true}, {44, 3, true}, {44, 2, true}, {44, 3, true}, {44, 4, true},
	{44, 4, true}, {44, 5, true}, {44, 6, true}, {44, 7, true}, {44, 8, true}, {44, 9, false},
	// 600
	{44, 10, false}, {45, 0, false}, {45, 1, false}, {45, 0, false}, {45, 0, false}, {45, 1, false},
	{45, 1, false}, {45, 2, false}, {45, 2, false}, {45, 3, false}, {45, 3, false}, {45, 4, false},
	{45, 4, false}, {45, 5, false}, {45, 5, false}, {45, 0, true}, {45, 6, true}, {45, 7, true},
	{45, 7, true}, {45, 8, false}, {45, 8, false}, {45, 9, false}, {45, 10, false}, {45, 11, false},
	{45, 12, false}, {45, 13, false}, {45, 14, false}, {45, 15, false}, {45, 16, false}, {45, 17, false},
	// 630
	{45, 18, false}, {45, 18, false}, {46, 0, false}, {46, 1, false}, {46, 0, false}, {46, 0, false},
	{46, 0, false}, {46, 1, false}, {46, 1, false}, {46, 2, false}, {46, 2, false}, {46, 3, false},
	{46, 3, false}, {46, 4, false}, {46, 4, false}, {46, 5, false}, {46, 5, false}, {46, 0, true},
	{46, 6, true}, {46, 7, true}, {46, 7, true}, {46, 8, false}, {46, 8, false}, {46, 9, false},
	{46, 10, false}, {46, 11, false}, {46, 12, false}, {46, 13, false}, {46, 14, false}, {46, 15, false},
	// 660
	{46, 15, false}, {46, 14, false}, {46, 13, false}, {46, 12, false}, {46, 11, false}, {46, 10, false},
	{46, 9, false}, {47, 0, true}, {47, 1, true}, {48, 0, true}, {48, 1, true}, {48, 2, true},
	{48, 3, true}, {48, 4, true}, {49, 0, false}, {49, 1, false}, {49, 0, false}, {49, 0, false},
	{49, 1, false}, {49, 1, false}, {49, 2, false}, {49, 2, false}, {49, 3, false}, {49, 3, false},
	{49, 4, false}, {49, 5, false}, {49, 4, false}, {49, 5, false}, {49, 4, false}, {49, 5, false},
	// 690
	{49, 6, false}, {49, 7, false}, {49, 8, false}, {49, 9, false}, {49, 10, false}, {49, 11, false},
	{49, 12, false}, {49, 13, false}, {49, 14, false}, {49, 15, false}, {49, 15, false}, {50, 0, false},
	{50, 0, false}, {50, 0, false}, {50, 1, false}, {50, 1, false}, {50, 2, false}, {50, 2, false},
	{50, 3, false}, {50, 4, false}, {50, 5, true}, {50, 5, true}, {50, 6, false}, {50, 6, false},
	{50, 7, true}, {50, 8, true}, {50, 9, true}, {50, 10, true}, {50, 11, true}, {50, 12, true},
	// 720
	{50, 12, false}, {50, 11, false}, {50, 10, false}, {50, 9, false}, {50, 8, false}, {50, 7, false},
	{51, 0, false}, {51, 1, false}, {51, 0, false}, {51, 0, false}, {51, 1, false}, {51, 1, false},
	{51, 2, false}, {51, 2, false}, {51, 3, false}, {51, 3, false}, {51, 4, false}, {51, 5, false},
	{51, 6, true}, {51, 5, false}, {51, 6, true}, {51, 5, false}, {51, 7, false}, {51, 7, false},
	{51, 8, false}, {51, 9, false}, {51, 10, false}, {51, 11, false}, {51, 12, false}, {51, 13, false},
	// 750
	{51, 14, false}, {51, 15, false}, {51, 16, false}, {51, 17, false}, {51, 18, false}, {51, 19, false},
	{51, 20, false}, {51, 21, false}, {51, 12, false}, {51, 11, false}, {51, 10, false}, {51, 9, false},
	{51, 8, false}, {52, 0, false}, {52, 0, false}, {52, 1, false}, {52, 2, false}, {52, 3, false},
	{52, 4, false}, {52, 5, false}, {52, 6, false}, {52, 7, false}, {52, 8, false}, {52, 9, false},
	{52, 10, false}, {52, 11, false}, {52, 12, false}, {52, 12, false}, {53, 0, false}, {53, 1, false},
	// 780
	{53, 0, false}, {53, 0, false}, {53, 0, false}, {53, 0, false}, {51, 0, false}, {51, 0, false},
	{51, 0, false}, {54, 0, true}, {54, 1, true}, {54, 2, true}, {54, 3, true}, {32, 0, true},
	{32, 1, true}, {32, 2, true}, {32, 3, true}, {32, 4, true}, {32, 5, true}, {32, 6, true},
	{32, 7, true}, {22, 1, true}, {22, 2, true}, {22, 3, true}, {55, 0, false}, {55, 1, true},
	{56, 0, false}, {56, 1, true}, {57, 0, false}, {57, 1, false}, {58, 0, true}, {58, 1, true},
	// 810
	{58, 2, true}, {58, 3, true}, {58, 4, true}, {59, 0, true}, {59, 1, true}, {59, 2, true},
	{60, 0, false}, {60, 1, false}, {60, 2, false}, {60, 3, false}, {60, 2, false}, {60, 1, false},
	{61, 0, false}, {61, 1, false}, {61, 2, false}, {61, 3, false}, {61, 2, false}, {61, 1, false},
	{62, 0, false}, {62, 1, true}, {63, 0, false}, {63, 1, true}, {64, 0, false}, {64, 1, true},
	{65, 0, false}, {65, 1, true}, {66, 0, false}, {66, 1, true}, {67, 0, false}, {67, 1, true},
	// 840
	{68, 0, false}, {69, 0, false}, {70, 0, true}, {70, 1, true}, {70, 2, true}, {70, 3, true},
	{70, 2, true}, {70, 1, true}, {71, 0, true}, {71, 1, true}, {71, 2, true}, {71, 3, true},
	{72, 0, true}, {73, 0, true}, {73, 1, true}, {73, 2, true}, {73, 3, true}, {74, 0, true},
	{74, 1, true}, {74, 2, true}, {74, 3, true}, {75, 0, true}, {76, 0, true}, {76, 1, true},
	{76, 2, true}, {76, 3, true}, {76, 2, true}, {76, 1, true}, {77, 0, true}, {77, 1, false},
	// 870
	{78, 0, false}, {79, 0, false}, {80, 0, false}, {81, 0, false}, {82, 0, false}, {83, 0, false},
	{84, 0, false}, {85, 0, false}, {86, 0, false}, {87, 0, false}, {88, 0, false}, {89, 0, false},
	{90, 0, false}, {91, 0, false}, {92, 0, false}, {93, 0, false}, {94, 0, true}, {95, 0, false},
	{96, 0, false}, {96, 1, false}, {96, 2, false}, {96, 1, false}, {28, 13, false}, {28, 18, false},
	{97, 0, false}, {98, 0, false}, {99, 0, false}, {100, 0, true}, {100, 0, true}, {101, 0, false},
	// 900
	{102, 0, false}, {102, 0, false}, {103, 0, false}, {104, 0, false}, {105, 0, false}, {106, 0, false},
	{107, 0, false}, {108, 0, false}, {109, 0, false}, {110, 0, false}, {111, 0, false}, {112, 0, true},
	{113, 0, true}, {114, 0, false}, {115, 0, false}, {116, 0, false}, {117, 0, false}, {118, 0, true},
	{118, 1, true}, {118, 2, true}, {118, 1, true}, {119, 0, true}, {119, 1, true}, {119, 2, true},
	{120, 0, false}, {120, 1, false}, {121, 0, true}, {121, 1, true}, {121, 2, true}, {121, 3, true},
	// 930
	{122, 0, true}, {122, 1, true}, {122, 2, true}, {122, 3, true}, {123, 0, true}, {123, 1, true},
	{123, 2, true}, {123, 3, true}, {124, 0, true}, {124, 1, true}, {124, 2, true}, {124, 3, true},
	{125, 0, true}, {125, 1, true}, {125, 2, true}, {125, 3, true}, {126, 0, true}, {126, 1, true},
	{126, 2, true}, {126, 3, true}, {127, 0, false}, {128, 0, false}, {129, 0, false}, {130, 0, false},
	{131, 0, false}, {132, 0, false}, {133, 0, false}, {134, 0, false}, {135, 0, false}, {136, 0, true},
	// 960
	{136, 1, true}, {136, 2, true}, {136, 3, true}, {137, 0, true}, {137, 1, true}, {137, 2, true},
	{137, 3, true}, {138, 0, false}, {22, 1, true}, {22, 1, true}, {22, 2, true}, {22, 3, true},
	{139, 0, false}, {139, 1, false}, {139, 0, false}, {139, 0, false}, {139, 1, false}, {139, 1, false},
	{139, 2, false}, {139, 2, false}, {139, 3, false}, {139, 3, false}, {139, 4, false}, {139, 5, false},
	{139, 6, false}, {139, 7, false}, {139, 7, false}, {139, 8, false}, {139, 9, false}, {139, 10, false},
	// 990
	{139, 11, false}, {139, 12, false}, {139, 13, false}, {139, 13, false}, {139, 12, false}, {139, 11, false},
	{139, 10, false}, {139, 9, false}, {139, 8, false}, {14, 0, false}, {14, 1, false}, {14, 1, false},
	{14, 1, false}, {14, 1, false}, {14, 1, false}, {14, 1, false}, {14, 1, false}, {14, 1, false},
	{14, 1, false}, {14, 1, false}, {14, 1, false}, {14, 1, false}, {14, 1, false}, {14, 1, false},
	{14, 1, false}, {14, 1, false}, {14, 1, false}, {14, 1, false}, {14, 1, false}, {14, 1, false},
	// 1020
	{14, 1, false}, {14, 1, false}, {14, 1, false}, {14, 1, false}, {14, 1, false}, {14, 1, false},
	{14, 1, false}, {14, 1, false}, {14, 1, false}, {14, 1, false}, {14, 1, false}, {14, 1, false},
	{14, 1, false}, {14, 1, false}, {14, 1, false}, {14, 1, false}, {14, 1, false}, {14, 1, false},
	{14, 1, false}, {14, 1, false}, {14, 1, false}, {14, 1, false}, {140, 0, true}, {140, 1, true},
	{140, 2, true}, {140, 3, true}, {140, 4, true}, {140, 5, true}, {140, 6, true}, {141, 0, true},
	// 1050
	{141, 1, true}, {141, 2, true}, {141, 3, true}, {141, 4, true}, {142, 0, false}, {143, 0, false},
	{44, 0, false}, {44, 1, false}, {44, 2, false}, {44, 3, false}, {44, 0, false}, {44, 4, false},
	{44, 5, false}, {44, 5, false}, {44, 6, false}, {44, 7, false}, {44, 8, false}, {44, 9, false},
	{44, 10, false}, {44, 11, false}, {44, 12, false}, {44, 13, false}, {44, 14, false}, {44, 15, false},
	{44, 16, false}, {22, 1, true}, {58, 0, false}, {144, 0, false}, {144, 1, false}, {144, 2, false},
	// 1080
	{144, 3, false}, {144, 4, false}, {144, 5, false}, {144, 6, false}, {144, 7, false}, {17, 0, true},
	{17, 1, false}, {17, 2, false}, {17, 3, false}, {138, 0, false}
	// 1090
};

static const frame_t DEHEXTRA_FRAME = {138, 0, false};

static const SString SPRITE_BY_INDEX[] =
{
	"TROO", "SHTG", "PUNG", "PISG", "PISF", "SHTF", "SHT2",
	"CHGG", "CHGF", "MISG", "MISF", "SAWG", "PLSG", "PLSF",
	"BFGG", "BFGF", "BLUD", "PUFF", "BAL1", "BAL2", "PLSS",
	"PLSE", "MISL", "BFS1", "BFE1", "BFE2", "TFOG", "IFOG",
	"PLAY", "POSS", "SPOS", "VILE", "FIRE", "FATB", "FBXP",
	// 35
	"SKEL", "MANF", "FATT", "CPOS", "SARG", "HEAD", "BAL7",
	"BOSS", "BOS2", "SKUL", "SPID", "BSPI", "APLS", "APBX",
	"CYBR", "PAIN", "SSWV", "KEEN", "BBRN", "BOSF", "ARM1",
	"ARM2", "BAR1", "BEXP", "FCAN", "BON1", "BON2", "BKEY",
	"RKEY", "YKEY", "BSKU", "RSKU", "YSKU", "STIM", "MEDI",
	// 70
	"SOUL", "PINV", "PSTR", "PINS", "MEGA", "SUIT", "PMAP",
	"PVIS", "CLIP", "AMMO", "ROCK", "BROK", "CELL", "CELP",
	"SHEL", "SBOX", "BPAK", "BFUG", "MGUN", "CSAW", "LAUN",
	"PLAS", "SHOT", "SGN2", "COLU", "SMT2", "GOR1", "POL2",
	"POL5", "POL4", "POL3", "POL1", "POL6", "GOR2", "GOR3",
	// 105
	"GOR4", "GOR5", "SMIT", "COL1", "COL2", "COL3", "COL4",
	"CAND", "CBRA", "COL6", "TRE1", "TRE2", "ELEC", "CEYE",
	"FSKU", "COL5", "TBLU", "TGRN", "TRED", "SMBT", "SMGT",
	"SMRT", "HDB1", "HDB2", "HDB3", "HDB4", "HDB5", "HDB6",
	"POB1", "POB2", "BRS1", "TLMP", "TLP2", "TNT1", "DOGS",
	// 140
	"PLS1", "PLS2", "BON3", "BON4"
	// 144
};
}

#endif
