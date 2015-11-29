####################################################################### 
# 
# calc_csum.py - Calculate checksums.
# 
####################################################################### 
#   Generate the csum for an input ascii block.
#
####################################################################### 
import sys
import os
import re
from struct import *

####################################################################### 
# 
#   This is the object used for calculating a checksum. 
####################################################################### 
class calc_checksum_object:
    def __init__(self):
        self._file_name = ""
        self._script_name =  "calc_csum.py"
        self._xor_total = 0
        self._data = []
        self._crc = -1
        self._last_data_word = -1

    def usage(self, script_name):
        print ''
        print ''
        print 'calc_csum.py - Calculate Sector Checksum Utility'
        print ''
        print 'Valid arguments:'
        print ' -check_crc [file with one block]'
        print '     Calculate the checksum for an input block.'
        print ''
        print ' -rba [checksum][final word]'
        print '    Calculate the rba displayed word for a given crc and final data word'
        print ''
        print 'Examples:'
        print ' Check checksum of a block of data located in a text file data.txt'
        print '    calc_csum.py -check_crc data.txt'
        print ''
        print ' Calculate the rba'
        print '    calc_csum.py -rba 0xd662 0x12345678'

    def process_args(self):
        """Validate the arguments and save them."""
        if len(sys.argv) < 2:
            self.usage(sys.argv[0])
            print "*** %s expects an argument" % self._script_name
            return 0
        if (sys.argv[1] == "-help" or sys.argv[1] == "-h"):
            self.usage(sys.argv[0])
            return 0
        if (sys.argv[1] == "-rba"):
            if len(sys.argv) < 4:
                print "%s expects [crc][last data word]" % self._script_name
                return 0
            self._last_data_word = int(sys.argv[3], 16)
            self._crc = int(sys.argv[2], 16)
            print "crc: " + hex(self._crc) + " data word: " + hex(self._last_data_word)
            return 1
        elif (sys.argv[1] != "-check_crc"):
            self.usage(sys.argv[0])
            print 'expected valid argument'
            return 0
        
        self._file_name = sys.argv[2]
        print("input_file: " + self._file_name)
        return 1

    def read_words(self):
        """Read in all 32 bit words from the sector."""
        with open(self._file_name, "r") as f:
            words = []
            for line in f:
                line = line.rstrip()
                line = line.rstrip(".")
                words = line.split(" ")
                print line
                for word in words:
                    word.rstrip("\n\r")
                    word.rstrip(" ")
                    word_data = int(word,16)
                    self._data.append(word_data)

    def calc_xor(self):
        """Calculate the xor of all 32 bit words in the block."""
        xor = 0
        for word in self._data:
            xor = xor ^ word
        return xor

    def calc_folded_word(self, word):
        """Fold over a 32 bit word into 16 bits."""
        word = (0xFFFF & ((word >> 16) ^ word))
        return word

    def cook_csum(self, xor):
        """Cook the checksum by adding the seed and rotating and folding the resulting crc."""
        SEED_CONST = 0x0000AF76
        checksum = xor & 0xFFFFFFFF
        checksum = checksum ^ SEED_CONST
        if checksum & 0x80000000:
            checksum = checksum << 1
            checksum = checksum | 1
        else:
            checksum = checksum << 1
        checksum = (0xffff & ((checksum >> 16) ^ checksum))
        return checksum

    def calculate_checksum(self):
        """Calculate the checksum and display the results."""

        if self._crc != -1:
            folded_word = self.calc_folded_word(self._last_data_word)
            rba_data = folded_word ^ self._crc
            print "Folded:         " + hex(folded_word)
            print "RBA Trace Data: " + hex(rba_data)
            return
        self.read_words()
        xor = self.calc_xor()
        cooked = self.cook_csum(xor)

        folded_word = self.calc_folded_word(self._data[127])
        rba_data = folded_word ^ cooked
        print "\n"
        print "Found " + str(len(self._data)) + " words."
        print "Final word: " + hex(self._data[127]) 
        print "Folded:     " + hex(folded_word)
        print "Total xor:  " + hex(xor)
        print
        print
        print "Calculated checksum: " + hex(cooked)
        print "RBA Trace Data:      " + hex(rba_data)

test_control_obj = calc_checksum_object()

# Parse our arguments and bail if an error was hit.
if not test_control_obj.process_args():
    exit(0)

# Just process the arguments.    
test_control_obj.calculate_checksum()
