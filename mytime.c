
#include <stdio.h>
#include <sys/time.h>
#include <stdint.h>
#include <unistd.h>
#include "pbPlots.h"
#include "supportLib.h"
#include "mytime.h"

#define NANOSECONDS_PER_SECOND 1000000000
#define DIV_TO_GRAPH_MS 1000
#define DIV_TO_GRAPH_S 1000

// MARK  => 3Vpp
// SPACE => 1Vpp

#define SIGNAL_SPACE 0.3
#define OFFSET_SPACE (2048 - (2048 * SIGNAL_SPACE))

#define SIGNAL_MARK 1
#define OFFSET_MARK 0


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


void send_signal(int signal_type, int offset)
{

}


int main()
{
    struct timespec *tv_diff;
    struct timespec *tv_started;

    struct timespec *tv_sleep;

    // Elapsed Time
    uint64_t etime_nsec;
    const int arraysize = 512;
    double xs[512];
    double ys[512];

    struct timespec tps, tpe;
    if ((clock_gettime(CLOCK_REALTIME, &tps) != 0) || (clock_gettime(CLOCK_REALTIME, &tpe) != 0))
    {
        perror("clock_gettime");
        return -1;
    }
    printf("%lu s, %lu ns\n", tpe.tv_sec - tps.tv_sec,
           tpe.tv_nsec - tps.tv_nsec);

    // double *xs;
    // double *ys;
    // xs = malloc(sizeof(double) * arraysize);
    // ys = malloc(sizeof(double) * arraysize);

    uint64_t timeoffset;

    tv_diff = malloc(sizeof(*tv_diff));
    tv_started = malloc(sizeof(*tv_started));


    clock_gettime(CLOCK_REALTIME, tv_started);

    int graphtime = 0;

    uint64_t starttime = 0;

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
        if (i == 0)
            timeoffset = etime_nsec;

        // +1 to prevent divide by 0
        xs[graphtime] = (etime_nsec - timeoffset + 1) / DIV_TO_GRAPH_MS;
        ys[graphtime] = DACLookup_FullSine_8Bit[i] * SIGNAL_LOW + OFFSET_LOW;
        graphtime++;

        // Try for active wait
        // int looptime = 0;
        // Small value as we get the start point
        starttime = etime_nsec - timeoffset;

        while (sleepcounter < sleep_period_nsec)
        {
            // gettimeofday(tv_diff, NULL);
            clock_gettime(CLOCK_REALTIME, tv_diff);
            etime_nsec = ((tv_diff->tv_sec - tv_started->tv_sec) * NANOSECONDS_PER_SECOND) + tv_diff->tv_nsec;
            // printf("sleepcounter %ld = etime_nsec %ld - timeoffset %ld- starttime %ld\n", sleepcounter, etime_nsec, timeoffset, starttime);
            sleepcounter = etime_nsec - timeoffset - starttime;
            // printf("sleepcounter %ld\n", sleepcounter);
            // looptime++;
        }

        sleepcounter = 0;
    }

    sleepcounter = 0;

    for (int i = 0; i < 256; i++)
    {
        // gettimeofday(tv_diff, NULL);
        clock_gettime(CLOCK_REALTIME, tv_diff);
        etime_nsec = ((tv_diff->tv_sec - tv_started->tv_sec) * NANOSECONDS_PER_SECOND) + tv_diff->tv_nsec;

        xs[graphtime] = (etime_nsec - timeoffset + 1) / DIV_TO_GRAPH_MS;
        ys[graphtime] = DACLookup_FullSine_8Bit[i];
        graphtime++;

        // Small value as we get the start point
        starttime = etime_nsec - timeoffset;
        // Try for active wait
        int looptime = 0;
        while (sleepcounter < sleep_period_nsec)
        {
            // gettimeofday(tv_diff, NULL);
            clock_gettime(CLOCK_REALTIME, tv_diff);

            etime_nsec = ((tv_diff->tv_sec - tv_started->tv_sec) * NANOSECONDS_PER_SECOND) + tv_diff->tv_nsec;
            // printf("sleepcounter %ld = etime_nsec %ld - timeoffset %ld- starttime %ld\n", sleepcounter, etime_nsec, timeoffset, starttime);
            sleepcounter = etime_nsec - timeoffset - starttime;
            // printf("sleepcounter %ld\n", sleepcounter);
            looptime++;
        }
        // printf("hi2\n");
        // printf("looptime %ld\n", looptime);
        sleepcounter = 0;
        // clock_nanosleep(CLOCK_MONOTONIC, 0, tv_sleep, NULL);
        //  nanosleep(tv_sleep, NULL);
        //   usleep(usleeptime);
    }

    draw_image(xs, ys, arraysize);
}