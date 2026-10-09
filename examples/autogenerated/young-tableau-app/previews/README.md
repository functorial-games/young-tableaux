# Actual Idriç host picture previews

These previews come from the eleven PPM files emitted and read back by the
successful source-bound host qualification. They are not Android screenshots.
The original frame identities are in `../_/host-evidence/frames.sha256`.

| Preview | Qualified source frame |
| --- | --- |
| `rsk-and-jeu.png` | `jeu-corner-removed.ppm` |
| `result-page-1.png` | `laboratory-graph-page-1.ppm` |
| `result-page-2.png` | `laboratory-graph-page-2.ppm` |

The PNGs are lossless conversions made by the installed ImageMagick executable
`convert-im6.q16`. `interaction.gif` takes, in order, `initial.ppm`,
`rsk-insertion-1.ppm`, `rsk-insertion-2.ppm`, `jeu-move-1.ppm`, `jeu-move-2.ppm`,
and `jeu-corner-removed.ppm`. It crops `360x490+0+55`, resets the canvas origin,
uses a one-second delay per frame, optimizes the layers, and loops. The GIF
palette is an illustration; assertions compare original production raster
values and the PPM file sink checks those original bytes.
