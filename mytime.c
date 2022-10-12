
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

// MARK  => 3Vpp amplitude
// SPACE => 1Vpp amplitude
#define SIGNAL_SPACE 0.3
// This offset allows for the amplitude to have the proper Y (vertical, voltage) Offset
#define OFFSET_SPACE (2048 - (2048 * SIGNAL_SPACE))

#define SIGNAL_MARK 1
#define OFFSET_MARK 0

int array_iterator = 0;

int draw_image(double *xs, double *ys, int size)
{

    _Bool success;

    RGBABitmapImageReference *canvasReference = CreateRGBABitmapImageReference();
    StringReference *errorMessage = (StringReference *)malloc(sizeof(StringReference));
    success = DrawScatterPlot(canvasReference, 1200, 800, xs, size, ys, size, errorMessage);

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
        for (int i = 0; i < errorMessage->stringLength; i++)
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
    // Ideal Sleep time is : 1/256 ms = 3.90625 microseconds = 3906.25 nanoseconds
    // Because : you need 256 different values (1 Period in 1 Millisecond)
    // A bit lower to prevent issues
    uint64_t sleep_period_nsec = 3800;

    for (int i = 0; i < 256; i++)
    {
        // gettimeofday(tv_diff, NULL);
        clock_gettime(CLOCK_REALTIME, tv_diff);
        etime_nsec = ((tv_diff->tv_sec - tv_started->tv_sec) * NANOSECONDS_PER_SECOND) + tv_diff->tv_nsec;
        // Get the real starting value so that the graph starts at 0
        if (array_iterator == 0)
            *timeoffset = etime_nsec;

        // +1 to prevent divide by 0
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

int main()
{
    struct timespec *tv_started;
    struct timespec *tv_diff;
    int arraysize = 512;

    // struct timespec *tv_sleep;

    // // Elapsed Time
    // uint64_t etime_nsec;
    double xs[100000];
    double ys[100000];

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

    send_binary(ZERO, timeoffset, tv_started, tv_diff, xs, ys);
    send_binary(ONE, timeoffset, tv_started, tv_diff, xs, ys);
    send_binary(POS, timeoffset, tv_started, tv_diff, xs, ys);
    // send_binary(ZERO, timeoffset, tv_started, tv_diff, xs, ys);
    // send_signal(SIGNAL_MARK, OFFSET_MARK, timeoffset, tv_started, tv_diff, xs, ys);
    // send_signal(SIGNAL_MARK, OFFSET_MARK, timeoffset, tv_started, tv_diff, xs, ys);

    // send_signal(SIGNAL_MARK, OFFSET_MARK, timeoffset, tv_started, tv_diff, xs, ys);
    // send_signal(SIGNAL_MARK, OFFSET_MARK, timeoffset, tv_started, tv_diff, xs, ys);
    // send_signal(SIGNAL_MARK, OFFSET_MARK, timeoffset, tv_started, tv_diff, xs, ys);
    // send_signal(SIGNAL_MARK, OFFSET_MARK, timeoffset, tv_started, tv_diff, xs, ys);

    // for (int i = 0; i < 256; i++)
    // {
    //     // gettimeofday(tv_diff, NULL);
    //     clock_gettime(CLOCK_REALTIME, tv_diff);
    //     etime_nsec = ((tv_diff->tv_sec - tv_started->tv_sec) * NANOSECONDS_PER_SECOND) + tv_diff->tv_nsec;
    //     // Get the real starting value so that the graph starts at 0
    //     if (i == 0)
    //         timeoffset = etime_nsec;

    //     // +1 to prevent divide by 0
    //     xs[graphtime] = (etime_nsec - timeoffset + 1) / DIV_TO_GRAPH_MS;
    //     ys[graphtime] = DACLookup_FullSine_8Bit[i] * SIGNAL_LOW + OFFSET_LOW;
    //     graphtime++;

    //     // Try for active wait
    //     // int looptime = 0;
    //     // Small value as we get the start point
    //     starttime = etime_nsec - timeoffset;

    //     while (sleepcounter < sleep_period_nsec)
    //     {
    //         // gettimeofday(tv_diff, NULL);
    //         clock_gettime(CLOCK_REALTIME, tv_diff);
    //         etime_nsec = ((tv_diff->tv_sec - tv_started->tv_sec) * NANOSECONDS_PER_SECOND) + tv_diff->tv_nsec;
    //         // printf("sleepcounter %ld = etime_nsec %ld - timeoffset %ld- starttime %ld\n", sleepcounter, etime_nsec, timeoffset, starttime);
    //         sleepcounter = etime_nsec - timeoffset - starttime;
    //         // printf("sleepcounter %ld\n", sleepcounter);
    //         // looptime++;
    //     }

    //     sleepcounter = 0;
    // }

    // sleepcounter = 0;

    // for (int i = 0; i < 256; i++)
    // {
    //     // gettimeofday(tv_diff, NULL);
    //     clock_gettime(CLOCK_REALTIME, tv_diff);
    //     etime_nsec = ((tv_diff->tv_sec - tv_started->tv_sec) * NANOSECONDS_PER_SECOND) + tv_diff->tv_nsec;

    //     xs[graphtime] = (etime_nsec - timeoffset + 1) / DIV_TO_GRAPH_MS;
    //     ys[graphtime] = DACLookup_FullSine_8Bit[i];
    //     graphtime++;

    //     // Small value as we get the start point
    //     starttime = etime_nsec - timeoffset;
    //     // Try for active wait
    //     int looptime = 0;
    //     while (sleepcounter < sleep_period_nsec)
    //     {
    //         // gettimeofday(tv_diff, NULL);
    //         clock_gettime(CLOCK_REALTIME, tv_diff);

    //         etime_nsec = ((tv_diff->tv_sec - tv_started->tv_sec) * NANOSECONDS_PER_SECOND) + tv_diff->tv_nsec;
    //         // printf("sleepcounter %ld = etime_nsec %ld - timeoffset %ld- starttime %ld\n", sleepcounter, etime_nsec, timeoffset, starttime);
    //         sleepcounter = etime_nsec - timeoffset - starttime;
    //         // printf("sleepcounter %ld\n", sleepcounter);
    //         looptime++;
    //     }
    //     // printf("hi2\n");
    //     // printf("looptime %ld\n", looptime);
    //     sleepcounter = 0;
    //     // clock_nanosleep(CLOCK_MONOTONIC, 0, tv_sleep, NULL);
    //     //  nanosleep(tv_sleep, NULL);
    //     //   usleep(usleeptime);
    // }

    draw_image(xs, ys, array_iterator);
}