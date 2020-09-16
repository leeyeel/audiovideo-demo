#!/usr/bin/env python
import time
import sys
import struct

class pcm(object):

    def __init__(self, inputname, offset = 0, outputname = 'out.raw', formats = 'S16_LE', channels = 2):
        self.iname = inputname
        self.offset = offset
        self.oname = outputname
        namelist = outputname.rsplit('.', 1)
        if len(namelist) > 1:
            self.lname = namelist[0] + '-lelf.' + namelist[1]
            self.rname = namelist[0] + '-right.' + namelist[1]
        else:
            self.lname = namelist[0] + '-lelf'
            self.rname = namelist[0] + '-right'
        self.format = 2
        self.channels = channels
        self.format_tag = {1:'pcm', 3:'FLOAT', 6:'G711-a', 7:'G711-u', 0xFFFE:'EXTENDED'}

    def split_left_right(self):
        with open(self.iname, 'rb') as inf:
            offsets = inf.read(self.offset)
            with open(self.rname, 'wb') as lf, open(self.lname, 'wb') as rf:
                while True:
                    lbytes = inf.read(self.format)
                    if not lbytes: break
                    lf.write(lbytes)
                    rbytes = inf.read(self.format)
                    if not rbytes: break
                    rf.write(rbytes)

    def soft_volume(self, coefficient = 1.0):
        with open(self.iname, 'rb') as inf:
            offsets = inf.read(self.offset)
            with open(self.oname, 'wb') as outf:
                while True:
                    by = inf.read(self.format)
                    if not by: break
                    res = struct.unpack('<h', by)[0] * coefficient 
                    s16 = 0x7FFF if round(res) > 0x7FFF else round(res)
                    s16 = (-0x7FFF - 1) if s16 < -0x7FFF else s16
                    strs = struct.pack('<h', s16)
                    outf.write(strs)
        print('function: %s done' % sys._getframe().f_code.co_name)

    def soft_speed(self, speed = 2.0):
        with open(self.iname, 'rb') as inf:
            offsets = inf.read(self.offset)
            with open(self.oname, 'wb') as outf:
                while True:
                    left = inf.read(self.format)
                    if not left: break
                    outf.write(left)
                    right = inf.read(self.format)
                    if not right: break
        print('function: %s is done' % sys._getframe().f_code.co_name)

    def __find_format(self, inf):
        ID = struct.unpack('>4c', inf.read(4))
        if ID == (b'f', b'm', b't', b' '):
            return 0
        else:
            size = struct.unpack('<I', inf.read(4))
            inf.read(size[0])
            self.__find_format(inf)
        
    def samplerate(self, rate = 48000):
        print('function: %s is empty' % sys._getframe().f_code.co_name)

    def wav_info(self, paramlist = []):
        with open(self.iname, 'rb') as inf:
            riffID = struct.unpack('>4c', inf.read(4))
            riff_size = struct.unpack('<I', inf.read(4))
            riff_type = struct.unpack('>4c', inf.read(4))
            if riff_type == (b'W', b'A', b'V', b'E'):
                self.__find_format(inf)
                format_size = struct.unpack('<I', inf.read(4))
                format_tag = struct.unpack('<H', inf.read(2))
                format_channels = struct.unpack('<H', inf.read(2))
                format_samplerate = struct.unpack('<I', inf.read(4))
                format_byterate = struct.unpack('<I', inf.read(4))
                format_align = struct.unpack('<H', inf.read(2))
                format_format = struct.unpack('<H', inf.read(2))
                if format_tag == 0xFFFE:
                    format_cbsize = struct.unpack('<H', inf.read(2))
                    format_cb = struct.unpack('<B', inf.read(22))
                print('format tag is: ' + str(self.format_tag[format_tag[0]]))
                print('channels : ' + str(format_channels[0]))
                print('format : ' + str(format_format[0]))
                print('samplerate is: ' + str(format_samplerate[0]))
            else:
                print('unrecognized format')

if __name__=="__main__":
    pcm = pcm('13_hush_hush.wav', 44)
    #pcm.soft_volume(3)
    #pcm.soft_speed()
    pcm.wav_info()
