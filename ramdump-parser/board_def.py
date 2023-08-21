"""
Copyright (c) 2014-2019, The Linux Foundation. All rights reserved.
Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
"""

from boards import Board

class Board8960(Board):
    def __init__(self, socid, board_num, phys_offset=0x80200000, ram_start=0x80000000):
        super(Board8960, self).__init__()
        self.socid = socid
        self.board_num = board_num
        self.cpu = 'KRAIT'
        self.ram_start = ram_start
        self.imem_start = 0x2a03f000
        self.smem_addr = 0x0
        self.phys_offset = phys_offset
        self.wdog_addr = 0x2a03f658
        self.imem_file_name = 'IMEM_C.BIN'

class Board8625(Board):
    def __init__(self, socid, board_num):
        super(Board8625, self).__init__()
        self.socid = socid
        self.board_num = board_num
        self.cpu = 'SCORPION'
        self.ram_start = 0
        self.imem_start = 0x0
        self.smem_addr = 0x00100000
        self.phys_offset = 0x00200000

class Board9615(Board):
    def __init__(self, socid):
        super(Board9615, self).__init__()
        self.socid = socid
        self.board_num = "9615"
        self.cpu = 'CORTEXA5'
        self.ram_start = 0x40000000
        self.imem_start = 0
        self.smem_addr = 0x0
        self.phys_offset = 0x40800000

class Board8974(Board):
    def __init__(self, socid, board_num="8974"):
        super(Board8974, self).__init__()
        self.socid = socid
        self.board_num = board_num
        self.cpu = 'KRAIT'
        self.ram_start = 0x0
        self.imem_start = 0xfe800000
        self.smem_addr = 0xfa00000
        self.phys_offset = 0x0
        self.wdog_addr = 0xfe805658
        self.imem_file_name = 'OCIMEM.BIN'

class Board9625(Board):
    def __init__(self, socid):
        super(Board9625, self).__init__()
        self.socid = socid
        self.board_num = "9625"
        self.cpu = 'CORTEXA5'
        self.ram_start = 0x0
        self.imem_start = 0xfe800000
        self.smem_addr = 0x0
        self.phys_offset = 0x200000
        self.wdog_addr = 0xfe805658
        self.imem_file_name = 'OCIMEM.BIN'

class Board8626(Board):
    def __init__(self, socid, board_num="8626"):
        super(Board8626, self).__init__()
        self.socid = socid
        self.board_num = board_num
        self.cpu = 'CORTEXA7'
        self.ram_start = 0x0
        self.imem_start = 0xfe800000
        self.smem_addr = 0x0fa00000
        self.phys_offset = 0x0
        self.wdog_addr = 0xfe805658
        self.imem_file_name = 'OCIMEM.BIN'

class Board8026LW(Board):
    def __init__(self, socid, board_num="8026"):
        super(Board8026LW, self).__init__()
        self.socid = socid
        self.board_num = board_num
        self.cpu = 'CORTEXA7'
        self.ram_start = 0x0
        self.imem_start = 0xfe800000
        self.smem_addr = 0x03000000
        self.phys_offset = 0x0
        self.wdog_addr = 0xfe805658
        self.imem_file_name = 'OCIMEM.BIN'

class Board8610(Board):
    def __init__(self, socid, board_num="8610"):
        super(Board8610, self).__init__()
        self.socid = socid
        self.board_num = board_num
        self.cpu = 'CORTEXA7'
        self.ram_start = 0x0
        self.imem_start = 0xfe800000
        self.smem_addr = 0x0d900000
        self.phys_offset = 0x0
        self.wdog_addr = 0xfe805658
        self.imem_file_name = 'OCIMEM.BIN'

class Board9635(Board):
    def __init__(self, socid):
        super(Board9635, self).__init__()
        self.socid = socid
        self.board_num = "9635"
        self.cpu = 'CORTEXA7'
        self.ram_start = 0x0
        self.imem_start = 0xfe800000
        self.smem_addr = 0x1100000
        self.phys_offset = 0
        self.wdog_addr = 0xfe805658
        self.imem_file_name = 'OCIMEM.BIN'

