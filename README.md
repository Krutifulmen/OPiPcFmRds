OrangePiPcFmRds
===============

## FM-RDS multiplex generator ported for Orange Pi PC

This repository is the Orange Pi PC (Allwinner H3) port of Pi-FM-RDS.  The
original Raspberry Pi program used Broadcom-only hardware blocks: the VideoCore
mailbox allocator, BCM DMA, PWM FIFO pacing, and the GPCLK fractional divider.
Those blocks do not exist on Orange Pi PC, so the hardware-dependent part has
been replaced with an Orange Pi PC front-end that produces the same 228 kHz
FM-stereo/RDS multiplex as a standard WAV file.

Use the generated multiplex with an external RF exciter/SDR that is legal in
your country and properly filtered.  Direct GPIO VHF transmission from the
Raspberry Pi version is intentionally not emulated on the H3 because the H3
clock/PWM hardware is not register-compatible and cannot safely run the original
BCM DMA chain.

## Build on Orange Pi PC / Armbian

```bash
sudo apt install build-essential libsndfile1-dev
cd src
make clean
make orange_pi_pc
```

The Orange Pi binary is `src/orange_pi_pc_fm_rds`.  The legacy Raspberry Pi
binary is still available as `make pi_fm_rds` when building on Raspberry Pi
hardware.

## Run

Generate a finite multiplex WAV:

```bash
sudo ./orange_pi_pc_fm_rds -audio stereo_44100.wav -ps OPi-PC -rt 'Orange Pi PC RDS test' -duration 30 -out mpx.wav
```

Generate continuously until interrupted:

```bash
sudo ./orange_pi_pc_fm_rds -ps OPi-PC -rt 'Live Orange Pi PC RDS' -out live_mpx.wav
```

Options:

* `-audio file` reads mono or stereo audio through libsndfile.  Use `-` for
  standard input.
* `-pi FFFF`, `-ps TEXT`, and `-rt TEXT` set the RDS PI, PS, and radiotext.
* `-ctl fifo` keeps the runtime PS/RT/TA control pipe support from Pi-FM-RDS.
* `-out file.wav` selects the generated multiplex WAV path.
* `-duration seconds` stops automatically after the requested duration.
* `-freq` and `-ppm` are accepted for script compatibility but ignored on Orange
  Pi PC because they controlled Raspberry Pi RF clock hardware.

## Legal warning

PiFmRds and this Orange Pi PC port are for experimentation only.  Transmitting
radio waves without the correct licence, filtering, power limits and test setup
is illegal in many countries.  Prefer a shielded lab setup or a compliant SDR/RF
exciter, and never attach an antenna to an unfiltered experimental source.
