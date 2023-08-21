###############################################################################################################################
# Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
###############################################################################################################################

# This script takes either one or 2 input log files, parses and generates memory stats comparison between 2 runs. It also
# generates stats information at each port along with overall memory usage stats.
# usage : get_memory_stats.py <output_file_name_without_extension> <output_sheet_name> <log_file1> <log_file1>
# or
# usage : get_memory_stats.py <output_file_name_without_extension> <output_sheet_name> <log_file>

# exa   : get_memory_stats.py preview preview memstats_build1.txt memstats_build2.txt
# exa   : get_memory_stats.py preview preview memstats_build1.txt
# exa   : get_memory_stats.py preview preview memstats_build2.txt

# override settings required: (do not enable unnecessary log masks, it may cause log drops)
# logInfoMask=32
# enableMemoryStats=TRUE

# Note - Make sure log file contains logs related to only one use case, i.e PrintMemoryPoolManagerStats logs should be there only once.
# or log should contain something like this: (there should be only one set like below), edit the log if required before running the script
#       .......
#       MPM : Number of groups is 0, register first buffer manager
#       .......
#       .......
#       MPM : Number of groups is 0 now, unregistered last buffer manager
#       .......

import csv
import re
import sys
import xlsxwriter
import os

# Get input args
files = sys.argv[1:]

# 1st arg is ouput filename
outputfile = files[0]

# 2nd arg is ouput sheet name
outputsheet = files[1]

# 3rd arg is input log1 filename
inputfile1 = files[2]

# 4th arg is input log2 filename
if len(files) > 3:
	inputfile2 = files[3]
	comparison = 1
else:
	comparison = 0

# --------------------------------------------------------Log1,2 start------------------------------------------------
#code :
#        CAMX_LOG_INFO(CamxLogGroupMemMgr,
#                      "[%s]-->[%s] : Stats : CreateData : (type=%s, Immediate=%d, Max=%d), "
#                      "Actual Peak=%d, sizeRequired=%zu (allocated in range of %zu to %zu), activated=%d",
#                      m_pGroupName, pMemPoolBufMgr->name,
#                      (BufferManagerType::CamxBufferManager == pMemPoolBufMgr->createData.bufferManagerType) ? "CamX" : "CHI",
#                      pMemPoolBufMgr->createData.immediateAllocBufferCount,
#                      pMemPoolBufMgr->createData.maxBufferCount,
#                      pMemPoolBufMgr->peakBuffersUsed,
#                      pMemPoolBufMgr->sizeRequired,
#                      m_leastBufferSizeAllocated,
#                      m_maxBufferSizeAllocated,
#                      pMemPoolBufMgr->bEverActivated);
#log exa : 09-03 21:41:30.739   733   733 I CamX    : [ INFO][MEMMGR ] camxmempoolgroup.cpp:537 UnregisterBufferManager() [MPG_92]-->[MCFusionBufferManager] : Stats : CreateData : (type=CHI, Immediate=1, Max=2), Actual Peak=1, sizeRequired=19869696 (allocated in range of 19869696 to 19869696), activated=1

UnregisterPattern = r'^.*\[ INFO\]\[MEMMGR \] camxmempoolgroup\.cpp:\d+ .*\(\) \[(.*)\]-->\[(.*)\] : Stats : CreateData : \(type=(.*), Immediate=(.*), Max=(.*)\), Actual Peak=(.*), sizeRequired=(.*) \(allocated in range of (.*) to (.*)\), activated=(.*)'
UnregisterProg = re.compile(UnregisterPattern)

#log exa : 01-04 13:54:05.119   766 10859 I CamX    : [ INFO][MEMMGR ] camxmempoolgroup.cpp:438 RegisterBufferManager() [MPG_3]-->[FastAECRealtime_IFE0_OutputPortId21_StatsAWBBG] : NumBufMgrs=1, type=CamX, heap=1, Original Immediate=6, Max=10, SelfTuned Immediate=6, Max=10, needDedicatedBuffers=1, lateBinding=0, numBatched=1, deviceCount=1, devices=0, 0, flags=0x19 width=1382400, height=1, format=6, size=1382400

RegisterPattern = r'^.*\[MEMMGR \] camxmempoolgroup\.cpp:\d+ .*RegisterBufferManager\(\) \[(.*)\]-->\[(.*)\] : NumBufMgrs=(.*), type=(.*), heap=(.*), Original Immediate=(.*), Max=(.*), SelfTuned Immediate=(.*), Max=(.*), needDedicatedBuffers=(.*), lateBinding=(.*), numBatched=(.*), deviceCount=(.*), devices=(.*), (.*), flags=(.*) width=(.*), height=(.*), format=(.*), size=(.*)'
RegisterProg = re.compile(RegisterPattern)