class Board8916(Board):
    def __init__(self, socid, smem_addr):
        super(Board8916, self).__init__()
        self.socid = socid
        self.board_num = "8916"
        self.cpu = 'CORTEXA7'
        self.ram_start = 0x80000000
        #self.ram_start = 0x0
        self.smem_addr = smem_addr
        self.phys_offset = 0x80000000
        self.imem_start = 0x8600000
        self.wdog_addr = 0x8600658
        self.imem_file_name = 'OCIMEM.BIN'

class Board8939(Board):
    def __init__(self, socid, smem_addr):
        super(Board8939, self).__init__()
        self.socid = socid
        self.board_num = "8939"
        self.cpu = 'CORTEXA7'
        self.ram_start = 0x80000000
        self.smem_addr = smem_addr
        self.phys_offset = 0x80000000
        self.imem_start = 0x8600000
        self.wdog_addr = 0x8600658
        self.imem_file_name = 'OCIMEM.BIN'

class Board8936(Board):
    def __init__(self, socid):
        super(Board8936, self).__init__()
        self.socid = socid
        self.board_num = "8936"
        self.cpu = 'CORTEXA7'
        self.ram_start = 0x80000000
        self.smem_addr = 0x6300000
        self.phys_offset = 0x80000000
        self.imem_start = 0x8600000
        self.wdog_addr = 0x8600658
        self.imem_file_name = 'OCIMEM.BIN'

class Board8994(Board):
    def __init__(self, socid):
        super(Board8994, self).__init__()
        self.socid = socid
        self.board_num = "8994"
        self.cpu = 'CORTEXA57A53'
        self.ram_start = 0x0
        self.smem_addr = 0x6a00000
        self.phys_offset = 0x0
        self.imem_start = 0xfe800000
        self.wdog_addr = 0xfe87f658
        self.imem_file_name = 'OCIMEM.BIN'

class Board8909(Board):
    def __init__(self, socid):
        super(Board8909, self).__init__()
        self.socid = socid
        self.board_num = "8909"
        self.cpu = 'CORTEXA7'
        self.ram_start = 0x80000000
        self.smem_addr = 0x7d00000
        self.phys_offset = 0x80000000
        self.imem_start = 0x8600000
        self.wdog_addr = 0x8600658
        self.imem_file_name = 'OCIMEM.BIN'

class Board8908(Board):
    def __init__(self, socid):
        super(Board8908, self).__init__()
        self.socid = socid
        self.board_num = "8908"
        self.cpu = 'CORTEXA7'
        self.ram_start = 0x80000000
        self.smem_addr = 0x7d00000
        self.phys_offset = 0x80000000
        self.imem_start = 0x8600000
        self.wdog_addr = 0x8600658
        self.imem_file_name = 'OCIMEM.BIN'

class Board9640(Board):
    def __init__(self, socid):
        super(Board9640, self).__init__()
        self.socid = socid
        self.board_num = "9640"
        self.cpu = 'CORTEXA7'
        self.ram_start = 0x80000000
        self.smem_addr = 0x7e80000
        self.phys_offset = 0x80000000
        self.imem_start = 0x8600000
        self.wdog_addr = 0x8600658
        self.imem_file_name = 'OCIMEM.BIN'

class Board8992(Board):
    def __init__(self, socid):
        super(Board8992, self).__init__()
        self.socid = socid
        self.board_num = "8992"
        self.cpu = 'CORTEXA57A53'
        self.ram_start = 0x0
        self.smem_addr = 0x6a00000
        self.phys_offset = 0x0
        self.imem_start = 0xfe800000
        self.wdog_addr = 0xfe87f658
        self.imem_file_name = 'OCIMEM.BIN'

class Board8929(Board):
    def __init__(self, socid, smem_addr):
        super(Board8929, self).__init__()
        self.socid = socid
        self.board_num = "8929"
        self.cpu = 'CORTEXA7'
        self.ram_start = 0x80000000
        self.smem_addr = smem_addr
        self.phys_offset = 0x80000000
        self.imem_start = 0x8600000
        self.wdog_addr = 0x8600658

class Board8996(Board):
    def __init__(self, socid):
        super(Board8996, self).__init__()
        self.socid = socid
        self.board_num = "8996"
        self.cpu = 'HYDRA'
        self.ram_start = 0x80000000
        self.smem_addr = 0x6000000
        self.phys_offset = 0x80000000
        self.imem_start = 0x6680000
        self.wdog_addr = 0x66BF658
        self.imem_file_name = 'OCIMEM.BIN'

