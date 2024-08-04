Tools: dnload, custom synth

All release binaries can be found in the "rel/" subfolder. The intro has ports
for FreeBSD, Linux and Windows. Compo version is for FreeBSD-ia32, but the
Linux-ia32 version is also under 4k. However, Ubuntu 24.04 no longer runs
minified binaries so we decided to release for FreeBSD like old times and lose
some 70 bytes in the process.

The rendering came to me in a dream (no, really, -Trilkk). It requires
GL_NV_mesh_shader, so it plain won't work on AMD or Intel GPUs. Nvidia GPUs
have to be Turing (20X0 RTX series) or newer. Some effects are also very
hostile to video compression, we'll try to provide a 'lossless' video file.
