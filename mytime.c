
#include <stdio.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "pbPlots.h"
#include "supportLib.h"
#include "mytime.h"

#ifdef __arm__
// do something
#endif

#define GRAPH 1

#define NANOSECONDS_PER_SECOND 1000000000
#define DIV_TO_GRAPH_MS 1000

// Ideal Sleep time is : 1/256 ms = 3.90625 microseconds = 3906.25 nanoseconds
// Because : you need 256 different values (1 Period in 1 Millisecond)
// A bit lower to prevent issues
//#define SLEEP_NS 3800;
#define SLEEP_NS 4000

// MCP4725 uses values from 0 to 4095.
// 0    => 0   Volts
// 4095 => 3.3 Volts
// But AFNOR Works with 2.2 volts peak, so we need to multiply by 2/3

// MARK  => 2.17  Vpp amplitude
// SPACE => 0.688 Vpp amplitude
#define SIGNAL_MARK (1.0 * 2 / 3)
#define OFFSET_MARK 0

//#define SIGNAL_SPACE (0.3 * 2 / 3)
#define SIGNAL_SPACE (0.3 * 2 / 3)
// This offset allows for the amplitude to have the proper Y (vertical, voltage) Offset
// Half of the total range (4096 values is 2048)
// We want the difference between the big and the small period to get the offset
#define OFFSET_SPACE ((2048 - (2048 * 0.3)) * 2 / 3)

// Ideal Sleep time is : 1/256 ms = 3.90625 microseconds = 3906.25 nanoseconds
// Because : you need 256 different values (1 Period in 1 Millisecond)
// A bit lower to prevent issues
uint64_t sleep_period_nsec = SLEEP_NS;

int array_iterator = 0;

char timeframe[100] = {0};

int draw_image(double *xs, double *ys, int size)
{

    _Bool success;

    ScatterPlotSeries *series = GetDefaultScatterPlotSeriesSettings();
    series->xs = xs;
    series->xsLength = size;
    series->ys = ys;
    series->ysLength = size;
    series->linearInterpolation = true;
    series->lineType = L"solid";
    series->lineTypeLength = wcslen(series->lineType);
    series->lineThickness = 2;
    series->color = GetGray(0.8);

    ScatterPlotSettings *settings = GetDefaultScatterPlotSettings();
    settings->width = 1200;
    settings->height = 800;
    settings->autoBoundaries = true;
    settings->autoPadding = true;
    settings->title = L"x^2 - 2";
    settings->titleLength = wcslen(settings->title);
    settings->xLabel = L"microseconds";
    settings->xLabelLength = wcslen(settings->xLabel);
    settings->yLabel = L"value";
    settings->yLabelLength = wcslen(settings->yLabel);
    ScatterPlotSeries *s[] = {series};
    settings->scatterPlotSeries = s;
    settings->scatterPlotSeriesLength = 1;

    RGBABitmapImageReference *canvasReference = CreateRGBABitmapImageReference();
    StringReference *errorMessage = (StringReference *)malloc(sizeof(StringReference));
    // success = DrawScatterPlot(canvasReference, 1200, 800, xs, size, ys, size, errorMessage);
    success = DrawScatterPlotFromSettings(canvasReference, settings, errorMessage);

    printf("coucou \n");
    if (success)
    {
        size_t length;
        double *pngdata = ConvertToPNG(&length, canvasReference->image);
        WriteToFile(pngdata, length, "example1.png");
        DeleteImage(canvasReference->image);
    }
    else
    {
        fprintf(stderr, "Error: ");
        for (unsigned int i = 0; i < errorMessage->stringLength; i++)
        {
            fprintf(stderr, "%c", errorMessage->string[i]);
        }
        fprintf(stderr, "\n");
    }
    return success ? 0 : 1;
}

#if GRAPH
void send_signal(float signal_type,
                 int signal_offset,
                 uint64_t *timeoffset,
                 struct timespec *tv_started,
                 struct timespec *tv_diff,
                 double *xs, double *ys)
#else
void send_signal(float signal_type,
                 int signal_offset,
                 uint64_t *timeoffset,
                 struct timespec *tv_started,
                 struct timespec *tv_diff)