# Column names for UnregisterBufferManager log
ColNameBufferManager = ['BufferMangerName']
UnregisterColNames   = ['Group',
                        'BufferMangerType',
                        'ImmediateCount',
                        'MaxCount',
                        'ActualPeak',
                        'sizeRequired',
                        'SizeRangeMin',
                        'SizeRangeMax',
                        'Activated']
UnregisterIndexGroup            = 0
UnregisterIndexBufferMangerType = 1
UnregisterIndexImmediateCount   = 2
UnregisterIndexMaxCount         = 3
UnregisterIndexActualPeak       = 4
UnregisterIndexsizeRequired     = 5
UnregisterIndexSizeRangeMin     = 6
UnregisterIndexSizeRangeMax     = 7
UnregisterIndexActivated        = 8

UnregisterNumProps = 9
UnregisterEmptyList = [0 for i in range(UnregisterNumProps)]

bufferManagerName = []
UnregisterValues1 = [[] for _ in xrange(UnregisterNumProps)]
UnregisterValues2 = [[] for _ in xrange(UnregisterNumProps)]

RegisterColNames   = ['Group',
                      'NumBufMgrs',
                      'Type',
                      'Heap',
                      'OrigImmediate',
                      'OrigMaxCount',
                      'SelfTunedImmediate',
                      'SelfTunedMaxCount',
                      'Dedicated',
                      'LateBinding',
                      'Batch',
                      'DeviceCount',
                      'Device0',
                      'Device1',
                      'flags',
                      'width',
                      'height',
                      'format',
                      'size']
RegisterIndexGroup              = 0
RegisterIndexNumBufMgrs         = 1
RegisterIndexType               = 2
RegisterIndexHeap               = 3
RegisterIndexOrigImmediate      = 4
RegisterIndexOrigMaxCount       = 5
RegisterIndexSelfTunedImmediate = 6
RegisterIndexSelfTunedMaxCount  = 7
RegisterIndexDedicated          = 8
RegisterIndexLateBinding        = 9
RegisterIndexBatch              = 10
RegisterIndexDeviceCount        = 11
RegisterIndexDevice0            = 12
RegisterIndexDevice1            = 13
RegisterIndexFlags              = 14
RegisterIndexWidth              = 15
RegisterIndexHeight             = 16
RegisterIndexFormat             = 17
RegisterIndexSize               = 18

RegisterNumProps = 19
RegisterEmptyList = [0 for i in range(RegisterNumProps)]

RegisterValues1 = [[] for _ in xrange(RegisterNumProps)]
RegisterValues2 = [[] for _ in xrange(RegisterNumProps)]


all_keys1 = []
all_keys2 = []

def parse_log(filename, prog, keyss):
	with open(filename, 'r') as f:
		result_dict = {}
		for line in f:
			res = prog.match(line)
			if res is not None:
				matches = res.groups()
				keyss.append(matches[1])
				result_dict[matches[1]] = matches[0:1] + matches[2:]
				#print(matches)
		return result_dict

UnregisterDictResult1 = parse_log(inputfile1, UnregisterProg, all_keys1)
RegisterDictResult1 = parse_log(inputfile1, RegisterProg, all_keys2)

if comparison==1:
	UnregisterDictResult2 = parse_log(inputfile2, UnregisterProg, all_keys2)
	RegisterDictResult2 = parse_log(inputfile2, RegisterProg, all_keys2)
else:
	UnregisterDictResult2 = {}
	RegisterDictResult2 = {}

def write_to_csv(cslfilename, f1_register, f1_unregister, f2_register, f2_unregister):
	with open(cslfilename, 'wb') as csvfile:
		csvwriter = csv.writer(csvfile)
		header = ColNameBufferManager
		if f1_register==1:
			header += RegisterColNames
		if f1_unregister==1:
			header += UnregisterColNames
		if f2_register==1:
			header += RegisterColNames
		if f2_unregister==1:
			header += UnregisterColNames
		csvwriter.writerow(header)
		for k in all_keys1:
			RegisterResult1 = list(RegisterDictResult1.get(k, RegisterEmptyList))
			UnregisterResult1 = list(UnregisterDictResult1.get(k, UnregisterEmptyList))
			RegisterResult2 = list(RegisterDictResult2.get(k, RegisterEmptyList))
			UnregisterResult2 = list(UnregisterDictResult2.get(k, UnregisterEmptyList))
			row = [k]
			if f1_register==1:
				row += RegisterResult1
			if f1_unregister==1:
				row += UnregisterResult1
			if f2_register==1:
				row += RegisterResult2
			if f2_unregister==1:
				row += UnregisterResult2
			#print(row)
			csvwriter.writerow(row)