class Board8952(Board):
    def __init__(self, socid):
        super(Board8952, self).__init__()
        self.socid = socid
        self.board_num = "8952"
        self.cpu = 'CORTEXA53'
        self.ram_start = 0x80000000
        self.smem_addr = 0x6300000
        self.phys_offset = 0x80000000
        self.imem_start = 0x8600000
        self.wdog_addr = 0x8600658
        self.imem_file_name = 'OCIMEM.BIN'

class Board8976(Board):
    def __init__(self, socid):
        super(Board8976, self).__init__()
        self.socid = socid
        self.board_num = "8976"
        self.cpu = 'CORTEXA53'
        self.ram_start = 0x80000000
        self.smem_addr = 0x6300000
        self.phys_offset = 0x20000000
        self.imem_start = 0x8600000
        self.wdog_addr = 0x8600658
        self.imem_file_name = 'OCIMEM.BIN'

class Board9607(Board):
    def __init__(self, socid):
        super(Board9607, self).__init__()
        self.socid = socid
        self.board_num = "9607"
        self.cpu = 'CORTEXA7'
        self.ram_start = 0x80000000
        self.smem_addr = 0x7d00000
        self.phys_offset = 0x80000000
        self.imem_start = 0x8600000
        self.wdog_addr = 0x8600658
        self.imem_file_name = 'OCIMEM.BIN'


class Board8937(Board):
    def __init__(self, socid):
        super(Board8937, self).__init__()
        self.socid = socid
        self.board_num = "8937"
        self.cpu = 'CORTEXA53'
        self.ram_start = 0x80000000
        self.smem_addr = 0x6300000
        self.phys_offset = 0x40000000
        self.imem_start = 0x8600000
        self.kaslr_addr = 0x86006d0
        self.wdog_addr = 0x8600658
        self.imem_file_name = 'OCIMEM.BIN'

class Board8940(Board):
     def __init__(self, socid):
         super(Board8940, self).__init__()
         self.socid = socid
         self.board_num = "8940"
         self.cpu = 'CORTEXA53'
         self.ram_start = 0x80000000
         self.smem_addr = 0x6300000
         self.phys_offset = 0x40000000
         self.imem_start = 0x8600000
         self.wdog_addr = 0x8600658
         self.imem_file_name = 'OCIMEM.BIN'

class Board8953(Board):
    def __init__(self, socid):
        super(Board8953, self).__init__()
        self.socid = socid
        self.board_num = "8953"
        self.cpu = 'CORTEXA53'
        self.ram_start = 0x80000000
        self.smem_addr = 0x6300000
        self.phys_offset = 0x40000000
        self.imem_start = 0x8600000
        self.wdog_addr = 0x8600658
        self.imem_file_name = 'OCIMEM.BIN'

class Board450(Board):
    def __init__(self, socid):
        super(Board450, self).__init__()
        self.socid = socid
        self.board_num = "450"
        self.cpu = 'CORTEXA53'
        self.ram_start = 0x80000000
        self.smem_addr = 0x6300000
        self.phys_offset = 0x40000000
        self.imem_start = 0x8600000
        self.wdog_addr = 0x8600658
        self.kaslr_addr = 0x86006d0
        self.imem_file_name = 'OCIMEM.BIN'

class Board632(Board):
    def __init__(self, socid):
        super(Board632, self).__init__()
        self.socid = socid
        self.board_num = "632"
        self.cpu = 'CORTEXA53'
        self.ram_start = 0x80000000
        self.smem_addr = 0x6300000
        self.phys_offset = 0x40000000
        self.imem_start = 0x8600000
        self.kaslr_addr = 0x86006d0
        self.wdog_addr = 0x8600658
        self.imem_file_name = 'OCIMEM.BIN'

class Board439(Board):
    def __init__(self, socid):
        super(Board439, self).__init__()
        self.socid = socid
        self.board_num = "sdm439"
        self.cpu = 'CORTEXA53'
        self.ram_start = 0x80000000
        self.smem_addr = 0x6300000
        self.phys_offset = 0x40000000
        self.imem_start = 0x8600000
        self.kaslr_addr = 0x86006d0
        self.wdog_addr = 0x8600658
        self.imem_file_name = 'OCIMEM.BIN'

