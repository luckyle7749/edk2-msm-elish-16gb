# Device-tested release baseline

Release: `elish-16gb-v1.0.0`, device elish / 16GB RAM.

The attached `boot-elish.img` is the exact conservative image flashed to boot_b and verified by readback on the test device. It predates the repository packaging/build-wrapper commits, so its embedded firmware version uses the original device-port revision. Rebuilds use the checked-out fork revision and build timestamps; their SHA256 can differ.

- Image size: 7075840 bytes
- Image SHA256: `75bf196eb85d3a3f198591dd59879f255fa5c64e147c6e4fff8e6f3d301dafed`
- Device-port base: `e1952621f419f8db60ed28271264e1b5184c571d`
- TianoCore core: `e7aac7fc137e247edad22f7ee53b9a1fba227397`
- SimpleInit: `29e062805776db5ab7a61d07ef040d43aed71161`
- All dependency revisions: `SUBMODULES.lock`
- Target: AARCH64, RELEASE, upstream CLANG38 tool definition
- Original build command: `bash build.sh --device elish --skip-rootfs-gen` after SimpleInit rootfs generation in the established tree
- Host Clang: 23.1.3
- Host cross GCC: 16.2.0
- Cross GNU ld: 2.47

A new checkout must use `scripts/build-elish.sh`, which regenerates SimpleInit rootfs rather than depending on old generated files. Clean-tree build results are recorded in the release notes. The wrapper also addresses the host GCC C23 default encountered during clean validation.

The firmware-visible Android boot-header OS version/patch level are upstream compatibility metadata (11.0.0 / 2022-02), not a statement of Android or firmware security patch currency.

## Clean checkout validation

A separate checkout with freshly generated BaseTools and SimpleInit rootfs completed the full build using `bash scripts/build-elish.sh --offline` after pinned dependencies were initialized. Both first-time and repeat patch application passed. Host regression tests passed all 18 layout/boundary checks under ASan/UBSan.

That validation image has SHA256 `80aeb76e1de6287c4617045ea05a8fa0a4e6209af3889be40d069ef095197541`. It was compile-tested, not flashed; the released device-tested image remains the `75bf...afed` image identified above.
