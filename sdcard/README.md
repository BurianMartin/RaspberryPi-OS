# SD card boot partition contents

Everything in this directory, plus a freshly-built `kernel.img`
(`make kernel_img`, then copy `build/kernel.img` in here), is what needs to
sit in the root of the SD card's single FAT32 partition for the Pi Zero W
to boot this kernel. Confirmed working, on real hardware, with exactly
these files.

## Files

- **`bootcode.bin`**, **`start.elf`**, **`fixup.dat`** — the Pi's own
  closed-ish GPU/VideoCore firmware. Not written by this project; these are
  vendored copies of whatever's currently confirmed working, so a working
  SD card can always be reproduced without having to re-source them or
  re-discover which version actually works. If they're ever missing or
  need updating, get matching versions from the official
  [`raspberrypi/firmware`](https://github.com/raspberrypi/firmware) repo's
  `boot/` directory — but there's no need to chase the latest `master` for
  this project; any release that boots is fine, and re-verifying a new set
  against real hardware before trusting it is worth the caution.
- **`config.txt`** — tells the firmware how to boot. The version here is
  the exact one confirmed working.

## The one thing that must never go back into `config.txt`

**`disable_commandline_tags=1` must never be set.** This was the actual
root cause of a very long, very confusing real-hardware debugging session
(see `PROGRESS.md` for the full story). That option silently changes the
firmware's kernel load address from `0x8000` to `0x0`, which this
project's linker script has no way to know about — every internal address
reference ends up `0x8000` bytes off, and the kernel corrupts its own
running code the moment it tries to relocate the exception vector table.
It produces a content-independent "runs briefly, then dies" pattern that
looks exactly like a code bug and is *not* — dozens of code changes were
tried against it before the actual cause was found. If a future `config.txt`
edit is ever tempted to add it back (e.g. copying a snippet from a
bare-metal tutorial that suggests it), don't.

## Flashing

This is a plain file copy onto a normally-FAT32-formatted SD card — not a
raw-disk-image write (no Etcher, no `dd`). Format the card FAT32, copy
these four files plus a built `kernel.img` onto it, eject properly, and
boot.
