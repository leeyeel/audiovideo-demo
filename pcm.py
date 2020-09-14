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
        '非常简单的加速两倍播放,抽帧实现'
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
        
    def samplerate(self, rate = 48000):
        print('function: %s is empty' % sys._getframe().f_code.co_name)

    def wav_info(self, paramlist = []):
        with open(self.iname, 'rb') as inf:
            header = struct.unpack('<4cI4c4cihhiihh4ci', inf.read(44))
            print(header)
            headernamelist = ['ChunkID', 'ChunkSize', 'Format', 'SubChun1kID', 'SubChunk1Size', 'AudioFormat', 'NumChannels',
                                'SampleRate', 'ByteRate', 'BlockAlign', 'BitsPerSample', 'SubChunk2ID', 'SubChunk2Size']
            headervaluelist = [header[0:4], header[4], header[5:9], header[9:13], header[13], header[14], 
                    header[15], header[16], header[17], header[18], header[19], header[20], header[21]]
            for (name, value) in zip(headernamelist, headervaluelist):
                print(name + ':' + str(value))

            print('function: %s is done' % sys._getframe().f_code.co_name)

if __name__=="__main__":
    pcm = pcm('13_hush_hush.wav', 44)
    #pcm.soft_volume(3)
    #pcm.soft_speed()
    pcm.wav_info()