#endif
{
    uint64_t sleep_starttime = 0;
    uint64_t etime_nsec = 0;
    uint64_t sleepcounter = 0;

    for (int i = 0; i < 256; i++)
    {
        // gettimeofday(tv_diff, NULL);
        clock_gettime(CLOCK_REALTIME, tv_diff);
        etime_nsec = ((tv_diff->tv_sec - tv_started->tv_sec) * NANOSECONDS_PER_SECOND) + tv_diff->tv_nsec;
        // Get the real starting value so that the graph starts at 0
        if (array_iterator == 0)
            *timeoffset = etime_nsec;

        // +1ns to prevent divide by 0
        xs[array_iterator] = (etime_nsec - *timeoffset + 1) / DIV_TO_GRAPH_MS;
        // ys[array_iterator] = (DACLookup_FullSine_8Bit[i] * signal_type + signal_offset);
        ys[array_iterator] = (DACLookup_FullSine_8Bit[i] * signal_type + signal_offset) / 4095 * 3.3;
        // ys[array_iterator] = (DACLookup_FullSine_8Bit[i] * signal_type) / 3.3 * 2.2;
        array_iterator++;

        sleep_starttime = etime_nsec - *timeoffset;

        while (sleepcounter < sleep_period_nsec)
        {
            clock_gettime(CLOCK_REALTIME, tv_diff);
            etime_nsec = ((tv_diff->tv_sec - tv_started->tv_sec) * NANOSECONDS_PER_SECOND) + tv_diff->tv_nsec;

            sleepcounter = etime_nsec - *timeoffset - sleep_starttime;
            // printf("sleepcounter %ld\n", sleepcounter);
            // looptime++;
        }
        sleepcounter = 0;
    }
}

// Send the necessary MARK / SPACE to make a Binary number
// Binary 0             => 2 MARK, 8 SPACE
// Binary 1             => 5 MARK, 5 SPACE
// Position Identifier  => 8 MARK, 2 SPACE
void send_binary(bit_type bit,
                 uint64_t *timeoffset,
                 struct timespec *tv_started,
                 struct timespec *tv_diff,
                 double *xs, double *ys)
{
    int i = 0;

    if (bit == ZERO)
    {
        for (i = 0; i < 2; i++)
            send_signal(SIGNAL_MARK, OFFSET_MARK, timeoffset, tv_started, tv_diff, xs, ys);
        for (i = 0; i < 8; i++)
            send_signal(SIGNAL_SPACE, OFFSET_SPACE, timeoffset, tv_started, tv_diff, xs, ys);
    }
    if (bit == ONE)
    {
        for (i = 0; i < 5; i++)
            send_signal(SIGNAL_MARK, OFFSET_MARK, timeoffset, tv_started, tv_diff, xs, ys);
        for (i = 0; i < 5; i++)
            send_signal(SIGNAL_SPACE, OFFSET_SPACE, timeoffset, tv_started, tv_diff, xs, ys);
    }
    if (bit == POS)
    {
        for (i = 0; i < 8; i++)
            send_signal(SIGNAL_MARK, OFFSET_MARK, timeoffset, tv_started, tv_diff, xs, ys);
        for (i = 0; i < 2; i++)
            send_signal(SIGNAL_SPACE, OFFSET_SPACE, timeoffset, tv_started, tv_diff, xs, ys);
    }
}

// shifting four places in binary multiplies or divides by 16
// Convert Decimal value to BCD
// 1 2 4 8  10 20 40 80  100 200
unsigned char decToBcd(unsigned char val)
{
    return ((val / 10 * 16) + (val % 10));
}

// Decimal to BCD (Binary Coded Decimal)
// Here the version for 2 bytes
uint32_t decToBcd2(uint32_t val)
{
    uint32_t out = 0;

    // Case for Day of year, number that need to be stored in 10 bit
    if ((val / 100) >= 1)
    {
        out = val / 100;
        out = out << 8;

        val = val % 100;
    }
    out = out + ((val / 10 * 16) + (val % 10));

    return out;
}

unsigned char bcdToDec(unsigned char val)
{
    return ((val / 16 * 10) + (val % 16));
}

