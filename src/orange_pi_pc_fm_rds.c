/*
 * OrangePiPcFmRds - Orange Pi PC (Allwinner H3) front-end for PiFmRds' FM/RDS
 * multiplex generator.
 *
 * The Raspberry Pi version modulates the VHF carrier by DMA-writing the
 * Broadcom GPCLK fractional divider.  Orange Pi PC/H3 has a different clock
 * tree and no compatible GPCLK/PWM FIFO pacing block, so this frontend does
 * NOT transmit RF from a GPIO pin.  It exposes the generated 228 kHz FM
 * multiplex as a WAV stream for an external RF modulator or SDR.  It keeps
 * the RDS, stereo and control-pipe logic intact and avoids poking
 * Raspberry-specific /dev/vcio mailbox or BCM registers.
 */

#include <locale.h>
#include <math.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "control_pipe.h"
#include "fm_mpx.h"
#include "rds.h"

#define DATA_SIZE 5000
#define MPX_SAMPLE_RATE 228000
#define DEFAULT_OUTPUT "orange_pi_pc_mpx.wav"

static char *g_control_pipe;
static int g_output_open;
static FILE *g_output;
static uint32_t g_samples_written;

static void write_u16_le(FILE *fp, uint16_t v)
{
    fputc(v & 0xff, fp);
    fputc((v >> 8) & 0xff, fp);
}

static void write_u32_le(FILE *fp, uint32_t v)
{
    write_u16_le(fp, v & 0xffff);
    write_u16_le(fp, (v >> 16) & 0xffff);
}

static void wav_header(FILE *fp, uint32_t samples)
{
    uint32_t data_bytes = samples * sizeof(int16_t);

    fseek(fp, 0, SEEK_SET);
    fwrite("RIFF", 1, 4, fp);
    write_u32_le(fp, 36 + data_bytes);
    fwrite("WAVEfmt ", 1, 8, fp);
    write_u32_le(fp, 16);
    write_u16_le(fp, 1);
    write_u16_le(fp, 1);
    write_u32_le(fp, MPX_SAMPLE_RATE);
    write_u32_le(fp, MPX_SAMPLE_RATE * sizeof(int16_t));
    write_u16_le(fp, sizeof(int16_t));
    write_u16_le(fp, 16);
    fwrite("data", 1, 4, fp);
    write_u32_le(fp, data_bytes);
}

static void close_everything(void)
{
    if (g_output_open && g_output) {
        wav_header(g_output, g_samples_written);
        fclose(g_output);
        g_output = NULL;
    }
    close_control_pipe();
    fm_mpx_close();
}

static void terminate(int num)
{
    close_everything();
    printf("Terminating: wrote %u multiplex samples.\n", g_samples_written);
    exit(num);
}

static void usage(const char *argv0)
{
    fprintf(stderr,
        "Syntax: %s [-audio file] [-pi pi_code] [-ps ps_text] [-rt rt_text]\n"
        "          [-ctl control_pipe] [-out mpx.wav] [-duration seconds]\n\n"
        "Orange Pi PC/H3 note: output is a 228 kHz mono FM multiplex WAV.\n"
        "This is not a GPIO RF transmitter. Feed the WAV to an SDR or an\n"
        "external VHF FM exciter; Broadcom GPCLK transmission is unavailable\n"
        "on Allwinner H3.\n", argv0);
}

int main(int argc, char **argv)
{
    char *audio_file = NULL;
    char *out_file = DEFAULT_OUTPUT;
    char *ps = NULL;
    char *rt = "OrangePiPcFmRds: FM-RDS multiplex from Orange Pi PC";
    uint16_t pi = 0x1234;
    double duration = 0.0;

    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];
        char *param = (arg[0] == '-' && i + 1 < argc) ? argv[i + 1] : NULL;

        if ((strcmp("-wav", arg) == 0 || strcmp("-audio", arg) == 0) && param) {
            audio_file = param;
            i++;
        } else if (strcmp("-pi", arg) == 0 && param) {
            pi = (uint16_t)strtol(param, NULL, 16);
            i++;
        } else if (strcmp("-ps", arg) == 0 && param) {
            ps = param;
            i++;
        } else if (strcmp("-rt", arg) == 0 && param) {
            rt = param;
            i++;
        } else if (strcmp("-ctl", arg) == 0 && param) {
            g_control_pipe = param;
            i++;
        } else if (strcmp("-out", arg) == 0 && param) {
            out_file = param;
            i++;
        } else if (strcmp("-duration", arg) == 0 && param) {
            duration = atof(param);
            i++;
        } else if (strcmp("-freq", arg) == 0 || strcmp("-ppm", arg) == 0) {
            fprintf(stderr, "Error: %s controls Raspberry Pi RF clock hardware and is not supported by the Orange Pi PC WAV frontend.\n", arg);
            fprintf(stderr, "This binary does not transmit directly from a GPIO pin; use -out to generate an MPX WAV for an SDR/exciter.\n");
            return 1;
        } else {
            usage(argv[0]);
            return 1;
        }
    }

    for (int i = 0; i < 64; i++) {
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = terminate;
        sigaction(i, &sa, NULL);
    }

    printf("Locale set to %s.\n", setlocale(LC_ALL, ""));

    if (fm_mpx_open(audio_file, DATA_SIZE) < 0)
        return 1;

    set_rds_pi(pi);
    set_rds_rt(rt);
    if (ps) {
        set_rds_ps(ps);
        printf("PI: %04X, PS: \"%s\".\n", pi, ps);
    } else {
        set_rds_ps("OPi-PC");
        printf("PI: %04X, PS: \"OPi-PC\".\n", pi);
    }
    printf("RT: \"%s\"\n", rt);

    if (g_control_pipe) {
        printf("Waiting for control pipe `%s` to be opened by the writer.\n", g_control_pipe);
        if (open_control_pipe(g_control_pipe) != 0) {
            fprintf(stderr, "Failed to open control pipe: %s.\n", g_control_pipe);
            g_control_pipe = NULL;
        }
    }

    g_output = fopen(out_file, "wb+");
    if (!g_output) {
        perror("fopen");
        close_everything();
        return 1;
    }
    g_output_open = 1;
    wav_header(g_output, 0);

    printf("Writing Orange Pi PC FM multiplex WAV to %s at %d samples/s.\n", out_file, MPX_SAMPLE_RATE);
    printf("Note: this is not direct GPIO RF transmission; use an SDR or external exciter.\n");

    uint32_t limit = duration > 0.0 ? (uint32_t)(duration * MPX_SAMPLE_RATE) : 0;
    float data[DATA_SIZE];
    while (!limit || g_samples_written < limit) {
        if (g_control_pipe)
            poll_control_pipe();
        if (fm_mpx_get_samples(data) < 0)
            break;
        for (int i = 0; i < DATA_SIZE && (!limit || g_samples_written < limit); i++) {
            float scaled = data[i] * 30000.0f;
            if (scaled > 32767.0f) scaled = 32767.0f;
            if (scaled < -32768.0f) scaled = -32768.0f;
            write_u16_le(g_output, (uint16_t)(int16_t)lrintf(scaled));
            g_samples_written++;
        }
    }

    terminate(0);
}
