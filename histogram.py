import sys
from collections import OrderedDict

def get_histogram(file):
    hist_dict = {}
    data = open(file, 'rb').read()
    for byte in data:
        value = ord(byte)
        hist_dict.setdefault(value, 0)
        hist_dict[value] += 1
    result = set()
    for key, item in hist_dict.items():
        result.add((key, item))
    return result

hist1, hist2 = map(get_histogram, sys.argv[1:3])


if hist1 != hist2:
    print "Histograms are different"
    print hist1
    print hist2
else:
    print "Histograms are equivalent"
