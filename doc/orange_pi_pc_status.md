# Orange Pi PC hardware status

This repository does **not** currently contain a direct Orange Pi PC GPIO RF
transmitter backend.

The Raspberry Pi transmitter path in `src/pi_fm_rds.c` is tied to Broadcom
hardware.  It allocates DMA memory through `/dev/vcio`, paces DMA through the
BCM PWM FIFO, and repeatedly writes the Broadcom GPCLK divider to frequency
modulate a carrier.  Orange Pi PC is based on the Allwinner H3 SoC and does not
provide those Broadcom registers or the same clock-divider/DMA behavior.

The implemented Orange Pi PC binary, `orange_pi_pc_fm_rds`, therefore only uses
the portable parts of the project:

- RDS group generation and shaping.
- FM stereo multiplex generation at 228 kHz.
- Runtime PS/RT/TA updates through the existing control pipe.
- WAV output for an SDR or external VHF FM exciter.

A true GPIO RF port would need a new Allwinner-H3-specific backend that proves:

1. which H3 clock output can generate a VHF-range carrier on an accessible pin;
2. whether its divider can be changed at a 228 kHz update rate with low enough
   jitter for FM/RDS;
3. how to pace H3 DMA safely from user space or through a kernel driver; and
4. that the result is legal, filtered, and stable on real Orange Pi PC hardware.

Until those points are implemented and tested on hardware, this project should
be treated as an Orange Pi PC **multiplex generator**, not as a complete RF
transmitter.

## Supported real RF path

For actual music on an FM receiver, use external transmitter hardware instead of
trying to synthesize VHF directly on an H3 GPIO pin.  The repository includes
`tools/orange_pi_pc_si4713_fm.sh` for Si4713-compatible transmitters exposed by
Linux as a V4L2 radio device, typically `/dev/radio0`.

The Orange Pi PC controls the Si4713 over I2C/V4L2 and plays audio through ALSA;
the Orange Pi analog audio output must be wired to the transmitter module's line
inputs.  This provides a real, testable FM workflow while keeping the RF carrier
generation inside hardware designed for FM transmission.
