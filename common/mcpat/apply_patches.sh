#!/bin/bash

echo "[Graphite/McPAT] Applying patches for 64-bit compilation"
patch -p1 < patches/64bit.patch
echo "[Graphite/McPAT] Applying patches for error exit codes"
patch -p1 < patches/exit_codes.patch
echo "[Graphite/McPAT] Applying patches for shared cache bug"
patch -p1 < patches/sharedcache.patch
echo "[Graphite/McPAT] Applying patches for printing out component energies (tag/data array)"
patch -p1 < patches/component_energy.patch
echo "[Graphite/McPAT] Applying patches for using correct output width"
patch -p1 < patches/output_width.patch
