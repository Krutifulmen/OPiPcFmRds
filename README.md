OrangePiPcFmRds
===============

## Orange Pi PC FM-RDS multiplex generator (not a direct GPIO transmitter)

This repository keeps the Pi-FM-RDS audio/RDS encoder but **does not implement
direct FM transmission from an Orange Pi PC GPIO pin**.  The original Raspberry
Pi program depends on Broadcom-only hardware blocks: the VideoCore mailbox
allocator, BCM DMA, PWM FIFO pacing, and the GPCLK fractional divider.  Orange
Pi PC uses an Allwinner H3 SoC, so those registers and DMA pacing mechanisms are
not present.

What works here is the hardware-independent part: the program produces the same
228 kHz FM-stereo/RDS multiplex as a standard WAV file.  Feed that WAV into a
legal SDR or external VHF FM exciter.  If you need the original "GPIO pin emits
RF" behavior, this code is **not sufficient**; it would require a real
Allwinner-H3 clock/DMA RF backend and hardware validation on an Orange Pi PC.

## Build on Orange Pi PC / Armbian

```bash
sudo apt install build-essential libsndfile1-dev
cd src
make clean
make orange_pi_pc
```

The Orange Pi binary is `src/orange_pi_pc_fm_rds`.  It writes an MPX WAV file;
it does not configure H3 GPIO, PWM, CCU, or DMA for RF output.  The legacy
Raspberry Pi binary is still available as `make pi_fm_rds` when building on
Raspberry Pi hardware.


## Actually playing music on FM from Orange Pi PC

Direct FM carrier generation from the bare Orange Pi PC GPIO header is still not
implemented.  To make music audible on a normal FM receiver from Orange Pi PC,
use an external FM transmitter such as a Si4713-compatible module:

1. Connect the Si4713 module to Orange Pi PC I2C and make sure Linux exposes it
   as a V4L2 radio device such as `/dev/radio0`.
2. Connect Orange Pi PC line/headphone audio output to the Si4713 analog `LIN`,
   `RIN`, and `GND` inputs.
3. Run the helper script:

```bash
tools/orange_pi_pc_si4713_fm.sh -audio stereo_44100.wav -freq 107.9 -radio /dev/radio0
```

That script tunes the external transmitter through V4L2 and plays audio through
ALSA.  It is the practical supported path for “turn on music and hear it on an
FM receiver” with Orange Pi PC.

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
* `-freq` and `-ppm` are rejected on Orange Pi PC because they imply direct RF
  carrier control, which this H3 frontend does not implement.

## Legal warning

PiFmRds and this Orange Pi PC port are for experimentation only.  Transmitting
radio waves without the correct licence, filtering, power limits and test setup
is illegal in many countries.  Prefer a shielded lab setup or a compliant SDR/RF
exciter, and never attach an antenna to an unfiltered experimental source.