class Board429(Board):
    def __init__(self, socid):
        super(Board429, self).__init__()
        self.socid = socid
        self.board_num = "sdm429"
        self.cpu = 'CORTEXA53'
        self.ram_start = 0x80000000
        self.smem_addr = 0x6300000
        self.phys_offset = 0x40000000
        self.imem_start = 0x8600000
        self.kaslr_addr = 0x86006d0
        self.wdog_addr = 0x8600658
        self.imem_file_name = 'OCIMEM.BIN'

class Board8917(Board):
    def __init__(self, socid):
        super(Board8917, self).__init__()
        self.socid = socid
        self.board_num = "8917"
        self.cpu = 'CORTEXA53'
        self.ram_start = 0x80000000
        self.smem_addr = 0x6300000
        self.phys_offset = 0x40000000
        self.imem_start = 0x8600000
        self.kaslr_addr = 0x86006d0
        self.wdog_addr = 0x8600658
        self.imem_file_name = 'OCIMEM.BIN'

class Board8920(Board):
     def __init__(self, socid):
         super(Board8920, self).__init__()
         self.socid = socid
         self.board_num = "8920"
         self.cpu = 'CORTEXA53'
         self.ram_start = 0x80000000
         self.smem_addr = 0x6300000
         self.phys_offset = 0x40000000
         self.imem_start = 0x8600000
         self.wdog_addr = 0x8600658
         self.imem_file_name = 'OCIMEM.BIN'

class BoardCalifornium(Board):
    def __init__(self, socid):
        super(BoardCalifornium, self).__init__()
        self.socid = socid
        self.board_num = "californium"
        self.cpu = 'CORTEXA7'
        self.ram_start = 0x80000000
        self.smem_addr = 0x7e80000
        self.phys_offset = 0x80000000
        self.imem_start = 0x08600000
        self.wdog_addr = 0x8600658
        self.imem_file_name = 'OCIMEM.BIN'


class BoardCobalt(Board):
    def __init__(self, socid):
        super(BoardCobalt, self).__init__()
        self.socid = socid
        self.board_num = "cobalt"
        self.cpu = 'CORTEXA53'
        self.ram_start = 0x80000000
        self.smem_addr = 0x6000000
        self.phys_offset = 0x80000000
        self.imem_start = 0x14680000
        self.kaslr_addr = 0x146bf6d0
        self.wdog_addr = 0x146BF658
        self.imem_file_name = 'OCIMEM.BIN'

class BoardSDM845(Board):
    def __init__(self, socid):
        super(BoardSDM845, self).__init__()
        self.socid = socid
        self.board_num = "sdm845"
        self.cpu = 'CORTEXA53'
        self.ram_start = 0x80000000
        self.smem_addr = 0x6000000
        self.smem_addr_buildinfo = 0x6007210
        self.phys_offset = 0x80000000
        self.imem_start = 0x14680000
        self.kaslr_addr = 0x146bf6d0
        self.wdog_addr = 0x146BF658
        self.imem_file_name = 'OCIMEM.BIN'

class BoardSDM710(Board):
    def __init__(self, socid):
        super(BoardSDM710, self).__init__()
        self.socid = socid
        self.board_num = "sdm710"
        self.cpu = 'CORTEXA53'
        self.ram_start = 0x80000000
        self.smem_addr = 0x6000000
        self.smem_addr_buildinfo = 0x6007210
        self.phys_offset = 0x80000000
        self.imem_start = 0x14680000
        self.kaslr_addr = 0x146bf6d0
        self.wdog_addr = 0x146BF658
        self.imem_file_name = 'OCIMEM.BIN'

class BoardTrinket(Board):
    def __init__(self, socid):
        super(BoardTrinket, self).__init__()
        self.socid = socid
        self.board_num = "trinket"
        self.cpu = 'CORTEXA53'
        self.ram_start = 0x40000000
        self.smem_addr = 0x6000000
        self.smem_addr_buildinfo = 0x6007210
        self.phys_offset = 0x40000000
        self.imem_start = 0x0c100000
        self.kaslr_addr = 0x0c1256d0
        self.wdog_addr = 0x0c125658
        self.imem_file_name = 'OCIMEM.BIN'

