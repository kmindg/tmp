Instructions for bullseye build and test (Only test in Rockies so far)
=========================================================================

All bullseye related files are in directory catmerge/disk/fbe/src/fbe_test/bullseye
You need change to this directory before running bullseye build and test

# Build and test
    perl regressMCR.pl -tagname bseye_armada64_checked -bullseye -build -view /path/to/your/workspace


# Build only
    perl regressMCR.pl -tagname bseye_armada64_checked -bullseye -build -buildOnly -view /path/to/your/workspace

# Test only
    perl regressMCR.pl -tagname bseye_armada64_checked -bullseye -view /path/to/your/workspace


Note: You need specific absolute path for your workspace.
