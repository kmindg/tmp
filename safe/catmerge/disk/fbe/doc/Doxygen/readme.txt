#---------------------------------------------------------------------------
# Getting started with Doxygen
#---------------------------------------------------------------------------

1) Doxygen can be downloaded here: http://www.doxygen.org/
   - make sure you download Doxygen 1.5.6 or later
2) Graphviz is also needed and can be found here. http://www.research.att.com/sw/tools/graphviz/

#---------------------------------------------------------------------------
# How to build the Doxygen documentation
#---------------------------------------------------------------------------
1) First edit the Doxyfile_fbe, which is the doxygen configuration file.
   - Replace all instances of d:\views\myview with the path of the view
     you will be generating documentation for.

2) To generate documentation via the GUI.
   - Click "load" and select your Doxygen Config file
   - Click "start" to begin generating the documentation.

3) The output window will list any errors encountered in generating the documentation.  It is important to examine this for any issues with your doxygen comments.  If there are errors here, then your Doxygen documentation might not be getting generated as expected.

4) To view the documentation point your browser at d:\views\Doxygen\index.html
   Modify OUTPUT_DIRECTORY in the config file to change the output directory.