class BoardQCS605(Board):
    def __init__(self, socid):
        super(BoardQCS605, self).__init__()
        self.socid = socid
        self.board_num = "qcs605"
        self.cpu = 'CORTEXA53'
        self.ram_start = 0x80000000
        self.smem_addr = 0x6000000
        self.phys_offset = 0x80000000
        self.imem_start = 0x14680000
        self.kaslr_addr = 0x146bf6d0
        self.wdog_addr = 0x146BF658
        self.imem_file_name = 'OCIMEM.BIN'

class BoardQCS405(Board):
    def __init__(self, socid):
        super(BoardQCS405, self).__init__()
        self.socid = socid
        self.board_num = "qcs405"
        self.cpu = 'CORTEXA53'
        self.ram_start = 0x80000000
        self.smem_addr = 0x6300000
        self.phys_offset = 0x40000000
        self.imem_start = 0x8600000
        self.kaslr_addr = 0x86006d0
        self.wdog_addr = 0x8600658
        self.imem_file_name = 'OCIMEM.BIN'

class BoardQCS403(Board):
    def __init__(self, socid):
        super(BoardQCS403, self).__init__()
        self.socid = socid
        self.board_num = "qcs403"
        self.cpu = 'CORTEXA53'
        self.ram_start = 0x80000000
        self.smem_addr = 0x6300000
        self.phys_offset = 0x40000000
        self.imem_start = 0x8600000
        self.wdog_addr = 0x8600658
        self.imem_file_name = 'OCIMEM.BIN'

class Board8998(Board):
    def __init__(self, socid):
        super(Board8998, self).__init__()
        self.socid = socid
        self.board_num = "8998"
        self.cpu = 'CORTEXA53'
        self.ram_start = 0x80000000
        self.smem_addr = 0x6000000
        self.phys_offset = 0x80000000
        self.imem_start = 0x14680000
        self.kaslr_addr = 0x146bf6d0
        self.wdog_addr = 0x146BF658
        self.imem_file_name = 'OCIMEM.BIN'

class Board660(Board):
    def __init__(self, socid):
        super(Board660, self).__init__()
        self.socid = socid
        self.board_num = "660"
        self.cpu = 'CORTEXA53'
        self.ram_start = 0x80000000
        self.smem_addr = 0x6000000
        self.smem_addr_buildinfo = 0x6006ec0
        self.phys_offset = 0x80000000
        self.imem_start = 0x14680000
        self.kaslr_addr = 0x146bf6d0
        self.wdog_addr = 0x146BF658
        self.imem_file_name = 'OCIMEM.BIN'

class Board630(Board):
    def __init__(self, socid):
        super(Board630, self).__init__()
        self.socid = socid
        self.board_num = "630"
        self.cpu = 'CORTEXA53'
        self.ram_start = 0x80000000
        self.smem_addr = 0x6000000
        self.phys_offset = 0x80000000
        self.imem_start = 0x14680000
        self.kaslr_addr = 0x146bf6d0
        self.wdog_addr = 0x146BF658
        self.imem_file_name = 'OCIMEM.BIN'

class BoardSDX20(Board):
    def __init__(self, socid):
        super(BoardSDX20, self).__init__()
        self.socid = socid
        self.board_num = "SDX20"
        self.cpu = 'CORTEXA7'
        self.ram_start = 0x80000000
        self.smem_addr = 0x7e80000
        self.phys_offset = 0x80000000
        self.imem_start = 0x08600000
        self.wdog_addr = 0x8600658
        self.imem_file_name = 'OCIMEM.BIN'

class BoardMsmnile(Board):
    def __init__(self, socid):
        super(BoardMsmnile, self).__init__()
        self.socid = socid
        self.board_num = "msmnile"
        self.cpu = 'CORTEXA53'
        self.ram_start = 0x80000000
        self.smem_addr = 0x6000000
        self.smem_addr_buildinfo = 0x6007210
        self.phys_offset = 0x80000000
        self.imem_start = 0x14680000
        self.kaslr_addr = 0x146bf6d0
        self.wdog_addr = 0x146BF658
        self.imem_file_name = 'OCIMEM.BIN'

