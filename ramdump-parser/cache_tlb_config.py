"""
Copyright (c) 2020 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
"""

from cachedumplib import lookuptable as cache_table
from tlbdumplib import lookuptable as tlb_table
from cachedumplib import CacheDumpType_v3
from tlbdumplib import TlbDumpType_v4
from cachedumplib import LLC_SYSTEM_CACHE_KRYO3XX
from cachedumplib import L1_DCache_KRYO4XX_SILVER, L1_ICache_KRYO4XX_SILVER
from tlbdumplib import L2_TLB_KRYO4XX_SILVER

class L1_ICache_KRYO5XX_SILVER(CacheDumpType_v3):
    def __init__(self):
        super(L1_ICache_KRYO5XX_SILVER, self).__init__()
        self.cpu_name = "Kryo5Silver"
	self.NumSets = 0x80
        self.NumWays = 4
        self.NumTagRegs = 1
        self.RegSize = 4
        self.tlbsubcache = []
        self.datasubcache = []

    def parse_dump(self):
        tagfile_name = 'tag_scratch' + self.outfile.name[-4:]
        self.kryo_cache_parse("CACHEDUMP_CACHE_ID_L1_I_TAG", 0, tagfile_name)

        datafile_name = 'data_scratch' + self.outfile.name[-4:]
        """the input file is the dump for this ICACHE, and this is divided into
           two parts: the tag contents for all of the ICACHE, followed by the
           data contents for all of the ICACHE. As such, you must calculate the
           size of the tag content for the ICACHE to get the offset into the
           dump where the data contents start."""
	data_offset = 0
	if self.tlbsubcache:
	    for x in self.tlbsubcache:
	        data_offset = data_offset + (int(x[1]) * int(x[2]) * int(x[3]))
        data_offset = data_offset + self.NumWays * self.NumSets * self.RegSize *\
                      self.NumTagRegs
        self.kryo_cache_parse("CACHEDUMP_CACHE_ID_L1_I_DATA", data_offset,\
                              datafile_name)

class L1_DCache_KRYO5XX_SILVER(CacheDumpType_v3):
    def __init__(self):
        super(L1_DCache_KRYO5XX_SILVER, self).__init__()
	self.NumSets = 0x80
        self.NumWays = 4
        self.NumTagRegs = 2
        self.RegSize = 4
        self.cpu_name = "Kryo5Silver"
        self.tlbsubcache = []
        self.datasubcache = []

    def parse_dump(self):
        tagfile_name = 'tag_scratch' + self.outfile.name[-4:]
        self.kryo_cache_parse("CACHEDUMP_CACHE_ID_L1_D_TAG", 0, tagfile_name)

        datafile_name = 'data_scratch' + self.outfile.name[-4:]
        """the input file is the dump for this DCACHE, and this is divided into
           two parts: the tag contents for all of the DCACHE, followed by the
           data contents for all of the DCACHE. As such, you must calculate the
           size of the tag content for the DCACHE to get the offset into the
           dump where the data contents start."""
	data_offset = 0
	if self.tlbsubcache:
	    for x in self.tlbsubcache:
	        data_offset = data_offset + (int(x[1]) * int(x[2]) * int(x[3]))
        data_offset = data_offset + self.NumWays * self.NumSets * self.RegSize *\
                      self.NumTagRegs
        self.kryo_cache_parse("CACHEDUMP_CACHE_ID_L1_D_DATA", data_offset,\
                              datafile_name)

