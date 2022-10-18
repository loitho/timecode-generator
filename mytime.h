// Redeclare to prevent issues with nanosleep undeclared
#include <time.h>
int nanosleep(const struct timespec *req, struct timespec *rem);
int clock_nanosleep(clockid_t clock_id, int flags, const struct timespec *rqtp, struct timespec *rmtp);

// For use in printf, Byte :
// printf("Leading text "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(byte));
// MultiByte
// printf("m: "BYTE_TO_BINARY_PATTERN" "BYTE_TO_BINARY_PATTERN"\n",
//   BYTE_TO_BINARY(m>>8), BYTE_TO_BINARY(m));
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c %c%c%c%c"
#define BYTE_TO_BINARY(byte)     \
  (byte & 0x80 ? '1' : '0'),     \
      (byte & 0x40 ? '1' : '0'), \
      (byte & 0x20 ? '1' : '0'), \
      (byte & 0x10 ? '1' : '0'), \
      (byte & 0x08 ? '1' : '0'), \
      (byte & 0x04 ? '1' : '0'), \
      (byte & 0x02 ? '1' : '0'), \
      (byte & 0x01 ? '1' : '0')

typedef enum
{
  ZERO,
  ONE,
  POS
} bit_type;

// Number of bits to put in the Array
typedef enum
{
  SEC = 7,
  MIN = 8,
  HOUR = 6,
  YDAY = 10,
  WDAY = 3,
  YEAR = 8,
  MONTH = 5,
  MDAY = 6
} time_type;

// Pos
typedef enum
{
  POS_SEC = 1,
  POS_MIN = 10,
  POS_HOUR = 20,
  POS_YDAY = 30,
  POS_WDAY = 44,
  POS_YEAR = 50,
  POS_MONTH = 60,
  POS_MDAY = 70
} pos_type;

// https://github.com/adafruit/Adafruit_MCP4725/blob/master/examples/sinewave/sinewave.ino
const int DACLookup_FullSine_8Bit[256] =
    {
        2048, 2098, 2148, 2198, 2248, 2298, 2348, 2398,
        2447, 2496, 2545, 2594, 2642, 2690, 2737, 2784,
        2831, 2877, 2923, 2968, 3013, 3057, 3100, 3143,
        3185, 3226, 3267, 3307, 3346, 3385, 3423, 3459,
        3495, 3530, 3565, 3598, 3630, 3662, 3692, 3722,
        3750, 3777, 3804, 3829, 3853, 3876, 3898, 3919,
        3939, 3958, 3975, 3992, 4007, 4021, 4034, 4045,
        4056, 4065, 4073, 4080, 4085, 4089, 4093, 4094,
        4095, 4094, 4093, 4089, 4085, 4080, 4073, 4065,
        4056, 4045, 4034, 4021, 4007, 3992, 3975, 3958,
        3939, 3919, 3898, 3876, 3853, 3829, 3804, 3777,
        3750, 3722, 3692, 3662, 3630, 3598, 3565, 3530,
        3495, 3459, 3423, 3385, 3346, 3307, 3267, 3226,
        3185, 3143, 3100, 3057, 3013, 2968, 2923, 2877,
        2831, 2784, 2737, 2690, 2642, 2594, 2545, 2496,
        2447, 2398, 2348, 2298, 2248, 2198, 2148, 2098,
        2048, 1997, 1947, 1897, 1847, 1797, 1747, 1697,
        1648, 1599, 1550, 1501, 1453, 1405, 1358, 1311,
        1264, 1218, 1172, 1127, 1082, 1038, 995, 952,
        910, 869, 828, 788, 749, 710, 672, 636,
        600, 565, 530, 497, 465, 433, 403, 373,
        345, 318, 291, 266, 242, 219, 197, 176,
        156, 137, 120, 103, 88, 74, 61, 50,
        39, 30, 22, 15, 10, 6, 2, 1,
        0, 1, 2, 6, 10, 15, 22, 30,
        39, 50, 61, 74, 88, 103, 120, 137,
        156, 176, 197, 219, 242, 266, 291, 318,
        345, 373, 403, 433, 465, 497, 530, 565,
        600, 636, 672, 710, 749, 788, 828, 869,
        910, 952, 995, 1038, 1082, 1127, 1172, 1218,
        1264, 1311, 1358, 1405, 1453, 1501, 1550, 1599,
        1648, 1697, 1747, 1797, 1847, 1897, 1947, 1997};


const int DACLookup_FullSine_6Bit[64] =
{
  2048, 2248, 2447, 2642, 2831, 3013, 3185, 3346,
  3495, 3630, 3750, 3853, 3939, 4007, 4056, 4085,
  4095, 4085, 4056, 4007, 3939, 3853, 3750, 3630,
  3495, 3346, 3185, 3013, 2831, 2642, 2447, 2248,
  2048, 1847, 1648, 1453, 1264, 1082,  910,  749,
   600,  465,  345,  242,  156,   88,   39,   10,
     0,   10,   39,   88,  156,  242,  345,  465,
   600,  749,  910, 1082, 1264, 1453, 1648, 1847
};

const int DACLookup_FullSine_5Bit[32] =
{
  2048, 2447, 2831, 3185, 3495, 3750, 3939, 4056,
  4095, 4056, 3939, 3750, 3495, 3185, 2831, 2447,
  2048, 1648, 1264,  910,  600,  345,  156,   39,
     0,   39,  156,  345,  600,  910, 1264, 1648
};