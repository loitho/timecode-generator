
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include "pbPlots.h"
#include "supportLib.h"
#include "time.h"


int draw_image(double *xs, double *ys, int size)
{

    _Bool success;

    RGBABitmapImageReference *canvasReference = CreateRGBABitmapImageReference();
    StringReference *errorMessage = (StringReference *)malloc(sizeof(StringReference));
    success = DrawScatterPlot(canvasReference, 1200, 800, xs, size, ys, size, errorMessage);

    printf("coucou");
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
    unsigned long etime_usec;
    const int arraysize = 512;
    double xs[512];
    double ys[512];

    // double *xs;
    // double *ys;
    // xs = malloc(sizeof(double) * arraysize);
    // ys = malloc(sizeof(double) * arraysize);

    int timeoffset;
    useconds_t usleeptime = 500;

    tv_diff = malloc(sizeof(*tv_diff));
    tv_started = malloc(sizeof(*tv_started));

    
    gettimeofday(tv_started, NULL);

    int graphtime = 0;

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
        //printf("time milli : %lu \t time usec  : %lu\n", etime_usec - timeoffset, tv_diff->tv_usec);
        // printf("time milli : %lu \t value  : %d\n", etime_usec, DACLookup_FullSine_8Bit[i]);
        // printf("time sec : %lu \n", etime_usec);

        // 1ms
        usleep(usleeptime);
    }

    for (int i = 0; i < 256; i++)
    {
        gettimeofday(tv_diff, NULL);
        etime_usec = ((tv_diff->tv_sec - tv_started->tv_sec) * 1000000) + tv_diff->tv_usec;

        xs[graphtime] = etime_usec - timeoffset;
        ys[graphtime] = DACLookup_FullSine_8Bit[i];
        graphtime++;

        // 1ms
        usleep(usleeptime);
    }

    draw_image(xs, ys, arraysize);
}