class BoardSteppe(Board):
    def __init__(self, socid):
        super(BoardSteppe, self).__init__()
        self.socid = socid
        self.board_num = "steppe"
        self.cpu = 'CORTEXA53'
        self.ram_start = 0x80000000
        self.smem_addr = 0x6000000
        self.smem_addr_buildinfo = 0x6007210
        self.phys_offset = 0x80000000
        self.imem_start = 0x14680000
        self.kaslr_addr = 0x146aa6d0
        self.wdog_addr = 0x146aa658
        self.imem_file_name = 'OCIMEM.BIN'

class BoardLito(Board):
    def __init__(self, socid):
        super(BoardLito, self).__init__()
        self.socid = socid
        self.board_num = "lito"
        self.cpu = 'CORTEXA53'
        self.ram_start = 0x80000000
        self.smem_addr = 0x900000
        self.smem_addr_buildinfo = 0x907210
        self.phys_offset = 0xA2400000
        self.imem_start = 0x14680000
        self.kaslr_addr = 0x146ab6d0
        self.wdog_addr = 0x146ab658
        self.imem_file_name = 'OCIMEM.BIN'

class BoardAtoll(Board):
    def __init__(self, socid):
        super(BoardAtoll, self).__init__()
        self.socid = socid
        self.board_num = "atoll"
        self.cpu = 'CORTEXA53'
        self.ram_start = 0x80000000
        self.smem_addr = 0x900000
        self.smem_addr_buildinfo = 0x907210
        self.phys_offset = 0xA1200000
        self.imem_start = 0x14680000
        self.kaslr_addr = 0x146aa6d0
        self.wdog_addr = 0x146aa658
        self.imem_file_name = 'OCIMEM.BIN'

class BoardLagoon(Board):
    def __init__(self, socid):
        super(BoardLagoon, self).__init__()
        self.socid = socid
        self.board_num = "lagoon"
        self.cpu = 'CORTEXA53'
        self.ram_start = 0x80000000
        self.smem_addr = 0x900000
        self.smem_addr_buildinfo = 0x907210
        self.phys_offset = 0xA2480000
        self.imem_start = 0x14680000
        self.kaslr_addr = 0x146aa6d0
        self.wdog_addr = 0x146aa658
        self.imem_file_name = 'OCIMEM.BIN'

class BoardPoorwills(Board):
    def __init__(self, socid):
        super(BoardPoorwills, self).__init__()
        self.socid = socid
        self.board_num = "poorwills"
        self.cpu = 'CORTEXA7'
        self.ram_start = 0x80000000
        self.smem_addr = 0xFE40000
        self.phys_offset = 0x80000000
        self.imem_start = 0x14680000
        self.wdog_addr =  0x14680658
        self.imem_file_name = 'OCIMEM.BIN'

class BoardPrairie(Board):
    def __init__(self, socid):
        super(BoardPrairie, self).__init__()
        self.socid = socid
        self.board_num = "sdxprairie"
        self.cpu = 'CORTEXA7'
        self.ram_start = 0x80000000
        self.smem_addr = 0xFE40000
        self.phys_offset = 0x80000000
        self.imem_start = 0x14680000
        self.wdog_addr =  0x14680658
        self.imem_file_name = 'OCIMEM.BIN'

class BoardKona(Board):
    def __init__(self, socid):
        super(BoardKona, self).__init__()
        self.socid = socid
        self.board_num = "kona"
        self.cpu = 'CORTEXA53'
        self.ram_start = 0x80000000
        self.smem_addr = 0x900000
        self.smem_addr_buildinfo = 0x907210
        self.phys_offset = 0xA0000000
        self.imem_start = 0x14680000
        self.kaslr_addr = 0x146bf6d0
        self.wdog_addr = 0x146BF658
        self.imem_file_name = 'OCIMEM.BIN'

class BoardBengal(Board):
    def __init__(self, socid):
        super(BoardBengal, self).__init__()
        self.socid = socid
        self.board_num = "bengal"
        self.cpu = 'CORTEXA53'
        self.ram_start = 0x40000000
        self.smem_addr = 0x6000000
        self.smem_addr_buildinfo = 0x6007210
        self.phys_offset = 0x40000000
        self.imem_start = 0x0c100000
        self.kaslr_addr = 0x0c1256d0
        self.wdog_addr = 0x0c125658
        self.imem_file_name = 'OCIMEM.BIN'