if comparison==1:
	write_to_csv(outputsheet+'.csv', 1, 1, 1, 1)
else:
	write_to_csv(outputsheet+'.csv', 1, 1, 0, 0)
# --------------------------------------------------------Log1 end------------------------------------------------

# --------------------------------------------------------Log3 start------------------------------------------------
# code :
#        CAMX_LOG_INFO(CamxLogGroupMemMgr, "MemPoolMgrStats : Allocation : num=%d, peakNum=%d, size=%zu, peakSize=%zu",
#                      s_memPoolMgrStats.numBuffersAllocated,   s_memPoolMgrStats.peakNumBuffersAllocated,
#                      s_memPoolMgrStats.sizeOfMemoryAllocated, s_memPoolMgrStats.peakSizeOfMemoryAllocated);
# log exa : 09-03 21:41:30.739   733   733 I CamX    : [ INFO][MEMMGR ] camxmempoolgroup.cpp:1710 PrintMemoryPoolManagerStats() MemPoolMgrStats : Allocation : num=0, peakNum=114, size=0, peakSize=461781360
pattern2 = r'^.*\[ INFO\]\[MEMMGR \] camxmempoolgroup\.cpp:\d+ .*PrintMemoryPoolManagerStats\(\) MemPoolMgrStats : Allocation : num=(.*), peakNum=(.*), size=(.*), peakSize=(.*)'
prog2 = re.compile(pattern2)

MemStatsEmptyList = [0 for i in range(4)]


def parse_log2(filename):
	with open(filename, 'r') as f:
		result_dict = {}
		for line in f:
			res = prog2.match(line)
			if res is not None:
				matches = res.groups()
				result_dict['dummykey'] = matches
		return result_dict

stats1 = parse_log2(inputfile1)

if comparison==1:
	stats2 = parse_log2(inputfile2)
else:
	stats2 = {}


all_keys_stats = set(stats1.keys()) | set(stats2.keys())

with open('temp_stats_summary.csv', 'wb') as csvfile:
	csvwriter = csv.writer(csvfile)
	for k in all_keys_stats:
		l1 = list(stats1.get(k, MemStatsEmptyList))
		l2 = list(stats2.get(k, MemStatsEmptyList))
		LogPeakNum1 = l1[1]
		LogPeakSize1 = l1[3]
		LogPeakNum2 = l2[1]
		LogPeakSize2 = l2[3]
		csvwriter.writerow(l1 + l2)
		#print(l1 + l2)

# --------------------------------------------------------Log3 end------------------------------------------------

# --------------------------------------------------------Log4 start------------------------------------------------
# code :
#        CAMX_LOG_INFO(CamxLogGroupMemMgr,
#			"MemPoolGroup[%s][%p] : Buffers : Allocated=%d, Inuse=%d, Free=%d PeakAllocated=%d, PeakUsed=%d",
#                     m_pGroupName,                 this,
#                     m_numBuffersAllocated,        (m_numBuffersAllocated - m_freeBufferList.NumNodes()),
#                     m_freeBufferList.NumNodes(),  m_peakNumBuffersAllocated,
#                     m_peakNumBuffersUsed);
# log exa : 01-04 13:55:24.621   768  6276 I CamX    : [ INFO][MEMMGR ] camxmempoolgroup.cpp:87 ~MemPoolGroup() MemPoolGroup[MPG_0][0xeb07e800] : Buffers : Allocated=0, Inuse=0, Free=0 PeakAllocated=10, PeakUsed=9
pattern4 = r'^.*\[ INFO\]\[MEMMGR \] camxmempoolgroup\.cpp:\d+ .*~MemPoolGroup\(\) MemPoolGroup\[(.*)\]\[.*\] : Buffers : Allocated=.*, Inuse=.*, Free=.* PeakAllocated=(.*), PeakUsed=(.*)'
prog4 = re.compile(pattern4)