class L1_ICache_KRYO5XX_GOLD(CacheDumpType_v3):
    def __init__(self):
        super(L1_ICache_KRYO5XX_GOLD, self).__init__()
        self.cpu_name = "Kryo5Gold"
        self.NumSets = 0x100
        self.NumWays = 4
        self.NumTagRegs = 1
        self.RegSize = 8
	self.datasubcache = []
	self.datasubcache.append(["CACHEDUMP_CACHE_ID_L0_MOP", 1024, 1, 16])
        self.tlbsubcache = []
        self.datasubcache = []

    def parse_dump(self):
        tagfile_name = 'tag_scratch' + self.outfile.name[-4:]
        self.kryo_cache_parse("CACHEDUMP_CACHE_ID_L1_I_TAG", 0, tagfile_name)

        datafile_name = 'data_scratch' + self.outfile.name[-4:]
        """the input file is the dump for this ICACHE, and this is divided into
           two parts: the tag contents for all of the ICACHE, followed by the
           data contents for all of the ICACHE. As such, you must calculate the
           size of the tag content for the ICACHE to get the offset into the
           dump where the data contents start."""
	data_offset = 0
	if self.tlbsubcache:
	    for x in self.tlbsubcache:
	        data_offset = data_offset + (int(x[1]) * int(x[2]) * int(x[3]))
        data_offset = data_offset + self.NumWays * self.NumSets * self.RegSize *\
                      self.NumTagRegs
        self.kryo_cache_parse("CACHEDUMP_CACHE_ID_L1_I_DATA", data_offset,\
                              datafile_name)

class L1_DCache_KRYO5XX_GOLD(CacheDumpType_v3):
    def __init__(self):
        super(L1_DCache_KRYO5XX_GOLD, self).__init__()
        self.cpu_name = "Kryo5Gold"
        self.NumSets = 0x100
        self.NumWays = 4
        self.NumTagRegs = 1
        self.RegSize = 8
        self.tlbsubcache = []
        self.datasubcache = []

    def parse_dump(self):
        tagfile_name = 'tag_scratch' + self.outfile.name[-4:]
        self.kryo_cache_parse("CACHEDUMP_CACHE_ID_L1_D_TAG", 0, tagfile_name)

        datafile_name = 'data_scratch' + self.outfile.name[-4:]
        """the input file is the dump for this DCACHE, and this is divided into
           two parts: the tag contents for all of the DCACHE, followed by the
           data contents for all of the DCACHE. As such, you must calculate the
           size of the tag content for the DCACHE to get the offset into the
           dump where the data contents start."""
	data_offset = 0
	if self.tlbsubcache:
	    for x in self.tlbsubcache:
	        data_offset = data_offset + (int(x[1]) * int(x[2]) * int(x[3]))
        data_offset = data_offset + self.NumWays * self.NumSets * self.RegSize *\
                      self.NumTagRegs
        self.kryo_cache_parse("CACHEDUMP_CACHE_ID_L1_D_DATA", data_offset,\
                              datafile_name)

class L2_Cache_KRYO5XX_GOLD(CacheDumpType_v3):
    def __init__(self, numsets):
        super(L2_Cache_KRYO5XX_GOLD, self).__init__()
        self.cpu_name = "Kryo5Gold"
        self.NumSets = numsets
        self.NumWays = 8
        self.NumTagRegs = 1
        self.RegSize = 8
        self.tlbsubcache = []
        self.datasubcache = []

    def parse_dump(self):
        tagfile_name = 'tag_scratch' + self.outfile.name[-4:]

        self.kryo_cache_parse("CACHEDUMP_CACHE_ID_L2_TAG", 0, tagfile_name)

        datafile_name = 'data_scratch' + self.outfile.name[-4:]
        """the input file is the dump for the cache, and this is divided into
           two parts: the tag contents for all of the cache, followed by the
           data contents for all of the cache. As such, you must calculate the
           size of the tag content for the cache to get the offset into the
           dump where the data contents start."""
	data_offset = 0
	if self.tlbsubcache:
	    for x in self.tlbsubcache:
	        data_offset = data_offset + (int(x[1]) * int(x[2]) * int(x[3]))
        data_offset = data_offset + self.NumWays * self.NumSets * self.RegSize *\
                      self.NumTagRegs
        self.kryo_cache_parse("CACHEDUMP_CACHE_ID_L2_DATA", data_offset,\
                              datafile_name)

