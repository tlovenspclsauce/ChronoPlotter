# Garmin Xero C1 File Preparation Guide for ChronoPlotter

This guide explains how to prepare Garmin Xero C1 chronograph data files for import into ChronoPlotter.

## Supported File Formats

ChronoPlotter supports two Garmin file formats:
- **XLSX (Excel)** - Recommended for multiple series
- **CSV (Comma-Separated Values)** - Single series only

## Quick Start: Using the Built-In File Preparation Tool

**New in v2.2.2!** ChronoPlotter now includes a built-in tool to automatically combine multiple Garmin CSV files into a single multi-series XLSX file.

### How to Use "Prepare Garmin files"

1. On the main ChronoPlotter screen, click the **"Prepare Garmin files"** button
2. In the file selection dialog, select **2 or more CSV files** exported from your Garmin Xero C1
   - Hold `Ctrl` (Windows) or `Cmd` (Mac) to select multiple files
   - Files will be processed in the order they are selected
3. Click **Open** to begin processing
4. Choose where to save the combined XLSX file
5. The tool will automatically:
   - Read each CSV file
   - Create a separate worksheet for each file
   - Combine them into a single XLSX workbook
   - Name the worksheets as Sheet1, Sheet2, Sheet3, etc.

### Benefits of Using This Tool

- **No manual editing required** - Automatically combines CSV files
- **Preserves all data** - Each CSV becomes a worksheet with all original data intact
- **Ready to import** - The resulting XLSX file can be immediately loaded using "Select Garmin CSV/XLSX file"
- **Error handling** - Validates files and provides clear error messages if something goes wrong

### Workflow Example

1. Export individual CSV files from your Garmin Xero C1 (one per shooting string)
2. Click **"Prepare Garmin files"** in ChronoPlotter
3. Select all exported CSV files
4. Save the combined XLSX file
5. Click **"Select Garmin CSV/XLSX file"** to load the combined data
6. All series will be imported at once, ready for analysis

## XLSX File Format (Multi-Series Support)

XLSX files can contain multiple shooting series, with each series stored on a separate worksheet within the same workbook.

### Worksheet Structure

Each worksheet must follow this specific format:

#### Row 1: Series Name
- **Cell A1**: Enter your series name
- Example: `"45.5gr Load Test"`, `"Series 1"`, `"OCW Test - 308 Win"`

#### Row 2: Column Headers
- **Cell A2**: `"Shot #"` or similar (ignored by parser)
- **Cell B2**: Velocity unit indicator - **must contain** either:
  - `"FPS"` for feet per second
  - `"MPS"` or `"M/S"` for meters per second
- Example: `"Velocity (FPS)"` or `"Speed (m/s)"`

#### Row 3 onward: Shot Data
Each row contains one shot record:
- **Column A**: Shot number (1, 2, 3, etc.)
- **Column B**: Velocity value (numeric)

#### Optional: Date/Time Row
Anywhere after Row 2, you can include:
- **Column A**: The exact text `"DATE"`
- **Column B**: Date and time in the format: `"MM/DD/YYYY at HH:MM:SS"`
  - Example: `"5/15/2024 at 14:30:00"`

### Example XLSX Worksheet

```
A                   B
1  45.5gr Test      
2  Shot #           Velocity (FPS)
3  1                2850
4  2                2847
5  3                2852
6  4                2849
7  5                2853
8  DATE             5/15/2024 at 14:30:00
```

### Multi-Worksheet Example

Create multiple worksheets in one XLSX file:
- **Sheet1**: "43.0gr Load" with 5 shots
- **Sheet2**: "43.5gr Load" with 5 shots  
- **Sheet3**: "44.0gr Load" with 5 shots

Each sheet will be imported as a separate series in ChronoPlotter.

## CSV File Format (Single Series)

CSV files contain a single shooting series.

### CSV Structure

#### Row 1: Series Name
- **Cell 1**: Series name
- Example: `"Morning Range Session"`