MemPoolGroupDestroyEmptyList = [0 for i in range(2)]
MemPoolGroupDestroyIndexPeakAllocated  = 0
MemPoolGroupDestroyIndexPeakUsed       = 1

mempoolkeyss = []

def parse_log4(filename, prog):
	with open(filename, 'r') as f:
		result_dict = {}
		for line in f:
			res = prog.match(line)
			if res is not None:
				matches = res.groups()
				mempoolkeyss.append(matches[0])
				result_dict[matches[0]] = matches[1:]
				#print(result_dict[matches[0]])
		return result_dict

MemPoolGroupDict1 = parse_log4(inputfile1, prog4)

if comparison==1:
	MemPoolGroupDict2 = parse_log4(inputfile2, prog4)
else:
	MemPoolGroupDict2 = {}


# --------------------------------------------------------Log4 end------------------------------------------------




# --------------------------------------------------------generate xlsx summary------------------------------------------
with open(outputsheet+'.csv', 'rb') as csvfile:
    reader = csv.reader(csvfile, delimiter=',')
    reader.next() # skip header
    for row in reader:
		offset = 0
		bufferManagerName.append(row[0])
		offset = 1
		for i in range(RegisterNumProps):
			RegisterValues1[i].append(row[offset+i])
		offset += RegisterNumProps
		for i in range(UnregisterNumProps):
			UnregisterValues1[i].append(row[offset+i])
		offset += UnregisterNumProps
		if comparison==1:
			for i in range(RegisterNumProps):
				RegisterValues2[i].append(row[offset+i])
			offset+= RegisterNumProps
			for i in range(UnregisterNumProps):
				UnregisterValues2[i].append(row[offset+i])
			offset+= UnregisterNumProps


# Create an new Excel file and add a worksheet.
workbook = xlsxwriter.Workbook(outputfile+'.xlsx')
#workbook = xlsxwriter.Workbook('temp1.xlsx')
worksheet = workbook.add_worksheet(outputfile)

cell_format = workbook.add_format()
cell_format.set_text_wrap()
cell_format.set_bold()
cell_format.set_border(5)

cell_format_bold = workbook.add_format()
cell_format_bold.set_bold()
cell_format_bold.set_border(5)
cell_format_bold.set_text_wrap()

merge_format = workbook.add_format({'align': 'center'})
merge_format.set_bold()
merge_format.set_border(5)

if comparison==1:
	worksheet.merge_range('F2:K2', 'Memory Stats '+inputfile1, merge_format)
	worksheet.merge_range('L2:Q2', 'Memory Stats '+inputfile2, merge_format)
	worksheet.merge_range('R2:S2', 'Difference', merge_format)
	worksheet.merge_range('T2:AJ2', 'Log1 CreateParams', merge_format)
else:
	worksheet.merge_range('F2:K2', 'Memory Stats', merge_format)
	worksheet.merge_range('L2:AB2', 'CreateParams', merge_format)

row = 0
worksheet.write_row(row, 1, [' '])
row += 1
worksheet.write_row(row, 1, [' '])
row += 1
if comparison==1:
	worksheet.write_row(row, 1, ['S.No', 'BufferManagerName', 'Type', 'SizeRequired', 'Group', 'PeakAllocated\nInGroup', 'PeakUsed\nInGroup', \
		'SizeAllocated\nPerBuffer', 'PeakUsed\nAtPort', 'TotalSize', \
		'Group', 'PeakAllocated\nInGroup', 'PeakUsed\nInGroup', 'SizeAllocated\nPerBuffer', 'PeakUsed\nAtPort', 'TotalSize', 'NumOfBuffers', 'TotalSize'], cell_format_bold)
	columnOffset = 19
else:
	worksheet.write_row(row, 1, ['S.No', 'BufferManagerName', 'Type', 'SizeRequired', 'Group', 'PeakAllocated\nInGroup', 'PeakUsed\nInGroup', \
		'SizeAllocated', 'PeakUsed\nAtPort', 'TotalSize'], cell_format_bold)
	columnOffset = 11

worksheet.write_row(row, columnOffset, ['Type', 'Heap', 'Orig\nImmediateCount', 'Orig\nMaxCount', 'SekfTune\nImmediate', 'SelfTune\nMaxCount', 'Dedicated', 'Late\nBinding', 'Batch', \
	'Device\nCount', 'Device0', 'Device1', 'Flags', 'Width', 'Height', 'Format', 'Size'], cell_format_bold)