class BoardSC7180(Board):
    def __init__(self, socid):
        super(BoardSC7180, self).__init__()
        self.socid = socid
        self.board_num = "sc7180"
        self.cpu = 'CORTEXA53'
        self.ram_start = 0x80000000
        self.smem_addr = 0x900000
        self.smem_addr_buildinfo = 0x907210
        self.phys_offset = 0x9b400000
        self.imem_start = 0x14680000
        self.kaslr_addr = 0x146aa6d0
        self.wdog_addr = 0x146aa658
        self.imem_file_name = 'OCIMEM.BIN'

class BoardScuba(Board):
    def __init__(self, socid):
        super(BoardScuba, self).__init__()
        self.socid = socid
        self.board_num = "scuba"
        self.cpu = 'CORTEXA53'
        self.ram_start = 0x40000000
        self.smem_addr = 0x6000000
        self.smem_addr_buildinfo = 0x6007210
        self.phys_offset = 0x40000000
        self.imem_start = 0x0c100000
        self.kaslr_addr = 0x0c1256d0
        self.wdog_addr = 0x0c125658
        self.imem_file_name = 'OCIMEM.BIN'

Board9640(socid=234)
Board9640(socid=235)
Board9640(socid=236)
Board9640(socid=237)
Board9640(socid=238)

Board8916(socid=206, smem_addr=0xe200000)
Board8916(socid=206, smem_addr=0x6300000)

Board8939(socid=239, smem_addr=0xe200000)
Board8939(socid=241, smem_addr=0xe200000)
Board8939(socid=239, smem_addr=0x6300000)
Board8939(socid=241, smem_addr=0x6300000)

Board8936(socid=233)
Board8936(socid=240)
Board8936(socid=242)
Board8936(socid=243)

Board8909(socid=245)
Board8909(socid=258)
Board8909(socid=265)

Board8908(socid=259)

Board8929(socid=268, smem_addr=0xe200000)
Board8929(socid=269, smem_addr=0xe200000)
Board8929(socid=270, smem_addr=0xe200000)
Board8929(socid=271, smem_addr=0x6300000)

Board8974(socid=126)
Board8974(socid=184)
Board8974(socid=185)
Board8974(socid=186)
Board8974(socid=208)
Board8974(socid=211)
Board8974(socid=214)
Board8974(socid=217)
Board8974(socid=209)
Board8974(socid=212)
Board8974(socid=215)
Board8974(socid=218)
Board8974(socid=194)
Board8974(socid=210)
Board8974(socid=213)
Board8974(socid=216)

Board9625(socid=134)
Board9625(socid=148)
Board9625(socid=149)
Board9625(socid=150)
Board9625(socid=151)
Board9625(socid=152)
Board9625(socid=173)
Board9625(socid=174)
Board9625(socid=175)


Board8626(socid=145)
Board8626(socid=158)
Board8626(socid=159)
Board8626(socid=198)
Board8626(socid=199)
Board8626(socid=200)
Board8626(socid=205)
Board8626(socid=219)
Board8626(socid=220)
Board8626(socid=222)
Board8626(socid=223)
Board8626(socid=224)

Board8026LW(socid=145)
Board8026LW(socid=158)
Board8026LW(socid=159)
Board8026LW(socid=198)
Board8026LW(socid=199)
Board8026LW(socid=200)
Board8026LW(socid=205)
Board8026LW(socid=219)
Board8026LW(socid=220)
Board8026LW(socid=222)
Board8026LW(socid=223)
Board8026LW(socid=224)

Board8610(socid=147)
Board8610(socid=161)
Board8610(socid=162)
Board8610(socid=163)
Board8610(socid=164)
Board8610(socid=165)
Board8610(socid=166)

Board8974(socid=178, board_num="8084")

Board9635(socid=187)
Board9635(socid=227)
Board9635(socid=228)
Board9635(socid=229)
Board9635(socid=230)
Board9635(socid=231)

Board8960(socid=87, board_num="8960")
Board8960(socid=122, board_num="8960")
Board8960(socid=123, board_num="8260")
Board8960(socid=124, board_num="8060")