class L1_ITLB_KRYO5XX_GOLD(TlbDumpType_v4):
    def __init__(self):
        super(L1_ITLB_KRYO5XX_GOLD, self).__init__()
        #name must match expected name from kryo tlb parser
        self.cpu_name = "Kryo5Gold"
        self.NumWays = 1
	self.NumSets = 48
        self.tlbsubcache = []
        self.datasubcache = []

    def parse_dump(self):
        datafile_name = "data_scratch" + self.outfile.name[-4:]
        self.kryo_tlb_parse("CACHEDUMP_CACHE_ID_L1_I_TLB_DATA", 0,\
                            datafile_name)

class L1_DTLB_KRYO5XX_GOLD(TlbDumpType_v4):
    def __init__(self):
        super(L1_DTLB_KRYO5XX_GOLD, self).__init__()
        #name must match expected name from kryo tlb parser
        self.cpu_name = "Kryo5Gold"
        self.NumWays = 1
        self.NumSets = 48
        self.tlbsubcache = []
        self.datasubcache = []

    def parse_dump(self):
        datafile_name = "data_scratch" + self.outfile.name[-4:]
        self.kryo_tlb_parse("CACHEDUMP_CACHE_ID_L1_D_TLB_DATA", 0,\
                            datafile_name)

class L2_TLB_KRYO5XX_GOLD(TlbDumpType_v4):
    def __init__(self):
        super(L2_TLB_KRYO5XX_GOLD, self).__init__()
        #name must match expected name from kryo tlb parser
        self.cpu_name = "Kryo5Gold"
        #num sets and num ways can be found in json/ directory within this
        #cpu's json file
        self.NumSets = 256
        self.NumWays = 5
        self.tlbsubcache = []
        self.datasubcache = []

    def parse_dump(self):
        datafile_name = "data_scratch" + self.outfile.name[-4:]
        self.kryo_tlb_parse("CACHEDUMP_CACHE_ID_L2_TLB_DATA", 0, datafile_name)

class L2_TLB_KRYO5XX_SILVER(TlbDumpType_v4):
    def __init__(self):
        super(L2_TLB_KRYO5XX_SILVER, self).__init__()
        self.cpu_name = "Kryo5Silver"
	#refer to section A5.2.2 in TRM
        self.NumWays = 4
        self.NumSets = 0x100
        #refer to new src for dumping tag data to see number of tag entries
        self.NumTagRegs = 3
        #refer to new src for dumping tag data to see size. Use bytes
        self.RegSize = 4
        self.tlbsubcache = []
        self.datasubcache = []
	#represet subcache in this form ['name', set, ways, blocksize]
	self.tlbsubcache.append(["CACHEDUMP_CACHE_ID_L2_TLB_TAG_WALK",16, 4, 12])
	self.tlbsubcache.append([ "CACHEDUMP_CACHE_ID_L2_TLB_TAG_IPA",16 , 4, 12])
	self.datasubcache.append(["CACHEDUMP_CACHE_ID_L2_TLB_DATA_WALK", 16, 4, 8])
	self.datasubcache.append(["CACHEDUMP_CACHE_ID_L2_TLB_DATA_IPA",16, 4, 8])

    def parse_dump(self):
        tagfile_name = "tag_scratch" + self.outfile.name[-4:]
        self.kryo_tlb_parse("CACHEDUMP_CACHE_ID_L2_TLB_TAG", 0, tagfile_name)

        datafile_name = "data_scratch" + self.outfile.name[-4:]
        """the input file is the dump for this TLB, and this is divided into
           two parts: the tag contents for all of the TLB, followed by the data
           contents for all of the TLB. As such, you must calculate the size of
           the tag content for the TLB to get the offset into the dump where the
           data contents start."""
	data_offset = 0
	if self.tlbsubcache:
	    for x in self.tlbsubcache:
	        data_offset = data_offset + (int(x[1]) * int(x[2]) * int(x[3]))
        data_offset = data_offset + self.NumWays * self.NumSets * self.RegSize *\
                      self.NumTagRegs
        self.kryo_tlb_parse("CACHEDUMP_CACHE_ID_L2_TLB_DATA", data_offset,\
                            datafile_name)

