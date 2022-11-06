
#include <stdio.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "pbPlots.h"
#include "supportLib.h"
#include "mytime.h"

#ifdef __arm__
#include <stdio.h>
#include <errno.h>
#include <pigpio.h>
#endif

// 40 MHz
#define SPI_SPEED 40000000

#define GRAPH 1
#define ARRAY_SIZE 200000

#define NANOSECONDS_PER_SECOND 1000000000
#define DIV_TO_GRAPH_MS 1000

// Ideal Sleep time is : 1/256 ms = 3.90625 microseconds = 3906.25 nanoseconds
// Because : you need 256 different values (1 Period in 1 Millisecond)
// A bit lower to prevent issues
//#define SLEEP_NS 3800;
// 7000 / 6932 seems OK for Raspy
#define SLEEP_NS 4000

#define SLEEP_ADJUST 1
#define SLEEP_ADJUST_CYCLE 20

// MCP4725 uses values from 0 to 4095.
// 0    => 0   Volts
// 4095 => 3.3 Volts
// But AFNOR Works with 2.2 volts peak, so we need to multiply by 2/3

// Niveau d’entrée nominale : 2.2Vcc.
// Impédance d’entrée : 3.5 Kohms.
// Tension minimale d’entrée : 70 mVcc.

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

// Global handle for SPI communication
int spi;

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

