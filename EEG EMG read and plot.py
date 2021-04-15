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

    def __init__(self, serialPort = 'COM9', serialBaud = 230400, plotTime = 2, sampleFreq = 256):
        self.port = serialPort
        self.baud = serialBaud
        self.packetList = []
        self.thread = None
        self.isRun = True
        self.errorNum = 0
        self.dict = ['0','1','2','3','4','5','6','7','8','9','*','-']
        self.EEG_previous_packet_timestamp = 0
        self.EMG_previous_packet_timestamp = 0
        self.plot_time = plotTime
        self.plot_EEG_timestamp = collections.deque([0]*plotTime*sampleFreq, maxlen = plotTime*sampleFreq+100)
        self.plot_EEG_signal = collections.deque([0]*plotTime*sampleFreq, maxlen = plotTime*sampleFreq+100)
        self.plot_EMG_timestamp = collections.deque([0]*plotTime*sampleFreq, maxlen = plotTime*sampleFreq+100)
        self.plot_EMG_signal = collections.deque([0]*plotTime*sampleFreq, maxlen = plotTime*sampleFreq+100)

        # Data file names
        self.currentTime = datetime.datetime.now()
        self.EEG_filename = 'EEG'+str(self.currentTime.month)+str(self.currentTime.day) \
        +str(self.currentTime.hour)+str(self.currentTime.minute)+'.txt'
        self.EMG_filename = 'EMG'+str(self.currentTime.month)+str(self.currentTime.day) \
        +str(self.currentTime.hour)+str(self.currentTime.minute)+'.txt'
        self.EEG_file = open(self.EEG_filename,'a+')
        self.EMG_file = open(self.EMG_filename,'a+')


        
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
            return 1
        elif (packetStr[0] != '(') | (packetStr[-2] != ')'): #check if packet complete
            return 2
        else:
            for i in range (1, packetStrLen-2, 1):
                if (packetStr[i] in self.dict) == 0:  #check if packet contains wrong characters
                    return 3
                elif ((packetStr[i] == '-') & (packetStr[i-1] != '*')): #check situation like "124-456" caused by missing "*"
                    return 4
                else:
                    continue
                            
            self.packetList =packetStr.split('*')
            packetListLen = len (self.packetList)
            
            for k in range(packetListLen):
                if self.packetList[k] == '': #check empty chars: if a digit is missed, we may get "**", causing empty char after splitting
                    return 5
                elif self.packetList[k] == '-': #check cases like '*-*'
                    return 6
                
            if packetListLen < 7: # make sure packetListStr[] index in range
                return 7
            elif (packetListLen != (int(self.packetList[4]) +6)): #check packet length
                return 8
            elif (int(self.packetList[1]) == 0):
                if int(self.packetList[2]) <= self.EEG_previous_packet_timestamp:
                  #  print(self.packetList[2])
                  #  print(self.EEG_previous_packet_timestamp)
                    self.packetList[2] = str(self.EEG_previous_packet_timestamp + int(self.packetList[3]) * int(self.packetList[4]))
                  #  print(self.packetList[2])
                self.EEG_previous_packet_timestamp = int(self.packetList[2])
                return 0
            elif (int(self.packetList[1]) == 1): #timestamp often loses one digit; Manuall correct it.
                if int(self.packetList[2]) <= self.EMG_previous_packet_timestamp:
                  #  print(self.packetList[2])
                  #  print(self.EMG_previous_packet_timestamp)
                    self.packetList[2] = str(self.EMG_previous_packet_timestamp + int(self.packetList[3]) * int(self.packetList[4]))
                  #  print(self.packetList[2])
                self.EMG_previous_packet_timestamp = int(self.packetList[2])
                return 0
            else:
                return 0
            


    def backgroundThread(self):
        self.serialConnection.reset_input_buffer()
        
        while self.isRun:
            packet = self.serialConnection.readline() #byte array
            packetStr = packet.decode('ASCII') #decoded to string
            #print(packetStr)

            error_code = self.errorcheck(packetStr)

            if error_code == 0:
                packet_len = len(self.packetList)
                device_type = int(self.packetList[1])
                last_timestamp = int(self.packetList[2])
                sampling_period = int(self.packetList[3])
                sample_num = int(self.packetList[4])

