# LUNA

<p align="center">
  <img src="nhddl/img/logo/logo.png" alt="LUNA logo" width="700">
</p>

LUNA (**Lightweight Unified Neutrino Access**) is a visual PlayStation 2 game
loader derived from [NHDDL](https://github.com/pcm720/nhddl) and
[Neutrino](https://github.com/rickgaiser/neutrino). It preserves NHDDL's
established ISO discovery, title configuration, and Neutrino launch path while
adding a new library experience and a coordinated LUNA runtime.

The current configuration is designed for an FMCB-hosted frontend and Neutrino
runtime with games, artwork, and writable library state on an internal
ATA/exFAT hard drive.

> **Project status:** this repository is a local release-staging tree. No public
> release, hosted binary, or release version is implied yet.

## Requirements

- **Compatible PlayStation 2:** the packaged setup targets a PS2 that can run
  Free McBoot and expose an internal DEV9/ATA hard drive. LUNA does not yet
  publish a complete model-by-model compatibility matrix; console revision,
  adapter, bridge, and hard-drive compatibility can affect results.
- **FMCB memory card:** a working Free McBoot installation with enough space
  for the LUNA application. The packaged configuration uses slot 1 (`mc0:`).
- **Internal ATA/exFAT drive:** an internal hard drive presented through DEV9,
  with an MBR- or GPT-formatted drive containing an exFAT partition. The
  supplied configuration uses `mode: ata`.
- **Network adapter or HDD bridge:** a PS2-compatible network adapter and
  SATA/IDE bridge that reliably exposes the internal drive through the ATA
  backend. There is no universal adapter/bridge compatibility list; validate
  unfamiliar hardware before relying on it for regular use.
- **Artwork format:** artwork is optional and is not required to launch an
  ISO. For the complete library presentation, use title-ID-matched PNG files:
  140x200 covers, optional transparent disc labels, and optional 256x256
  square PSBBN artwork. The exact filenames are shown in [Artwork layout](#artwork-layout).

## Installation

The supported hardware setup runs LUNA and Neutrino from a Free McBoot (FMCB)
memory card in slot 1 while games, artwork, and writable library state remain
on the internal ATA/exFAT hard drive.

1. Back up the FMCB memory card and the target hard drive.
2. Obtain the packaged `LUNA-FMCB-mc0.zip` archive and extract its
   `APP_LUNA` directory.
3. Copy `APP_LUNA` to the root of the FMCB memory card and rename the copied
   directory to `LUNA` if necessary. Confirm these paths exist:

   ```text
   mc0:/LUNA/luna.elf
   mc0:/LUNA/luna.yaml
   mc0:/LUNA/neutrino.elf
   mc0:/LUNA/config/
   mc0:/LUNA/modules/
   ```

4. In the Free McBoot Configurator, add a menu item named `LUNA` with the path
   `mc0:/LUNA/luna.elf`, then save the FMCB configuration.
5. Keep the game drive’s existing ISO and artwork layout. The packaged
   configuration uses `mode: ata`; do not copy `ART`, Favorites, cache, or
   options files to the memory card.

Launch **LUNA** from the FMCB menu. For a card installed in slot 2, change the
FMCB path and `return_path` in `luna.yaml` from `mc0:` to `mc1:`. See
[FMCB.md](FMCB.md) for the complete memory-card layout and hardware checklist.

## Preparing the game drive

Prepare the ATA/exFAT drive before launching LUNA for the first time. Keep a
backup of the drive while testing a new adapter, bridge, or console.

1. Format or prepare an MBR- or GPT-partitioned drive with an exFAT partition
   that the PS2's ATA setup can mount. Keep the partition's root available for
   the game and artwork directories.
2. Copy game images as `.iso` or `.ISO` files into the drive. They may be in
   the root or in ordinary folders such as `/DVD/` or `/games/`, up to the
   scanner's supported directory depth. Human-readable names are fine; LUNA
   reads the title ID from each ISO and uses that ID to match artwork.

   ```text
   /DVD/Example Game.iso
   /games/Another Game.iso
   ```

   Do not place ISOs inside `/ART/` or other library/system directories that
   the scanner skips. Keep each image as a complete, valid PS2 ISO.
3. Create `/ART/` at the root of the game drive and add any optional artwork
   using the title ID from the ISO. Classic uses OPL-compatible covers and
   optional disc labels; Collection, Grid, Constellation, and Orbit use the
   optional square artwork under `/ART/PSBBN/`.
4. If an existing drive already works with OPL, preserve its ISO and artwork
   layout. LUNA uses the same title-ID conventions for OPL-compatible cover
   art and does not require commercial game data or artwork to be moved to the
   FMCB memory card.

### HDL/OPL compatibility

The packaged FMCB configuration is ATA-only. It does not scan an APA/HDL drive
unless LUNA is configured with `mode: hdl` instead. LUNA retains NHDDL's HDL
backend and OPL metadata conventions for compatible setups: artwork and title
options are read from the OPL partition named by
`hdd0:__common/OPL/conf_hdd.cfg`, with `+OPL` and `__common/OPL` used as
fallbacks. The HDL backend does not support virtual memory cards or virtual
hard drives.

## What LUNA adds

LUNA is more than a visual rename of NHDDL. It contains coordinated changes to
both the NHDDL-derived frontend and the Neutrino-derived game runtime.

| Area | LUNA addition |
| --- | --- |
| Library interface | A PS2-inspired glass interface with an animated star field, crystal elements, LUNA branding, and five switchable library views. |
| Classic view | A refined list-and-cover layout with a rotating disc label, Favorites controls, and paired cover/disc artwork. |
| Collection view | A PSBBN-inspired cover flow with animated focus changes and a Collection/Favorites filter. |
| Grid view | A 4x4 artwork grid with paged caching, row-cascade transitions, large selected-cover preview, and fast-track shoulder navigation. |
| Constellation view | A spatial cover map with animated focus movement and a Square-button Random Scan that avoids reselecting the current title. |
| Orbit view | A depth-sorted ring of covers with perspective, fading, and shared artwork caching. |
| Favorites | Per-drive Favorites stored in `/LUNA/favorites.txt`, shared by Classic and Collection without modifying the game library. |
| Artwork | OPL-compatible covers plus optional disc labels and PSBBN-style square artwork, with view-specific caching and GS VRAM recovery. |
| HDD-focused operation | The library accepts ATA and HDL hard-drive sources only. The shipped configuration uses internal ATA/exFAT and avoids waiting for a nonexistent second mass-storage device. |
| Safer persistent state | LUNA writes cache, last-title, global options, and per-title settings under `/LUNA`, reads legacy `/nhddl` state as a fallback, bounds stored paths, and replaces key files only after a complete temporary write. |
| FMCB deployment | The frontend and runtime can live together at `mc0:/LUNA` while each hard drive retains its own artwork, cache, settings, and Favorites. |
| In-game return | **Work in progress.** The planned LUNA Neutrino runtime will recognize a held controller combination and return directly to a configured memory-card ELF or through the HDD/browser boot chain. |
| Return safety | The planned return path will request a coordinated optical/DEV9 shutdown and fail closed if safe shutdown cannot be confirmed. |
| Physical power button | LUNA adds a dedicated IOP-side safe-shutdown path that preserves the console's normal power-off behavior. Unlike NHDDL, it coordinates DEV9 shutdown before issuing the standard power-off command. |

### Physical power-button shutdown

NHDDL did not provide a coordinated safe-shutdown path for the console's
physical power button. That distinction matters when a game is using the
DEV9-connected hard drive: simply resetting or powering off while storage work
is still active can leave outstanding I/O interrupted.

LUNA keeps the physical button separate from the planned controller-based
in-game return. The native power event is handled by a priority-1 listener on
the IOP CD/DVD side, rather than depending on an EE pad hook, a VBlank loop, or
the running game's controller support. The listener acknowledges the hardware
event, stops active emulated optical work, asks DEV9 to shut down through its
normal shutdown callback, and only then issues the standard `sceCdPowerOff`
command. The power-off path does not require DEV9 to be present, so the front
panel button continues to behave normally for USB, MX4SIO, and other launch
paths as well.

This is intentionally a true power-off operation: pressing the physical
button does not reinterpret the event as an in-game return or reload LUNA.
The same coordinated shutdown service is also used by the planned return path,
but the controller-based in-game return remains a work in progress.

## Library views and controls

Press **Circle** to cycle through **Classic**, **Collection**, **Grid**,
**Constellation**, and **Orbit**.

- **Cross:** launch the selected game.
- **Triangle:** open the selected game's options.
- **Start:** open global options.
- **Square in Classic:** add or remove the selected game from Favorites.
- **Select in Classic or Collection:** switch between the full library and
  Favorites.
- **Square in Constellation:** start Random Scan. Any deliberate navigation
  input cancels it.
- **L1/L2 or R1/R2 in Grid:** tap for one page or hold for fast-track paging.
  Artwork loading resumes only at the final page when the buttons are released.

The in-game return feature is currently a work in progress. Its planned control
combination is **L1 + L2 + R1 + R2 + Start + Select** held for roughly one
second; the default FMCB configuration is intended to return to
`mc0:/LUNA/luna.elf`.

## Artwork layout

LUNA continues to use OPL-compatible title IDs and PNG artwork names:

```text
/ART/<TITLE_ID>_COV.png       140x200 cover used by Classic
/ART/<TITLE_ID>_ICO.png       optional 64x64 transparent disc label used by Classic
/ART/PSBBN/<TITLE_ID>.png     optional 256x256 square artwork for other views
```

Missing optional artwork is handled without changing the underlying ISO list.
The original NHDDL cover-loading path and metadata-device fallback are retained.

## Storage and configuration

The supplied FMCB configuration uses:

```yaml
mode: ata
return_path: mc0:/LUNA/luna.elf
```

Writable state stays with the game drive:

```text
/ART/
/LUNA/cache.bin
/LUNA/lastTitle.bin
/LUNA/favorites.txt
/LUNA/global.yaml
/LUNA/<game name>.yaml
```

This means swapping hard drives also swaps their library, artwork, Favorites,
cache, and per-game configuration. Existing `/nhddl` cache and option files can
still be read for migration, but new writes go to `/LUNA`.

## Source layout

- `nhddl/`: LUNA's modified NHDDL-derived frontend.
- `neutrino/`: LUNA's modified Neutrino-derived backend.
- `tools/`: local build, FMCB packaging, and package-verification helpers.
- `LICENSES/`: licenses retained from upstream projects and dependencies.

See [UPSTREAM.md](UPSTREAM.md) for exact source lineage,
[AUTHORS.md](AUTHORS.md) for attribution, [BUILDING.md](BUILDING.md) for local
build instructions, [FMCB.md](FMCB.md) for the installation layout, and
[NHDDL_FIDELITY.md](NHDDL_FIDELITY.md) for the inherited behavior LUNA preserves.

## Repository contents

This source tree intentionally does **not** contain commercial game data,
console firmware, downloaded cover artwork, virtual hard-drive images, emulator
binaries, development backups, or release packages.

LUNA modifications and original LUNA code are attributed to
**Danny Nunez (dnunezx) 2026**. Upstream code remains credited to its respective
authors and is distributed under the licenses preserved in `LICENSES/`.