#### Row 2: Velocity Unit
- **Cell 1**: Header label (ignored)
- **Cell 2**: Must contain `"FPS"` or `"MPS"`/`"M/S"` 

#### Row 3+: Shot Data
- **Column 1**: Shot number (1, 2, 3, etc.)
- **Column 2**: Velocity value

#### Optional: Date Row
- **Column 1**: The exact text `"DATE"`
- **Column 2**: Date string (format flexible)

### Example CSV File

```
45.5gr Load Test
Shot #,Velocity (FPS)
1,2850
2,2847
3,2852
4,2849
5,2853
DATE,5/15/2024
```

## Important Notes

### International Number Formats
ChronoPlotter handles both decimal formats:
- Period as decimal separator: `2850.5`
- Comma as decimal separator: `2850,5`

### Data Validation
Before importing, ensure:
- ? All velocity values are numeric (no text or symbols)
- ? Shot numbers are sequential integers starting from 1
- ? The velocity unit indicator (`FPS` or `MPS`/`M/S`) appears in Row 2, Column 2 (XLSX) or Row 2, Column 2 (CSV)
- ? Series name is present in Row 1, Column 1
- ? At least one shot is recorded per series

### What Gets Ignored
ChronoPlotter ignores:
- Worksheet tab names (uses Row 1 name instead)
- Sighter shots or hidden shots (if marked in original data)
- Empty rows
- Rows with invalid/non-numeric velocity data

### Common Issues

**Issue**: Series not detected  
**Solution**: Verify Row 2, Column 2 contains `"FPS"` or `"MPS"`/`"M/S"` 

**Issue**: Velocities not importing  
**Solution**: Ensure Column A contains sequential integers (1, 2, 3...) and Column B contains only numbers

**Issue**: Wrong velocity units displayed  
**Solution**: Check that Row 2, Column 2 explicitly contains the unit indicator

## Exporting from Garmin Xero C1

### For XLSX (Recommended):

#### Method 1: Use ChronoPlotter's Built-In Tool (Easiest)
1. Export individual CSV files from your Garmin Xero C1
2. In ChronoPlotter, click **"Prepare Garmin files"**
3. Select all CSV files you want to combine
4. Save the combined XLSX file
5. Import using **"Select Garmin CSV/XLSX file"**

#### Method 2: Direct XLSX Export
1. Connect your Garmin Xero C1 to your computer
2. Use the Garmin Connect app or device export feature
3. Select **XLSX format** for export
4. The device will create one worksheet per string
5. Import the XLSX file directly into ChronoPlotter

### For CSV:
1. Export individual strings as CSV from your Garmin device
2. Each CSV will contain one series
3. Import CSV files one at a time, or use the **"Prepare Garmin files"** tool to combine them into an XLSX workbook

## Tips for Best Results

1. **Use descriptive series names** - Include charge weight, date, or test purpose
   - Good: `"43.5gr IMR4064 - 5/15/24"`
   - Avoid: `"Series 1"`

2. **Keep worksheets organized** - In XLSX files, arrange worksheets in ascending charge weight order

3. **Remove unwanted data before export** - Delete sighter shots or test shots from the device before exporting

4. **Verify units** - Double-check that all series use the same velocity unit (either all FPS or all MPS)

5. **Include dates** - The DATE row helps track when series were recorded for reference

6. **Use the preparation tool for multiple CSV files** - Instead of manually combining files, use the built-in "Prepare Garmin files" button to save time

## Getting Help

If you encounter issues importing Garmin files:
1. Verify your file matches the format specifications above
2. Check the ChronoPlotter debug output for specific error messages
3. Report issues on the [ChronoPlotter GitHub repository](https://github.com/tlovenspclsauce/ChronoPlotter)

---

**Last Updated**: December 10, 2025  
**Compatible with**: ChronoPlotter v2.2.2 and later