# "kona"
cache_table[("kona", 0x80, 0x14)] = L1_DCache_KRYO5XX_SILVER()
cache_table[("kona", 0x81, 0x14)] = L1_DCache_KRYO5XX_SILVER()
cache_table[("kona", 0x82, 0x14)] = L1_DCache_KRYO5XX_SILVER()
cache_table[("kona", 0x83, 0x14)] = L1_DCache_KRYO5XX_SILVER()
cache_table[("kona", 0x84, 0x14)] = L1_DCache_KRYO5XX_GOLD()
cache_table[("kona", 0x85, 0x14)] = L1_DCache_KRYO5XX_GOLD()
cache_table[("kona", 0x86, 0x14)] = L1_DCache_KRYO5XX_GOLD()
cache_table[("kona", 0x87, 0x14)] = L1_DCache_KRYO5XX_GOLD()
cache_table[("kona", 0x60, 0x14)] = L1_ICache_KRYO5XX_SILVER()
cache_table[("kona", 0x61, 0x14)] = L1_ICache_KRYO5XX_SILVER()
cache_table[("kona", 0x62, 0x14)] = L1_ICache_KRYO5XX_SILVER()
cache_table[("kona", 0x63, 0x14)] = L1_ICache_KRYO5XX_SILVER()
cache_table[("kona", 0x64, 0x14)] = L1_ICache_KRYO5XX_GOLD()
cache_table[("kona", 0x65, 0x14)] = L1_ICache_KRYO5XX_GOLD()
cache_table[("kona", 0x66, 0x14)] = L1_ICache_KRYO5XX_GOLD()
cache_table[("kona", 0x67, 0x14)] = L1_ICache_KRYO5XX_GOLD()
cache_table[("kona", 0x140, 0x10)] = LLC_SYSTEM_CACHE_KRYO3XX()
cache_table[("kona", 0x141, 0x10)] = LLC_SYSTEM_CACHE_KRYO3XX()
cache_table[("kona", 0x142, 0x10)] = LLC_SYSTEM_CACHE_KRYO3XX()
cache_table[("kona", 0x143, 0x10)] = LLC_SYSTEM_CACHE_KRYO3XX()
cache_table[("kona", 0xc4, 0x10)] = L2_Cache_KRYO5XX_GOLD(numsets=0x200)
cache_table[("kona", 0xc5, 0x10)] = L2_Cache_KRYO5XX_GOLD(numsets=0x200)
cache_table[("kona", 0xc6, 0x10)] = L2_Cache_KRYO5XX_GOLD(numsets=0x200)
cache_table[("kona", 0xc7, 0x10)] = L2_Cache_KRYO5XX_GOLD(numsets=0x400)

#kona
tlb_table[("kona", 0x24, 0x14)] = L1_ITLB_KRYO5XX_GOLD()
tlb_table[("kona", 0x25, 0x14)] = L1_ITLB_KRYO5XX_GOLD()
tlb_table[("kona", 0x26, 0x14)] = L1_ITLB_KRYO5XX_GOLD()
tlb_table[("kona", 0x27, 0x14)] = L1_ITLB_KRYO5XX_GOLD()
tlb_table[("kona", 0x44, 0x14)] = L1_DTLB_KRYO5XX_GOLD()
tlb_table[("kona", 0x45, 0x14)] = L1_DTLB_KRYO5XX_GOLD()
tlb_table[("kona", 0x46, 0x14)] = L1_DTLB_KRYO5XX_GOLD()
tlb_table[("kona", 0x47, 0x14)] = L1_DTLB_KRYO5XX_GOLD()
tlb_table[("kona", 0x120, 0x14)] = L2_TLB_KRYO5XX_SILVER()
tlb_table[("kona", 0x121, 0x14)] = L2_TLB_KRYO5XX_SILVER()
tlb_table[("kona", 0x122, 0x14)] = L2_TLB_KRYO5XX_SILVER()
tlb_table[("kona", 0x123, 0x14)] = L2_TLB_KRYO5XX_SILVER()
tlb_table[("kona", 0x124, 0x14)] = L2_TLB_KRYO5XX_GOLD()
tlb_table[("kona", 0x125, 0x14)] = L2_TLB_KRYO5XX_GOLD()
tlb_table[("kona", 0x126, 0x14)] = L2_TLB_KRYO5XX_GOLD()
tlb_table[("kona", 0x127, 0x14)] = L2_TLB_KRYO5XX_GOLD()