##                #The sensor sometimes returns an extra value with abnormal sampling interval at begin of packet, discard it.
##                if sample_num == 33:
##                    sample_num = 32

                if device_type == 0:
                    for i in range(sample_num):
                        timestamp = last_timestamp - (sample_num - i - 1)*sampling_period
                        signal = int(self.packetList[i+5])
                        self.EEG_file.write(str(sample_num) + '-' + str(i + 1) + ',' + str(timestamp) + ',' + str(signal) + '\n')
                        self.plot_EEG_timestamp.append(timestamp/1000000)
                        self.plot_EEG_signal.append(signal/16)
                elif device_type ==1:
                    for i in range(sample_num):
                        timestamp = last_timestamp - (sample_num - i - 1)*sampling_period
                        signal = int(self.packetList[i+5])
                        self.EMG_file.write(str(sample_num) + '-' + str(i + 1) + ',' + str(timestamp) + ',' + str(signal) + '\n')
                        self.plot_EMG_timestamp.append(timestamp/1000000)
                        self.plot_EMG_signal.append(signal/16)
                else:
                    continue
            else:
                print('error code = ' + str(error_code))
                print(packetStr)
                self.errorNum += 1
                print('Err:'+str(self.errorNum)+'\n')
                continue
            

    def close(self):
        self.isRun = False
        self.thread.join()
        self.serialConnection.close()
        print('Disconnected...')

    def getPlotData(self, frame, ax_EEG, ax_EMG, line_EEG, line_EMG):
        # Set format
        ax_EEG.set_xlim(self.plot_EEG_timestamp[-1]-self.plot_time,
                        self.plot_EEG_timestamp[-1])                             
        ax_EMG.set_xlim(self.plot_EMG_timestamp[-1]-self.plot_time,
                        self.plot_EMG_timestamp[-1])
##        EEG_max, EMG_max = self.mymax()
##        ax_EEG.set_ylim(-EEG_max*1.5, EEG_max*1.5)
##        ax_EMG.set_ylim(-EMG_max*1.5, EMG_max*1.5)
        


        # Plot data
        line_EEG.set_data(self.plot_EEG_timestamp, self.plot_EEG_signal)
        line_EMG.set_data(self.plot_EMG_timestamp, self.plot_EMG_signal)

    def mymax(self):
        EEG_deque_len = self.plot_EEG_signal.maxlen
        EMG_deque_len = self.plot_EMG_signal.maxlen
        EEG_max = 1
        EMG_max = 1
        for i in np.arange(EEG_deque_len):
            if abs(self.plot_EEG_signal[i]) > EEG_max:
                EEG_max = abs(self.plot_EEG_signal[i])
            else:
                continue
        for i in np.arange(EMG_deque_len):
            if abs(self.plot_EMG_signal[i]) > EMG_max:
                EMG_max = abs(self.plot_EMG_signal[i])
            else:
                continue
        return EEG_max, EMG_max     


def main():
    portName = 'COM3'
    baudRate = 230400
    pltInterval = 400 # in ms
    plotTime = 2 # in s
    sampleFreq = 256 
    s = serialPlot(portName, baudRate, plotTime, sampleFreq)
    s.readSerialStart()

    time.sleep(1)

    fig, (ax_EEG, ax_EMG) = plt.subplots(2,1)
    plt.subplots_adjust(hspace=0.5, left=0.15)
    line_EEG, =ax_EEG.plot([],[],label = 'EEG')
    line_EMG, =ax_EMG.plot([],[], label = 'EMG')
    ax_EEG.set_title('EEG')
    ax_EEG.set_xlabel('Time(s)')
    ax_EEG.set_ylabel('Voltage(uV)')
    ax_EMG.set_title('EMG')
    ax_EMG.set_xlabel('Time(s)')
    ax_EMG.set_ylabel('Voltage(uV)')
    ax_EEG.set_ylim(-100, 100)
    ax_EMG.set_ylim(-100, 100)
    anim = animation.FuncAnimation(fig, s.getPlotData, fargs=(ax_EEG, ax_EMG, line_EEG, line_EMG), interval=pltInterval)
    plt.show()    

    s.close()
        
if __name__ == '__main__':
    main()