void send_signal(float signal_type,
                 int signal_offset,
                 uint64_t *timeoffset,
                 struct timespec *tv_started,
                 struct timespec *tv_diff,
                 double *xs, double *ys)
{
    uint64_t sleep_starttime = 0;
    uint64_t etime_nsec = 0;
    uint64_t sleepcounter = 0;
    // Min 0, Max 4095
    int dac_value = 0;

    char buf[2];

    for (int i = 0; i < 256; i++)
    {
        // gettimeofday(tv_diff, NULL);
        clock_gettime(CLOCK_REALTIME, tv_diff);
        etime_nsec = ((tv_diff->tv_sec - tv_started->tv_sec) * NANOSECONDS_PER_SECOND) + tv_diff->tv_nsec;
        // Get the real starting value so that the graph starts at 0
        if (array_iterator == 0)
            *timeoffset = etime_nsec;

        dac_value = (DACLookup_FullSine_8Bit[i] * signal_type + signal_offset);

#if GRAPH
        // +1ns to prevent divide by 0
        xs[array_iterator] = (etime_nsec - *timeoffset + 1) / DIV_TO_GRAPH_MS;

        // Graph with a +3.3V Max display
        ys[array_iterator] = dac_value / 4095.0 * 3.3;

        // Graph the DAC value sent to the chip
        // ys[array_iterator] = dac_value;
#endif

#ifdef __arm__
        // I2C
        // if (wiringPiI2CWriteReg8(i2c_fd, (dac_value >> 8) & 0xFF, dac_value & 0xFF) == -1)
        //     printf("error I2C: %s\n", strerror(errno));

        // SPI
        buf[0] = 0x30 + (uint8_t)(dac_value >> 8);
        buf[1] = 0x00 + (dac_value & 0xff);
        if (spiWrite(spi, &buf[0], 2) != 2)
            printf("SPI Communication ERROR\n");

            // SpiWriteAndRead(spi, &buf[0], &buf[0], 2, false); // Transfer buffer data to SPI call
            // SpiWriteBlockRepeat (spi, &buf[0], 2, 1, true);
#endif

        // printf("i2c value : %d, real value: %lf\n", dac_value, (DACLookup_FullSine_8Bit[i] * signal_type + signal_offset));
        //  ys[array_iterator] = (DACLookup_FullSine_8Bit[i] * signal_type) / 3.3 * 2.2;
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
        // usleep(100000);
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

// We're going to send test signal, calculate the duration for sending a certain amount
// And we'll adjust the active wait variable, sleep_period_nsec based on that
int autoadjust_sleep(uint64_t *timeoffset,
                     struct timespec *tv_started,
                     struct timespec *tv_diff,
                     double *xs, double *ys)
{
    printf("adjusting timing ...\n");

    // float timing;
    float correction;
    uint64_t sleep_period_nsec_corrected = 0;
    uint64_t etime_nsec = 0;

    int i;

    // How many time we want to adjust the timing
    for (int loop = 0; loop < SLEEP_ADJUST_CYCLE; loop++)
    {
        // timing = 0;
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

            etime_nsec += ((tv_diff->tv_sec - tv_started->tv_sec) * NANOSECONDS_PER_SECOND) + tv_diff->tv_nsec - *timeoffset;

            // One position back in the array
            // timing += xs[array_iterator - 1] - xs[0];
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

    // Position All default values
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
}

void send_data(uint64_t *timeoffset,
               struct timespec *tv_started,
               struct timespec *tv_diff,
               double *xs, double *ys)
{
    int loop_send = 0;

    for (loop_send = 0; loop_send < 100; loop_send++)
        send_binary(timeframe[loop_send], timeoffset, tv_started, tv_diff, xs, ys);
}

int main(int argc, char *argv[])
{
    int arg1 = 0;
    if (argc == 2)
    {
        printf("Selecting sleep period from CLI");
        sleep_period_nsec = atoi(argv[1]);
        arg1 = atoi(argv[1]); // argv[0] is the program name
    }                         // otherwise continue on our merry way....

    // atoi = ascii to int
    struct timespec *tv_started;
    struct timespec *tv_diff;

    double xs[ARRAY_SIZE];
    double ys[ARRAY_SIZE];

#ifdef __arm__

    printf("ARM Only\n");

    if (gpioInitialise() < 0)
    {
        // pigpio initialisation failed.
        printf("pigpio failed initialisation\n");

        exit(1);
    }
    else
    {
        printf("pigpio initialized\n");
    }

    // 5 MHz
    // spi = SpiOpenPort(0, 8, SPI_SPEED, 0, false);
    spi = spiOpen(0, SPI_SPEED, 0);
    uint16_t data = arg1 % 4096;

    if (spi >= 0)
    {
        // int data = 4095;
        //  uint8_t bufdata = 0;
        //  MIN: echo -ne "\x30\x00" > /dev/spidev0.0
        //  MAX: echo -ne "\x3F\xFF" > /dev/spidev0.0
        printf("SPI initiated\n");

        char buf[2];
        int ret;

        buf[0] = 0x30 + (char)(data >> 8);
        buf[1] = 0x00 + (data & 0xff);
        printf(" 1er : %d, 2nd : %d\n", buf[0], buf[1]);
        printf(" 1er : %04X, 2nd : %04X\n", buf[0], buf[1]);

        // ret = spiWrite(spi, &buf[0], 2);
        // printf("Return value for SPIWrite : %d\n", ret);

        // ret = spiXfer(spi, &buf[0], NULL, 2);
        // printf("Return value for SPIReadWrite : %d\n", ret);

        // exit(0);
    }
    else
    {
        printf("SPI failure, SPI Handle value : %d \n", spi);
        exit(1);
    }
#endif

    uint64_t *timeoffset;
    timeoffset = malloc(sizeof(*timeoffset));

    tv_diff = malloc(sizeof(*tv_diff));
    tv_started = malloc(sizeof(*tv_started));

    clock_gettime(CLOCK_REALTIME, tv_started);

#if SLEEP_ADJUST
    autoadjust_sleep(timeoffset, tv_started, tv_diff, xs, ys);
#endif

    sleep(2);
    clock_gettime(CLOCK_REALTIME, tv_started);

    // Loop here
    // +1 to seconds
    generate_data(tv_started);

    // send_data(timeoffset, tv_started, tv_diff, xs, ys);
    // send_data(timeoffset, tv_started, tv_diff, xs, ys);
    // send_data(timeoffset, tv_started, tv_diff, xs, ys);

    send_binary(ZERO, timeoffset, tv_started, tv_diff, xs, ys);
    //  send_binary(ONE, timeoffset, tv_started, tv_diff, xs, ys);
    //  send_binary(POS, timeoffset, tv_started, tv_diff, xs, ys);

    // send_signal(SIGNAL_MARK, OFFSET_MARK, timeoffset, tv_started, tv_diff, xs, ys);

#ifdef __arm__
    // close(i2c_fd);
    // SpiClosePort(spi);
    spiClose(spi);
    gpioTerminate();
#endif

#if GRAPH
    draw_image(xs, ys, array_iterator);
#endif
}