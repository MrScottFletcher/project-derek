# Derek Unit QLC+ 3D Model Parts

This folder contains separate Wavefront OBJ files for the QLC+ fixture model
system:

- `base.obj`: fixed tin can body, rolled rim, and front pinspot lens housing.
- `yoke.obj`: lift bracket and lid panel intended to follow the lift/tilt axis.
- `head.obj`: paired eyeballs, eye LED lenses, pan bar, and simple forward beam
  reference cones.

## Orientation

- Y is up.
- Front is negative Z.
- The can sits on the ground plane at Y = 0.
- The eyeballs are centered around the fixture centerline so pan rotation turns
  both eyes together.

## Notes

The files are intentionally low-poly and material-free for broad OBJ importer
compatibility. The grouped mesh names inside each file describe the visible
sub-parts, but QLC+ should load each component by the filename.
