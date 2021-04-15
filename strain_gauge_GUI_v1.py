from __future__ import division
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from threading import Thread
from matplotlib import style
import numpy as np
import serial
import time
import datetime
import collections



class serialPlot:

    def __init__(self, serialPort = 'COM3', serialBaud = 115200, plotTime = 2, sampleFreq = 100):
        self.port = serialPort
        self.baud = serialBaud
        self.packetList = []
        self.thread = None
        self.isRun = True
        self.errorNum = 0
        self.dict = ['0','1','2','3','4','5','6','7','8','9','*','-']
        self.previous_packet_timestamp = 0
        self.plot_time = plotTime
        self.plot_EEG_timestamp = collections.deque([0]*plotTime*sampleFreq, maxlen = plotTime*sampleFreq+10)
        self.plot_EEG_signal = collections.deque([0]*plotTime*sampleFreq, maxlen = plotTime*sampleFreq+10)

        # Data file names
        self.currentTime = datetime.datetime.now()
        self.EEG_filename = 'Strain'+str(self.currentTime.month)+str(self.currentTime.day) \
        +str(self.currentTime.hour)+str(self.currentTime.minute)+'.txt'
        self.EEG_file = open(self.EEG_filename,'a+')
        self.EEG_prev_timestamp = 0

        
        try:
            self.serialConnection = serial.Serial(serialPort, serialBaud)
            print('Connected to', serialPort, 'at baudrate',serialBaud)
        except:
            print('Failed to connected to', serialPort, 'at baudrate',serialBaud)


    def readSerialStart(self):
        if self.thread == None:
            self.thread = Thread(target=self.backgroundThread)
            self.thread.start()

    def errorcheck (self, packetStr):
        packetStrLen = len(packetStr)
        
        if packetStrLen < 3:# make sure packetStr[] index in range
            return False
        elif (packetStr[0] != '(') | (packetStr[-2] != ')'): #check if packet complete
            return False
        else:
            for i in range (1, packetStrLen-2, 1):
                if (packetStr[i] in self.dict) == 0:  #check if packet contains wrong characters
                    return False
                elif ((packetStr[i] == '-') & (packetStr[i-1] != '*')): #check situation like "124-456" caused by missing "*"
                    return False
                else:
                    continue
                            
            self.packetList =packetStr.split('*')
            packetListLen = len (self.packetList)
            
            for k in range(packetListLen):
                if self.packetList[k] == '': #check empty chars: if a digit is missed, we may get "**", causing empty char after splitting
                    return False
                elif self.packetList[k] == '-': #check cases like '*-*'
                    return False
                
            if packetListLen < 7: # make sure packetListStr[] index in range
                return False
            elif (packetListLen != (int(self.packetList[3]) +5)): #check packet length
                return False
            return True


    def backgroundThread(self):
        self.serialConnection.reset_input_buffer()
        
        while self.isRun:
            packet = self.serialConnection.readline() #byte array
            packetStr = packet.decode('ASCII') #decoded to string
            
            
            if self.errorcheck(packetStr) == True:
                packet_len = len(self.packetList)
                tot_count = int(self.packetList[1])
                timer_period = int(self.packetList[2])
                buffer_len = int(self.packetList[3])


                for i in range(buffer_len):
                    timestamp = tot_count*timer_period - (buffer_len - i - 1)*timer_period
                    signal = int(self.packetList[i+4])
                    self.EEG_file.write(str(timestamp) + ',' + str(signal) + '\n')
                    self.plot_EEG_timestamp.append(timestamp/1000)
                    self.plot_EEG_signal.append(signal)

            else:
                self.errorNum += 1
                print('Err:'+str(self.errorNum)+'\n')
                continue
            

    def close(self):
        self.isRun = False
        self.thread.join()
        self.serialConnection.close()
        print('Disconnected...')

    def getPlotData(self, frame, ax_EEG, line_EEG):
        # Set format
        ax_EEG.set_xlim(self.plot_EEG_timestamp[-1]-self.plot_time,
                        self.plot_EEG_timestamp[-1])                             

        EEG_max, EEG_min = self.mymaxmin()
        ax_EEG.set_ylim(EEG_min-100, EEG_max+100)
        
        # Plot data
        line_EEG.set_data(self.plot_EEG_timestamp, self.plot_EEG_signal)

    def mymaxmin(self):
        EEG_deque_len = self.plot_EEG_signal.maxlen
        
        EEG_max = 1
        EEG_min = 16000
        
        for i in np.arange(EEG_deque_len):
            if self.plot_EEG_signal[i] > EEG_max:
                EEG_max = self.plot_EEG_signal[i]
            elif self.plot_EEG_signal[i] < EEG_min:
                EEG_min = self.plot_EEG_signal[i]
            else:
                continue

        return EEG_max, EEG_min 


def main():
    portName = 'COM7'
    baudRate = 115200
    pltInterval = 100 # in ms
    plotTime = 8 # in s
    sampleFreq = 100 
    s = serialPlot(portName, baudRate, plotTime, sampleFreq)
    s.readSerialStart()

    time.sleep(1)

    fig, ax_EEG = plt.subplots(1,1)
    plt.subplots_adjust(hspace=0.5, left=0.15)
    line_EEG, =ax_EEG.plot([],[],label = 'Strain Gauge')
    ax_EEG.set_title('Strain Gauge')
    ax_EEG.set_xlabel('Time(s)')
    ax_EEG.set_ylabel('ADC(a.u.)')
    
    ax_EEG.set_ylim(0, 16000)
    anim = animation.FuncAnimation(fig, s.getPlotData, fargs=(ax_EEG,line_EEG), interval=pltInterval)
    plt.show()    

    s.close()
        
if __name__ == '__main__':
    main()
