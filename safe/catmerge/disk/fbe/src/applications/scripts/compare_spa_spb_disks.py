#!/usr/bin/python

import sys, getopt
import re
from prettytable import PrettyTable
import itertools

x = PrettyTable(["SPA", "SPB"])
x.align["SPA"] = "l" # Left align
x.align["SPB"] = "l" # Left align
x.padding_width = 2


#Find if any bes has different serial number
def findbesdiff(f1_hash, f2_hash):
    global x
    common = list(set.intersection(set(f1_hash.keys()), set(f2_hash.keys())))
    for key in common:
        aextra = list(set(f1_hash[key]) - set(f2_hash[key]))
        bextra = list(set(f2_hash[key]) - set(f1_hash[key]))
        aextra.sort()
        bextra.sort()
        if aextra:
            arow = key + "\n" + ''.join(aextra)
        else:
            arow = ""
        if bextra:
            brow = key + "\n" + ''.join(bextra)
        else:
            brow = ""
        if not (arow == "" and brow == ""):
            x.add_row([arow, brow])
   

#Find any extra bes on either side; spa or spb
def findextrabes(f1_hash, f2_hash):
    global x
    aextra = list(set(f1_hash.keys()) - set(f2_hash.keys()))
    bextra = list(set(f2_hash.keys()) - set(f1_hash.keys()))
    for a, b in itertools.izip_longest(aextra, bextra):
        if a is None:
            arow = ""
        else:
            arow = a + "\n" + ''.join(f1_hash[a])
        if b is None:
            brow = ""
        else:
            brow = b + "\n" + ''.join(f2_hash[b])
        if not (arow == "" and brow == ""):
            x.add_row([arow, brow])
    
#Pick only the attributes we are interested            
def cleanfile(f):
    f_clean_hash = {}
    
    header = r"Bus \d+"
    #patterns = ['Bus ', 'Read', 'Write', 'Ticks:', 'Request Service', 'Stripe', 'Hot', 'Prct', 'Bind Signature:', 'Remapped', 'Written']
    patterns = ['Serial Number']
    patterns = '|'.join(patterns)
    
    l = f.readline()
    key = ""
    while l:
        b1 = re.search(header, l)
        if b1 is not None:
            key = l.strip()
            f_clean_hash.setdefault(key, [])
        b2 = re.search(patterns, l)
        if ((b2 is not None) and (key != "")):
            f_clean_hash[key].append(l)
        l = f.readline()
        
    return f_clean_hash
        

def main(argv):
    spafile = ''
    spbfile = ''
    result = 'result.txt'
    global x
    
    try:
        opts, args = getopt.getopt(argv, "ha:b:", ["help", "spa=", "spb="])
        if not opts:
            print 'CompareSPASPBDisks.py -a <SPA file> -b <SPB file>'
            sys.exit()
    except getopt.GetoptError:
        print 'CompareSPASPBDisks.py -a <SPA file> -b <SPB file>'
        sys.exit(2)
        
    for opt, arg in opts:
        if opt == '-h':
            print 'CompareSPASPBDisks.py -a <SPA file> -b <SPB file>'
            sys.exit()
        elif opt in ("-a", "--spa"):
            spafile = arg
        elif opt in ("-b", "--spb"):
            spbfile = arg
            
    with open(spafile, 'r') as spaf, open(spbfile, 'r') as spbf:
        spa_hash = cleanfile(spaf)
        spb_hash = cleanfile(spbf)

    findextrabes(spa_hash, spb_hash)
    findbesdiff(spa_hash, spb_hash)
    
    with open(result, 'w') as resf:
        resf.write(x.get_string())


if __name__ == "__main__":
    main(sys.argv[1:])