Board8960(socid=244, board_num="8064", phys_offset=0x40200000,
                        ram_start=0x40000000)
Board8960(socid=109, board_num="8064")
Board8960(socid=130, board_num="8064")
Board8960(socid=153, board_num="8064")

Board8960(socid=116, board_num="8930")
Board8960(socid=117, board_num="8930")
Board8960(socid=118, board_num="8930")
Board8960(socid=119, board_num="8930")
Board8960(socid=154, board_num="8930")
Board8960(socid=155, board_num="8930")
Board8960(socid=156, board_num="8930")
Board8960(socid=157, board_num="8930")
Board8960(socid=160, board_num="8930")

Board8960(socid=120, board_num="8627")
Board8960(socid=121, board_num="8627")
Board8960(socid=138, board_num="8960")
Board8960(socid=139, board_num="8960")
Board8960(socid=140, board_num="8960")
Board8960(socid=141, board_num="8960")
Board8960(socid=142, board_num="8930")
Board8960(socid=143, board_num="8630")
Board8960(socid=144, board_num="8630")

Board9615(socid=104)
Board9615(socid=105)
Board9615(socid=106)
Board9615(socid=107)

Board8625(socid=88, board_num="8625")
Board8625(socid=89, board_num="8625")
Board8625(socid=96, board_num="8625")
Board8625(socid=90, board_num="8625")
Board8625(socid=91, board_num="8625")
Board8625(socid=92, board_num="8625")
Board8625(socid=97, board_num="8625")
Board8625(socid=98, board_num="8625")
Board8625(socid=99, board_num="8625")
Board8625(socid=100, board_num="8625")
Board8625(socid=101, board_num="8625")
Board8625(socid=102, board_num="8625")
Board8625(socid=103, board_num="8625")
Board8625(socid=127, board_num="8625")
Board8625(socid=128, board_num="8625")
Board8625(socid=129, board_num="8625")
Board8625(socid=131, board_num="8625")
Board8625(socid=132, board_num="8625")
Board8625(socid=133, board_num="8625")
Board8625(socid=135, board_num="8625")

Board8994(socid=207)

Board8992(socid=251)
Board8992(socid=252)

Board8996(socid=246)
Board8996(socid=291)
Board8996(socid=315)
Board8996(socid=316)

Board8952(socid=264)

Board8976(socid=266)
Board8976(socid=274)
Board8976(socid=277)
Board8976(socid=278)

Board9607(socid=290)
Board9607(socid=296)
Board9607(socid=297)
Board9607(socid=298)
Board9607(socid=299)

Board8937(socid=294)
Board8937(socid=295)

Board8940(socid=313)

Board8953(socid=293)
Board8953(socid=304)
Board450(socid=338)
Board632(socid=349)
Board632(socid=350)

Board8917(socid=303)
Board8917(socid=307)
Board8917(socid=308)
Board8917(socid=309)
Board8917(socid=386)

Board8920(socid=320)

BoardCalifornium(socid=279)

BoardCobalt(socid=292)
Board8998(socid=292)

Board660(socid=317)
Board660(socid=324)
Board660(socid=325)
Board660(socid=326)

Board630(socid=318)
Board630(socid=327)

BoardSDM845(socid=321)
BoardMsmnile(socid=339)
BoardSDX20(socid=333)

BoardSteppe(socid=355)
BoardSteppe(socid=369)
BoardSteppe(socid=365)
BoardSteppe(socid=366)

BoardLito(socid=400)

BoardAtoll(socid=407)

BoardLagoon(socid=434)

BoardSDM710(socid=336)
BoardSDM710(socid=337)
BoardSDM710(socid=360)

BoardQCS605(socid=347)
BoardQCS405(socid=352)
BoardQCS403(socid=373)

BoardTrinket(socid=394)

BoardPoorwills(socid=334)
BoardPoorwills(socid=335)

BoardPrairie(socid=357)
BoardPrairie(socid=368)

Board439(socid=353)
Board439(socid=363)

Board429(socid=354)
Board429(socid=364)

BoardKona(socid=356)

BoardBengal(socid=417)
BoardBengal(socid=444)
BoardBengal(socid=445)
BoardSC7180(socid=407)

BoardScuba(socid=441)