row += 1
OveralPeak1 = 0
OveralPeak2 = 0
OveralSize1 = 0
OveralSize2 = 0
OveralSavings = 0
SizeAllocated2 = 0
TotalSize2 = 0
Savings = 0
MemPoolGroupPeakAllocated1 = 0
MemPoolGroupPeakUsed1 = 0
MemPoolGroupPeakAllocated2 = 0
MemPoolGroupPeakUsed2 = 0

for i,x in enumerate(bufferManagerName):
	SizeAllocated1 = max(int(UnregisterValues1[UnregisterIndexsizeRequired][i]), \
						int(UnregisterValues1[UnregisterIndexSizeRangeMin][i]), \
						int(UnregisterValues1[UnregisterIndexSizeRangeMax][i]))
	TotalSize1 = int(UnregisterValues1[UnregisterIndexActualPeak][i])*SizeAllocated1
	OveralPeak1 += int(UnregisterValues1[UnregisterIndexActualPeak][i])
	OveralSize1 += TotalSize1
	if comparison==1:
		SizeAllocated2 = max(int(UnregisterValues2[UnregisterIndexsizeRequired][i]), \
							int(UnregisterValues2[UnregisterIndexSizeRangeMin][i]), \
							int(UnregisterValues2[UnregisterIndexSizeRangeMax][i]))
		TotalSize2 = int(UnregisterValues2[UnregisterIndexActualPeak][i])*SizeAllocated2
		OveralPeak2 += int(UnregisterValues2[UnregisterIndexActualPeak][i])
		OveralSize2 += TotalSize2
		Savings = TotalSize1-TotalSize2
		OveralSavings += Savings

	columnOffset = 1
	worksheet.write_row(row, columnOffset, \
		[i+1, \
		bufferManagerName[i], \
		UnregisterValues1[UnregisterIndexBufferMangerType][i], \
		int(UnregisterValues1[UnregisterIndexsizeRequired][i])])
	columnOffset += 4

	MemPoolGroupDictList1 = list(MemPoolGroupDict1.get(UnregisterValues1[UnregisterIndexGroup][i], MemStatsEmptyList))
	MemPoolGroupPeakAllocated1 += int(MemPoolGroupDictList1[MemPoolGroupDestroyIndexPeakAllocated])
	MemPoolGroupPeakUsed1 += int(MemPoolGroupDictList1[MemPoolGroupDestroyIndexPeakUsed])

	worksheet.write_row(row, columnOffset, \
		[UnregisterValues1[UnregisterIndexGroup][i], \
		int(MemPoolGroupDictList1[MemPoolGroupDestroyIndexPeakAllocated]), \
		int(MemPoolGroupDictList1[MemPoolGroupDestroyIndexPeakUsed]), \
		SizeAllocated1, \
		int(UnregisterValues1[UnregisterIndexActualPeak][i]), \
		TotalSize1])
	columnOffset += 6

	if comparison==1:
		MemPoolGroupDictList2 = list(MemPoolGroupDict2.get(UnregisterValues2[UnregisterIndexGroup][i], MemStatsEmptyList))
		MemPoolGroupPeakAllocated2 += int(MemPoolGroupDictList2[MemPoolGroupDestroyIndexPeakAllocated])
		MemPoolGroupPeakUsed2 += int(MemPoolGroupDictList2[MemPoolGroupDestroyIndexPeakUsed])
		worksheet.write_row(row, columnOffset, \
			[UnregisterValues2[UnregisterIndexGroup][i], \
			int(MemPoolGroupDictList2[MemPoolGroupDestroyIndexPeakAllocated]), \
			int(MemPoolGroupDictList2[MemPoolGroupDestroyIndexPeakUsed]), \
			SizeAllocated2, \
			int(UnregisterValues2[UnregisterIndexActualPeak][i]), \
			TotalSize2, \
			int(UnregisterValues1[UnregisterIndexActualPeak][i])-int(UnregisterValues2[UnregisterIndexActualPeak][i]), \
			Savings])
		columnOffset += 8

	worksheet.write_row(row, columnOffset, \
		[UnregisterValues1[UnregisterIndexBufferMangerType][i], \
		int(RegisterValues1[RegisterIndexHeap][i]), \
		int(RegisterValues1[RegisterIndexOrigImmediate][i]), \
		int(RegisterValues1[RegisterIndexOrigMaxCount][i]), \
		int(RegisterValues1[RegisterIndexSelfTunedImmediate][i]), \
		int(RegisterValues1[RegisterIndexSelfTunedMaxCount][i]), \
		int(RegisterValues1[RegisterIndexDedicated][i]), \
		int(RegisterValues1[RegisterIndexLateBinding][i]), \
		int(RegisterValues1[RegisterIndexBatch][i]), \
		int(RegisterValues1[RegisterIndexDeviceCount][i]), \
		int(RegisterValues1[RegisterIndexDevice0][i]), \
		int(RegisterValues1[RegisterIndexDevice1][i]), \
		RegisterValues1[RegisterIndexFlags][i], \
		int(RegisterValues1[RegisterIndexWidth][i]), \
		int(RegisterValues1[RegisterIndexHeight][i]), \
		int(RegisterValues1[RegisterIndexFormat][i]), \
		int(RegisterValues1[RegisterIndexSize][i])])
	row += 1