// We're going to send test signal, calculate the duration for sending a certain amount
// And we'll adjust the active wait variable, sleep_period_nsec based on that
int autoadjust_sleep(uint64_t *timeoffset,
                     struct timespec *tv_started,
                     struct timespec *tv_diff,
                     double *xs, double *ys)
{
    printf("adjusting timing ...\n");

    float timing;
    float correction;
    uint64_t sleep_period_nsec_corrected = 0;
    uint64_t etime_nsec = 0;

    int i;

    // How many time we want to adjust the timing
    for (int loop = 0; loop < 10; loop++)
    {
        timing = 0;
        etime_nsec = 0;
        // Send 30 periods
        for (i = 0; i < 10; i++)
        {
            array_iterator = 0;
            *timeoffset = 0;

            // Each send command is 10 periods of 1 ms, so 10ms
            send_binary(ZERO, timeoffset, tv_started, tv_diff, xs, ys);
            send_binary(ONE, timeoffset, tv_started, tv_diff, xs, ys);
            send_binary(POS, timeoffset, tv_started, tv_diff, xs, ys);

            // One position back in the array

            etime_nsec += ((tv_diff->tv_sec - tv_started->tv_sec) * NANOSECONDS_PER_SECOND) + tv_diff->tv_nsec - *timeoffset;

            timing += xs[array_iterator - 1] - xs[0];
        }

        // printf("Pass %d average etime_nsec is %lu ...\n", i, etime_nsec / 1000 / i);
        int tmp = etime_nsec / 1000 / i;
        // printf("tmp %lu \n", tmp);
        //  printf("Pass %d average timing is %f ...\n", i, timing / i);

        // 10 iteration, of 3 periods, with 1000 microseconds each
        // correction = (10 * 3 * 1000) / (timing / i);
        correction = (10 * 3 * 1000) / (float)tmp;
        // printf("correction %f \n", correction);

        // For unknown reasons sometimes, on raspberry, the correction has a very strange value
        if (correction > 0.5)
            sleep_period_nsec_corrected = (correction * sleep_period_nsec);

        // printf("Correction to apply %0.3f, sleep_period_nsec was : %llu, will be : %llu ...\n", correction, sleep_period_nsec, sleep_period_nsec_corrected);
        sleep_period_nsec = sleep_period_nsec_corrected;

        array_iterator = 0;
    }
    printf("Sleep_period_nsec was %d :, will be : %llu ...\n", SLEEP_NS, sleep_period_nsec);

    // array_iterator = 0;
    return 0;
}

// Configuration Position Identifier, every 10 bits
// Configure static bits
void set_pos_ident()
{
    timeframe[0] = POS;

    for (int i = 0; i < 100; i++)
    {
        if (i % 10 == 9)
            timeframe[i] = POS;
    }
}

void set_timeframe(int value, time_type type, pos_type pos_timeframe)
{
    int value_bcd = decToBcd2(value);

    printf("value : %d, bcd: %d\n", value, value_bcd);

    // Read bit per bit the value
    for (unsigned int i = 0; i < type; i++)
    {

        // If we're at the 4th bit or 8th bit, we jump the next value in array to keep it at 0 or POS
        // https://en.wikipedia.org/w/index.php?title=IRIG_timecode#IRIG_timecode
        if ((i != 0) && (i % 4 == 0))
            pos_timeframe++;

        timeframe[pos_timeframe] = (value_bcd >> i) & 1;
        pos_timeframe++;

        printf("%d", (value_bcd >> i) & 1);
    }
    printf("\n");
}

void set_sbs(uint16_t sbs_value)
{
    int pos_timeframe = 80;

    printf("sbs value : %d\n", sbs_value);

    // Read bit per bit the value
    for (unsigned int i = 0; i < 16; i++)
    {
        // If we're at the 9th bit, we jump the next value in array to keep it at POS P9
        // https://en.wikipedia.org/w/index.php?title=IRIG_timecode#IRIG_timecode
        if ((i == 9))
            pos_timeframe++;

        timeframe[pos_timeframe] = (sbs_value >> i) & 1;
        pos_timeframe++;

        printf("%d", (sbs_value >> i) & 1);
    }
    printf("\n");
}

