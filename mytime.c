
#include <stdio.h>
#include <sys/time.h>
#include <stdint.h>
#include <unistd.h>
#include "pbPlots.h"
#include "supportLib.h"
#include "mytime.h"

#define NANOSECONDS_PER_SECOND   1000000000

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

int main()
{
    struct timeval *tv_diff;
    struct timeval *tv_started;

    struct timespec *tv_sleep;
    
    uint64_t test;

    
    unsigned long etime_usec;
    const int arraysize = 512;
    double xs[512];
    double ys[512];


  struct timespec tps, tpe;
  if ((clock_gettime(CLOCK_REALTIME, &tps) != 0)
  || (clock_gettime(CLOCK_REALTIME, &tpe) != 0)) {
    perror("clock_gettime");
    return -1;
  }
  printf("%lu s, %lu ns\n", tpe.tv_sec-tps.tv_sec,
    tpe.tv_nsec-tps.tv_nsec);


    // double *xs;
    // double *ys;
    // xs = malloc(sizeof(double) * arraysize);
    // ys = malloc(sizeof(double) * arraysize);

    int timeoffset;

    // Ideal Sleep time is : 1/256 ms = 3.90625 microseconds
    // Because : you need 256 different values (1 Period in 1 Millisecond)
    int usleeptime = 0;
    // int usleeptime = 1000;

    tv_diff = malloc(sizeof(*tv_diff));
    tv_started = malloc(sizeof(*tv_started));
    tv_sleep = malloc(sizeof(time_t) + sizeof(long));

    tv_sleep->tv_sec = 0;
    tv_sleep->tv_nsec = 4000;

    // clock_nanosleep(CLOCK_MONOTONIC, 0, tv_sleep, NULL);
    // nanosleep(tv_sleep, NULL);

    gettimeofday(tv_started, NULL);

    int graphtime = 0;

    int sleepcounter = 0;
    int starttime = 0;
    // Microseconds
    int sleepcounter_period = 4;

    for (int i = 0; i < 256; i++)
    {
        gettimeofday(tv_diff, NULL);

        etime_usec = ((tv_diff->tv_sec - tv_started->tv_sec) * 1000000) + tv_diff->tv_usec;
        // Get the real starting value so that the graph starts at 0
        if (i == 0)
            timeoffset = etime_usec;

        xs[graphtime] = etime_usec - timeoffset;
        ys[i] = DACLookup_FullSine_8Bit[i];
        graphtime++;

        // etime_usec = (tv_diff->tv_usec / 1000);
        // printf("time sec : %d\t time usec  : %d\n", tv_diff->tv_sec, tv_diff->tv_usec);
        // printf("time milli : %lu \t time usec  : %lu\n", etime_usec - timeoffset, tv_diff->tv_usec);
        // printf("time milli : %lu \t value  : %d\n", etime_usec, DACLookup_FullSine_8Bit[i]);
        // printf("time sec : %lu \n", etime_usec);
        // printf("hi\n");

        // Small value as we get the start point
        starttime = etime_usec - timeoffset;
        // Try for active wait
        int looptime = 0;
        while (sleepcounter < sleepcounter_period)
        {
            gettimeofday(tv_diff, NULL);
            etime_usec = ((tv_diff->tv_sec - tv_started->tv_sec) * 1000000) + tv_diff->tv_usec;
            // printf("sleepcounter %ld = etime_usec %ld - timeoffset %ld- starttime %ld\n", sleepcounter, etime_usec, timeoffset, starttime);
            sleepcounter = etime_usec - timeoffset - starttime;
            // printf("sleepcounter %ld\n", sleepcounter);
            looptime++;
        }
        // printf("hi2\n");
        //printf("looptime %ld\n", looptime);
        sleepcounter = 0;
        // clock_nanosleep(CLOCK_MONOTONIC, 0, tv_sleep, NULL);
        //  nanosleep(tv_sleep, NULL);
        //   usleep(usleeptime);
    }

    sleepcounter = 0;
    for (int i = 0; i < 256; i++)
    {
        gettimeofday(tv_diff, NULL);
        etime_usec = ((tv_diff->tv_sec - tv_started->tv_sec) * 1000000) + tv_diff->tv_usec;

        xs[graphtime] = etime_usec - timeoffset;
        ys[graphtime] = DACLookup_FullSine_8Bit[i];
        graphtime++;

        // Small value as we get the start point
        starttime = etime_usec - timeoffset;
        // Try for active wait
        int looptime = 0;
        while (sleepcounter < sleepcounter_period)
        {
            gettimeofday(tv_diff, NULL);
            etime_usec = ((tv_diff->tv_sec - tv_started->tv_sec) * 1000000) + tv_diff->tv_usec;
            // printf("sleepcounter %ld = etime_usec %ld - timeoffset %ld- starttime %ld\n", sleepcounter, etime_usec, timeoffset, starttime);
            sleepcounter = etime_usec - timeoffset - starttime;
            // printf("sleepcounter %ld\n", sleepcounter);
            looptime++;
        }
        // printf("hi2\n");
        //printf("looptime %ld\n", looptime);
        sleepcounter = 0;
        // clock_nanosleep(CLOCK_MONOTONIC, 0, tv_sleep, NULL);
        //  nanosleep(tv_sleep, NULL);
        //   usleep(usleeptime);
    }

    draw_image(xs, ys, arraysize);
}