worksheet.write_row(row, 1, [' '])
row += 1
if comparison==1:
	worksheet.write_row(row, 1, ['', 'Summary from above', '', '', '', MemPoolGroupPeakAllocated1, MemPoolGroupPeakUsed1, '', OveralPeak1, OveralSize1, \
		'', MemPoolGroupPeakAllocated2, MemPoolGroupPeakUsed2, '', OveralPeak2, OveralSize2, OveralPeak1-OveralPeak2, OveralSavings], cell_format_bold)
else:
	worksheet.write_row(row, 1, ['', 'Summary from above', '', '', '', MemPoolGroupPeakAllocated1, MemPoolGroupPeakUsed1, '', OveralPeak1, OveralSize1], cell_format_bold)

row += 1
if comparison==1:
	worksheet.write_row(row, 1, ['', '', '', '', '', '', '', '', 'Peak\nNumberOf\nAllocations', 'Peak\nSize\nAllocated', \
		'', '', '', '', 'Peak\nNumberOf\nAllocations', 'Peak\nSize\nAllocated', 'MemoryDiff\nBuffers', 'MemoryDiff\nSize'], cell_format)
else:
	worksheet.write_row(row, 1, ['', '', '', '', '', '', '', '', 'Peak\nNumberOf\nAllocations', 'Peak\nSize\nAllocated'], cell_format)
row += 1
if comparison==1:
	worksheet.write_row(row, 1, ['', 'Summary from log (code)', '', '', '', '', '', '', int(LogPeakNum1), int(LogPeakSize1), \
		'', '', '', '', int(LogPeakNum2), int(LogPeakSize2), int(LogPeakNum1)-int(LogPeakNum2), int(LogPeakSize1)-int(LogPeakSize2)], cell_format_bold)
else:
	worksheet.write_row(row, 1, ['', 'Summary from log (code)', '', '', '', '', '', '', int(LogPeakNum1), int(LogPeakSize1)], cell_format_bold)
row += 1

format_border_left = workbook.add_format()
format_border_left.set_border(1)
format_border_left.set_left(5)

format_border_right = workbook.add_format()
format_border_right.set_border(1)
format_border_right.set_right(5)

format_border = workbook.add_format()
format_border.set_border(1)

worksheet.set_column('A:A', 4)
worksheet.set_column('B:B', 5, format_border_left)
worksheet.set_column('C:C', 70, format_border)
worksheet.set_column('D:D', 5, format_border)
worksheet.set_column('E:E', 12, format_border_right)
worksheet.set_column('F:F', 8, format_border)
worksheet.set_column('G:G', 8, format_border)
worksheet.set_column('H:H', 8, format_border)
worksheet.set_column('I:I', 12, format_border)
worksheet.set_column('J:J', 10, format_border)
worksheet.set_column('K:K', 9, format_border_right)
if comparison==1:
	worksheet.set_column('L:L', 8, format_border)
	worksheet.set_column('M:M', 8, format_border)
	worksheet.set_column('N:N', 8, format_border)
	worksheet.set_column('O:O', 12, format_border)
	worksheet.set_column('P:P', 10, format_border)
	worksheet.set_column('Q:Q', 9, format_border_right)
	worksheet.set_column('R:R', 12, format_border)
	worksheet.set_column('S:S', 12, format_border_right)

	worksheet.set_column('T:AI', 10, format_border)
	worksheet.set_column('AJ:AJ', 10, format_border_right)
else:
	worksheet.set_column('L:AA', 10, format_border)
	worksheet.set_column('AB:AB', 10, format_border_right)


workbook.close()

#os.remove('temp.csv')
os.remove('temp_stats_summary.csv')
