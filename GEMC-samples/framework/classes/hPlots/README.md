
# Histogram Helper Classes

**Author:** Alon Sportes  
**Project:** `2N-Analyzer-e4nu`

# Overview

This folder contains lightweight ROOT helper classes used throughout the analysis framework to create, fill, draw, and save histograms in a consistent and controlled way.

The three main classes are:

- **hPlot1D** – Wrapper around a single `TH1D`
- **hPlot2D** – Wrapper around a single `TH2D`
- **hsPlots** – Manager for sliced histograms (`TH1D` or `TH2D`)

These classes standardize:

- Histogram naming conventions
- Axis titles and formatting
- Normalization behavior
- Log and linear scale output
- PDF export structure
- Integration with external `HistoList` containers

---

# The `hPlot1D` class

## Purpose

`hPlot1D` encapsulates a single `TH1D` and provides a structured interface for:

- Defining binning and axis ranges
- Encoding contextual metadata (reaction, region, final state)
- Filling with or without weights
- Normalization control
- Overlaying cut lines
- Exporting plots (linear and/or log scale)

It centralizes 1D histogram behavior so that all plots across the analysis follow the same visual and structural conventions.

---

## Core Responsibilities

### 1. Histogram Construction

Constructors support multiple metadata configurations:

- Reaction + final state + region
- Final state + region only
- Minimal constructor with only titles and axis limits
- Optional save name and save path

The underlying `TH1D` is allocated using `new`.

The ROOT title string is encoded as:

Title;X-axis;Y-axis

The histogram name is derived from the stat title and metadata fields.

---

### 2. Filling

Thin wrapper methods:

```cpp
hFill(double x);
hFill(double x, double weight);
```

These directly forward to `TH1D::Fill`.

---

### 3. Drawing and Saving

Main interface: `histPlotter1D(...)`

Features:

- Optional normalization to percent
- Automatic y-axis title selection
- Axis title and label sizing
- Optional statistics box suppression
- Optional symmetric cut line drawing
- Optional peak-location line
- Linear and/or log-scale PDF output
- Optional insertion into `THStack`
- Appending to external `HistoList`

If normalization is requested and the histogram is empty, an "Empty histogram" notice is drawn.

---

## Memory Model

`hPlot1D` owns its internal `TH1D` pointer. Lifetime must be handled by the owner of the `hPlot1D` instance or by transferring ownership externally.

---

# The `hPlot2D` class

## Purpose

`hPlot2D` performs the same role as `hPlot1D`, but for `TH2D` histograms.

It standardizes:

- 2D binning and axis limits
- Metadata encoding
- Drawing with color maps
- Z-axis log or linear scaling
- Export conventions

---

## Core Responsibilities

### 1. Histogram Construction

Constructors allow:

- Reaction + final state + region
- Final state + region only
- Optional save path and save name

The histogram title format follows ROOT convention:

Title;X-axis;Y-axis

The underlying histogram is allocated with `new TH2D`.

---

### 2. Filling

```cpp
hFill(double x, double y);
hFill(double x, double y, double weight);
hFillByBin(int binX, int binY, double content);
```

---

### 3. Drawing and Saving

Main interface: `histPlotter2D(...)`

Features:

- Draw with `colz`
- Optional statistics box
- Empty histogram overlay message
- Z-log-scale output
- Z-linear-scale output
- Integration with external `HistoList`

---

## Memory Model

`hPlot2D` owns its internal `TH2D` pointer. Deallocation responsibility follows the owning object.

---

# The `hsPlots` class

## Purpose

`hsPlots` manages a collection of histograms organized by slices of a chosen variable.

Instead of one histogram, it builds one histogram per slice interval.

This is used for studies such as:

- Resolution vs truth variable
- Efficiency vs kinematic bins
- Distributions within fixed kinematic ranges

---

## Slice Definition

Slices are defined as:

```cpp
std::vector<std::vector<double>> SliceLimits;
```

Each element must be:

```cpp
{lower_bound, upper_bound}
```

Validation ensures:

- Non-empty slice list
- `lower < upper`
- Non-decreasing lower bounds

Two slice schemes are supported:

1. Standard bounded slices: `lower <= value < upper`
2. Common-upper scheme (all slices share identical upper bound)

---

## Constructor Behavior

The constructor:

1. Validates slice limits
2. Parses a title template formatted as:

   ```
   MainTitle;X-axis;Y-axis
   ```

3. Creates one histogram per slice
4. Encodes slice limits into histogram names
5. Decorates titles using ROOT splitline formatting
6. Appends histograms to:
   - An external `HistoList`
   - A user-provided sliced histogram container

The histogram type is selected using:

```cpp
hsPlots::TH1D_TYPE
hsPlots::TH2D_TYPE
```

---

## Filling

```cpp
Fill(double sliceVar, double x, double y = 0, double weight = 1.0);
```

- Determines slice index using `FindSliceIndex`
- Fills only the corresponding histogram

---

## Saving

```cpp
SaveHistograms(const std::string& outputDir,
               const std::string& baseFileName) const;
```

Produces:

- Multi-page PDF containing all slices
- Individual per-slice PDFs
- Empty histogram overlays where needed

---

## Memory Model

`hsPlots` is intentionally non-owning:

- Histograms are stored in externally managed vectors
- The destructor does not delete histograms
- This avoids double-deletion when histograms are written or managed elsewhere

---

# Summary

| Class    | Manages | Owns Histogram | Main Use Case |
|----------|---------|----------------|---------------|
| hPlot1D  | TH1D    | Yes            | Standardized 1D plots |
| hPlot2D  | TH2D    | Yes            | Standardized 2D plots |
| hsPlots  | Many    | No             | Sliced histogram families |

These classes ensure consistent histogram handling, naming, drawing, and export across the entire analysis framework.