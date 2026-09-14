# AMaps Class Guide

**Author:** Alon Sportes  
**Project:** `2N-Analyzer-e4nu`

## Overview

`AMaps` manages detector acceptance products in `(phi, theta)` momentum slices for electrons, protons, and neutrons.

It supports two modes:

- `AMaps`: binary acceptance maps (`0/1`) after thresholding `Reco/TL`.
- `WMaps`: continuous weight maps for acceptance correction.

## Core Responsibilities

- Fill truth/reco hit maps from events.
- Compute acceptance efficiency (`Reco/TL`).
- Build in-memory map matrices:
  - `e_AMap_Slices`, `p_AMap_Slices`, `n_AMap_Slices`, `n_AMap_ConstSlices`
  - `e_WMap_Slices`, `p_WMap_Slices`, `n_WMap_Slices`, `n_WMap_ConstSlices`
- Save/load map slices in `.par` format.
- Save map ROOT/PDF products.
- Provide acceptance matching and weight lookup during analysis.

## Main Methods

- `hFillMaps(...)`: fills per-particle truth/reco histograms by event.
- `CalcAcceptanceEfficiency()`: computes `Reco/TL` histograms.
- `GenerateFilteredRecoMaps(cP_minR, nP_minR)`: applies thresholds to ratio maps.
- `GenerateMapMatrices(cP_minR, nP_minR)`: converts histograms to AMap/WMap matrices.
- `DrawAndSaveMaps(...)`: full generation + draw + save pipeline.
- `SaveMaps(...)`: writes mode-dependent `.par` slice files.
- `ReadMapSlices(...)` / `ReadMap(...)`: load `.par` slices.
- `MatchToAMap(...)`, `GetWeight(...)`, `IsInFDQuery(...)`: analysis-time map queries.

## New WMap Loader From ROOT Ratios

`LoadWMapsFromRatioROOTFiles(...)` in `AMaps` reads ratio ROOT files and recreates WMap `.par` files.

Signature:

```cpp
void LoadWMapsFromRatioROOTFiles(
    const std::string& SampleName,
    const std::string& InputAcceptanceMapsDirectory,
    const std::string& OutputAcceptanceMapsDirectory = ""
);
```

Behavior:

- Input directory is used to read ratio ROOT files and slice-limit references.
- Output directory is used to write WMap `.par` files.
- If output directory is empty, input directory is reused.
- Thresholding used during WMap export:
  - electrons/protons: keep ratio `>= 0.7`, else `0`
  - neutrons (regular and const): keep ratio `>= 0.2`, else `0`

## `.par` Output Format

Each slice file includes:

- `Lower_P_lim`, `Upper_P_lim`
- bin labels (`HistElectronSliceNumOfXBins` / `HistNucSliceNumOfXBins`, etc.)
- row data with `Weight\t` and `:`-separated bin values

Each particle directory also has a slice-limits file:

- `e_slice_limits.par`
- `p_slice_limits.par`
- `n_slice_limits.par`

## Directory Layout (Current)

Per sample (`<SampleName>`):

- AMaps mode output:
  - `e_AMaps_by_slice/`
  - `p_AMaps_by_slice/`
  - `n_AMaps_by_slice/`
  - `n_AMaps_by_ConstSlice/`
  - `e_extended_AMaps_by_slice/`
- WMaps mode output:
  - `e_WMap_by_slice/`
  - `p_WMap_by_slice/`
  - `n_WMap_by_slice/`
  - `n_WMap_by_ConstSlice/`

ROOT summary outputs are also written with prefixes:

- `00_AMaps_BC_-_`
- `01_AMap_TL_-_`
- `02_AMap_Reco_-_`
- `03_AMap_Ratio_-_`
- `04_cPart_Sep_AMaps_-_` (and const neutron variant)
- `05_AMaps_-_`

## Binning Profiles

Electron profiles:

- `reformat_e_bins`
- `varying_P_e_bins`
- `uniform_P_e_bins`
- `equi_inverted_P_e`

Nucleon profiles:

- `equi_inverted_P_nuc`
- `varying_P_nuc_bins`
- `uniform_P_nuc_bins`

## Usage Notes

- In generation mode, use the generation constructor and call `DrawAndSaveMaps(...)`.
- In loading mode, use the loading constructor and `ReadMapSlices`/query methods.
- `IsInFDQuery(...)` behavior depends on generation/loading context and map availability.
- Neutron maps are managed both in regular momentum slices and const slices.