# "lagoon"
cache_table[("lagoon", 0x80, 0x14)] = L1_DCache_KRYO4XX_SILVER()
cache_table[("lagoon", 0x81, 0x14)] = L1_DCache_KRYO4XX_SILVER()
cache_table[("lagoon", 0x82, 0x14)] = L1_DCache_KRYO4XX_SILVER()
cache_table[("lagoon", 0x83, 0x14)] = L1_DCache_KRYO4XX_SILVER()
cache_table[("lagoon", 0x84, 0x14)] = L1_DCache_KRYO4XX_SILVER()
cache_table[("lagoon", 0x85, 0x14)] = L1_DCache_KRYO4XX_SILVER()
cache_table[("lagoon", 0x86, 0x14)] = L1_DCache_KRYO5XX_GOLD()
cache_table[("lagoon", 0x87, 0x14)] = L1_DCache_KRYO5XX_GOLD()
cache_table[("lagoon", 0x60, 0x14)] = L1_ICache_KRYO4XX_SILVER()
cache_table[("lagoon", 0x61, 0x14)] = L1_ICache_KRYO4XX_SILVER()
cache_table[("lagoon", 0x62, 0x14)] = L1_ICache_KRYO4XX_SILVER()
cache_table[("lagoon", 0x63, 0x14)] = L1_ICache_KRYO4XX_SILVER()
cache_table[("lagoon", 0x64, 0x14)] = L1_ICache_KRYO4XX_SILVER()
cache_table[("lagoon", 0x65, 0x14)] = L1_ICache_KRYO4XX_SILVER()
cache_table[("lagoon", 0x66, 0x14)] = L1_ICache_KRYO5XX_GOLD()
cache_table[("lagoon", 0x67, 0x14)] = L1_ICache_KRYO5XX_GOLD()
cache_table[("lagoon", 0x140, 0x10)] = LLC_SYSTEM_CACHE_KRYO3XX()
cache_table[("lagoon", 0x141, 0x10)] = LLC_SYSTEM_CACHE_KRYO3XX()
cache_table[("lagoon", 0xc6, 0x10)] = L2_Cache_KRYO5XX_GOLD(numsets=0x200)
cache_table[("lagoon", 0xc7, 0x10)] = L2_Cache_KRYO5XX_GOLD(numsets=0x200)

#lagoon
tlb_table[("lagoon", 0x26, 0x14)] = L1_ITLB_KRYO5XX_GOLD()
tlb_table[("lagoon", 0x27, 0x14)] = L1_ITLB_KRYO5XX_GOLD()
tlb_table[("lagoon", 0x46, 0x14)] = L1_DTLB_KRYO5XX_GOLD()
tlb_table[("lagoon", 0x47, 0x14)] = L1_DTLB_KRYO5XX_GOLD()
tlb_table[("lagoon", 0x120, 0x14)] = L2_TLB_KRYO4XX_SILVER()
tlb_table[("lagoon", 0x121, 0x14)] = L2_TLB_KRYO4XX_SILVER()
tlb_table[("lagoon", 0x122, 0x14)] = L2_TLB_KRYO4XX_SILVER()
tlb_table[("lagoon", 0x123, 0x14)] = L2_TLB_KRYO4XX_SILVER()
tlb_table[("lagoon", 0x124, 0x14)] = L2_TLB_KRYO4XX_SILVER()
tlb_table[("lagoon", 0x125, 0x14)] = L2_TLB_KRYO4XX_SILVER()
tlb_table[("lagoon", 0x126, 0x14)] = L2_TLB_KRYO5XX_GOLD()
tlb_table[("lagoon", 0x127, 0x14)] = L2_TLB_KRYO5XX_GOLD()