void generate_data(struct timespec *tv_started)
{
    // https://stackoverflow.com/questions/19377396/c-get-day-of-year-from-date
    time_t rawtime = tv_started->tv_sec;
    struct tm ts;
    char buf[80];

    // Format time, "ddd yyyy-mm-dd hh:mm:ss zzz"
    ts = *localtime(&rawtime);
    strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", &ts);
    printf("%s\n", buf);

    // int s = tv_started->tv_sec % 60;
    // int m = (tv_started->tv_sec / 60) % 60;
    // int h = (tv_started->tv_sec / 3600) % 24;

    set_pos_ident();

    set_timeframe(ts.tm_sec, SEC, POS_SEC);
    set_timeframe(ts.tm_min, MIN, POS_MIN);
    set_timeframe(ts.tm_hour, HOUR, POS_HOUR);
    set_timeframe(ts.tm_yday, YDAY, POS_YDAY);
    set_timeframe(ts.tm_wday, WDAY, POS_WDAY);
    // % 100 as tm_year is number of years since 1900
    set_timeframe(ts.tm_year % 100, YEAR, POS_YEAR);
    // tm_mon counts from 0
    set_timeframe(ts.tm_mon + 1, MONTH, POS_MONTH);
    set_timeframe(ts.tm_mday, MDAY, POS_MDAY);

    set_sbs(ts.tm_sec + (ts.tm_min * 60) + (ts.tm_hour * 60 * 60));

    // set_timeframe_mn(ts.tm_min);
    // set_timeframe_h(ts.tm_hour);
    // set_timeframe_doy(ts.tm_yday);
}

int main()
{
    struct timespec *tv_started;
    struct timespec *tv_diff;

    // struct timespec *tv_sleep;

    // // Elapsed Time
    // uint64_t etime_nsec;
    double xs[100000];
    double ys[100000];

    // 60
    // number      : 0011 1100 0011 1100
    // number  bcd : 0000 0000 0110 0000

    // 160
    // number      : 1010 0000 1010 0000
    // number  bcd : 0000 0001 0000 0000

    uint32_t number = 354;
    uint32_t numberp1 = number & 0xFF00;
    uint32_t numberp2 = number >> 8;

    printf("number Leading text " BYTE_TO_BINARY_PATTERN "\n", BYTE_TO_BINARY(number));

    printf("numberp1 Leading text " BYTE_TO_BINARY_PATTERN "\n", BYTE_TO_BINARY(numberp1));
    printf("numberp2 Leading text " BYTE_TO_BINARY_PATTERN "\n", BYTE_TO_BINARY(numberp2));

    printf("short DEC to BCD %d\n", decToBcd(number) >> 8);
    printf("short DEC to BCD %d\n", decToBcd(number));

    printf("long  DEC to BCD %d\n", decToBcd2(number));

    printf("short Leading text " BYTE_TO_BINARY_PATTERN "\n", BYTE_TO_BINARY(decToBcd(number)));
    printf("long  Leading text " BYTE_TO_BINARY_PATTERN "\n", BYTE_TO_BINARY(decToBcd2(number)));

    printf("number      : " BYTE_TO_BINARY_PATTERN " " BYTE_TO_BINARY_PATTERN "\n",
           BYTE_TO_BINARY(number), BYTE_TO_BINARY(number));
    printf("number  bcd : " BYTE_TO_BINARY_PATTERN " " BYTE_TO_BINARY_PATTERN "\n",
           BYTE_TO_BINARY(decToBcd2(number) >> 8), BYTE_TO_BINARY(decToBcd2(number)));

    uint64_t *timeoffset;
    timeoffset = malloc(sizeof(*timeoffset));

    tv_diff = malloc(sizeof(*tv_diff));
    tv_started = malloc(sizeof(*tv_started));

    clock_gettime(CLOCK_REALTIME, tv_started);

    autoadjust_sleep(timeoffset, tv_started, tv_diff, xs, ys);

    clock_gettime(CLOCK_REALTIME, tv_started);

    generate_data(tv_started);

    // send_data(ts.tm_sec, 8, timeoffset, tv_started, tv_diff, xs, ys)

    send_binary(ZERO, timeoffset, tv_started, tv_diff, xs, ys);
    send_binary(ONE, timeoffset, tv_started, tv_diff, xs, ys);
    // send_binary(POS, timeoffset, tv_started, tv_diff, xs, ys);
    // send_binary(ZERO, timeoffset, tv_started, tv_diff, xs, ys);
    // send_binary(ONE, timeoffset, tv_started, tv_diff, xs, ys);
    // send_binary(POS, timeoffset, tv_started, tv_diff, xs, ys);

// NE PAS OUBLIER LE BAUD RATE A 400 000
#ifdef __arm__
    // do something
    int piHiPri(int priority);
#endif

    draw_image(xs, ys, array_iterator);
}