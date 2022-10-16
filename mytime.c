
#include <stdio.h>
#include <sys/time.h>
#include <stdint.h>
#include <unistd.h>
#include "pbPlots.h"
#include "supportLib.h"
#include "mytime.h"

#define GRAPH 1

#define NANOSECONDS_PER_SECOND 1000000000
#define DIV_TO_GRAPH_MS 1000

// Ideal Sleep time is : 1/256 ms = 3.90625 microseconds = 3906.25 nanoseconds
// Because : you need 256 different values (1 Period in 1 Millisecond)
// A bit lower to prevent issues
//#define SLEEP_NS 3800;
#define SLEEP_NS 4000;

// MARK  => 3Vpp amplitude
// SPACE => 1Vpp amplitude
#define SIGNAL_SPACE 0.3
// This offset allows for the amplitude to have the proper Y (vertical, voltage) Offset
#define OFFSET_SPACE (2048 - (2048 * SIGNAL_SPACE))

#define SIGNAL_MARK 1
#define OFFSET_MARK 0

// Ideal Sleep time is : 1/256 ms = 3.90625 microseconds = 3906.25 nanoseconds
// Because : you need 256 different values (1 Period in 1 Millisecond)
// A bit lower to prevent issues
uint64_t sleep_period_nsec = SLEEP_NS;

int array_iterator = 0;

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
    settings->xLabel = L"X axis";
    settings->xLabelLength = wcslen(settings->xLabel);
    settings->yLabel = L"microseconds";
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
        ys[array_iterator] = DACLookup_FullSine_8Bit[i] * signal_type + signal_offset;
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
    uint64_t sleep_period_nsec_corrected;

    int i;

    // How many time we want to adjust the timing
    for (int loop = 0; loop < 10; loop++)
    {
        timing = 0;
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
            timing += xs[array_iterator - 1] - xs[0];
        }

        printf("Pass %d average timing is %f ...\n", i, timing / i);

        // 10 iteration, of 3 periods, with 1000 microseconds
        correction = (10 * 3 * 1000) / (timing / i);
        sleep_period_nsec_corrected = (correction * sleep_period_nsec);

        printf("Correction to apply %0.3f, sleep_period_nsec was : %llu, will be : %llu ...\n", correction, sleep_period_nsec, sleep_period_nsec_corrected);
        sleep_period_nsec = sleep_period_nsec_corrected;

        array_iterator = 0;
    }

    // array_iterator = 0;
    return 0;
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

    uint32_t number = 366;
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

    // struct timespec tps, tpe;
    // if ((clock_gettime(CLOCK_REALTIME, &tps) != 0) || (clock_gettime(CLOCK_REALTIME, &tpe) != 0))
    // {
    //     perror("clock_gettime");
    //     return -1;
    // }
    // printf("%lu s, %lu ns\n", tpe.tv_sec - tps.tv_sec,
    //        tpe.tv_nsec - tps.tv_nsec);

    // double *xs;
    // double *ys;
    // xs = malloc(sizeof(double) * arraysize);
    // ys = malloc(sizeof(double) * arraysize);

    uint64_t *timeoffset;
    timeoffset = malloc(sizeof(*timeoffset));

    tv_diff = malloc(sizeof(*tv_diff));
    tv_started = malloc(sizeof(*tv_started));

    clock_gettime(CLOCK_REALTIME, tv_started);

    // int graphtime = 0;

    // uint64_t starttime = 0;

    // uint64_t sleepcounter = 0;
    // // Ideal Sleep time is : 1/256 ms = 3.90625 microseconds = 3906.25 nanoseconds
    // // Because : you need 256 different values (1 Period in 1 Millisecond)
    // // A bit lower to prevent issues
    // uint64_t sleep_period_nsec = 3800;

    autoadjust_sleep(timeoffset, tv_started, tv_diff, xs, ys);

    clock_gettime(CLOCK_REALTIME, tv_started);

    send_binary(ZERO, timeoffset, tv_started, tv_diff, xs, ys);
    send_binary(ONE, timeoffset, tv_started, tv_diff, xs, ys);
    send_binary(POS, timeoffset, tv_started, tv_diff, xs, ys);

    draw_image(xs, ys, array_iterator);
}