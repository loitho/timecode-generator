#include "stdio.h"
#include "pbPlots.h"
#include "supportLib.h"
#include "time.h"
#include <sys/time.h>
#include <unistd.h>

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
    unsigned long timemillisec;
    int arraysize = 512;
    // double xs[] = {-2, -1, 0, 1, 2};
    // double ys[] = {2, -1, -2, -1, 2};
    double xs[arraysize];
    double ys[arraysize];
    int timeoffset;

    tv_diff = malloc(sizeof(*tv_diff));
    tv_started = malloc(sizeof(*tv_started));

    gettimeofday(tv_started, NULL);

    int graphtime = 0;

    for (int i = 0; i < 256; i++)
    {
        gettimeofday(tv_diff, NULL);
        // Get the real starting value so that the graph starts at 0
        if (i == 0)
            timeoffset = tv_diff->tv_sec;

        timemillisec = ((tv_diff->tv_sec - tv_started->tv_sec) * 1000) + (tv_diff->tv_usec / 1000);
        //timemillisec -= timeoffset;

        xs[graphtime++] = timemillisec;
        ys[i] = DACLookup_FullSine_8Bit[i];

        // timemillisec = (tv_diff->tv_usec / 1000);
        // printf("time sec : %d\t time usec  : %d\n", tv_diff->tv_sec, tv_diff->tv_usec);
        // printf("time milli : %lu \t time usec  : %lu\n", timemillisec, tv_diff->tv_usec);
        // printf("time milli : %lu \t value  : %d\n", timemillisec, DACLookup_FullSine_8Bit[i]);
        // printf("time sec : %lu \n", timemillisec);

        // 1ms
        usleep(1000);
    }

    for (int i = 0; i < 256; i++)
    {
        gettimeofday(tv_diff, NULL);
        timemillisec = ((tv_diff->tv_sec - tv_started->tv_sec) * 1000) + (tv_diff->tv_usec / 1000);
        //timemillisec -= timeoffset;

        xs[graphtime] = timemillisec;
        ys[graphtime] = DACLookup_FullSine_8Bit[i];
        graphtime++;

        // 1ms
        usleep(1000);
    }

    draw_image(xs, ys, arraysize);
}