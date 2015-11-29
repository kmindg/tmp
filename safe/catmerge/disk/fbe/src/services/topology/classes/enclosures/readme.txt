How to add a new leaf enclosure.
1. cleartool checkout catmerge\disk\fbe\src\services\topology\classes\enclosures
2. cleartool mkdir catmerge\disk\fbe\src\services\topology\classes\enclosures\new_leaf_enclosure
3. cd catmerge\disk\fbe\src\services\topology\classes\enclosures\new_leaf_enclosure
4. cleartool mkelem -nc -elt text_file CONTROL.mak 
5. cleartool mkelem -nc -elt text_file SOURCES.mak 
   We can copy the content of the two files from sas_bullet_enclosure as a start.  
5. in the new SOURCES.mak, change TARGETNAME to new_leaf_enclosure.
6. checkout catmerge\disk\fbe\src\services\topology\classes\enclosures\SOURCE.mak
7. add new_leaf_enclosure to SUBDIRS
8. checkout catmerge\disk\fbe\src\services\topology\classes\SOURCE.mak
9. add new_leaf_enclosure.lib to TARGETLIBS
10. add new_leaf_enclosure.lib to targetmap.txt
11. make sure to add your new fbe_class_methods to physical_package_class_table.h
