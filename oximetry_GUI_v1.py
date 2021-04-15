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
        self.plot_LED1_timestamp = collections.deque([0]*(plotTime*sampleFreq+50), maxlen = plotTime*sampleFreq+50)
        self.plot_PD1_LED1 = collections.deque([0]*(plotTime*sampleFreq+50), maxlen = plotTime*sampleFreq+50)
        self.plot_PD1_LED2 = collections.deque([0]*(plotTime*sampleFreq+50), maxlen = plotTime*sampleFreq+50)
        self.plot_LED2_timestamp = collections.deque([0]*(plotTime*sampleFreq+50), maxlen = plotTime*sampleFreq+50)
        self.plot_PD2_LED1 = collections.deque([0]*(plotTime*sampleFreq+50), maxlen = plotTime*sampleFreq+50)
        self.plot_PD2_LED2 = collections.deque([0]*(plotTime*sampleFreq+50), maxlen = plotTime*sampleFreq+50)

        # Data file names
        self.currentTime = datetime.datetime.now()
        self.Ox_filename = 'Oximeter'+str(self.currentTime.month)+str(self.currentTime.day) \
        +str(self.currentTime.hour)+str(self.currentTime.minute)+'.txt'
        self.Ox_file = open(self.Ox_filename,'a+')
        self.Ox_file.write('LED1 Timestamp (ms),LED1_PD1,LED1_PD2,LED2 Timestamp (ms),LED2_PD1,LED2_PD2\n')
                    
        self.Ox_prev_timestamp = 0

        
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


                for i in range(int(buffer_len/4)):
                    LED1_timestamp = tot_count*timer_period - (int(buffer_len/4) - i)*timer_period*2
                    LED2_timestamp = tot_count*timer_period - (int(buffer_len/4) - i)*timer_period*2 + timer_period
                    signal_PD1_LED1 = int(self.packetList[i*4+4])
                    signal_PD2_LED1 = int(self.packetList[i*4+5])
                    signal_PD1_LED2 = int(self.packetList[i*4+6])
                    signal_PD2_LED2 = int(self.packetList[i*4+7])
                    #print(signal_PD1_LED1, signal_PD2_LED1, signal_PD1_LED2, signal_PD2_LED2)
                    self.Ox_file.write(str(LED1_timestamp) + ',' + str(signal_PD1_LED1) + ',' + str(signal_PD2_LED1) +','+ str(LED2_timestamp) + ',' + str(signal_PD1_LED2) + ',' + str(signal_PD2_LED2)+ '\n')
                    self.plot_LED1_timestamp.append(LED1_timestamp/1000)
                    self.plot_LED2_timestamp.append(LED2_timestamp/1000)
                    delta = LED1_timestamp - self.Ox_prev_timestamp
                    if delta!= 20:
                        print(delta)
                    self.Ox_prev_timestamp = LED1_timestamp
                    #print(LED1_timestamp, LED2_timestamp)
                    self.plot_PD1_LED1.append(signal_PD1_LED1)
                    self.plot_PD1_LED2.append(signal_PD1_LED2)
                    self.plot_PD2_LED1.append(signal_PD2_LED1)
                    self.plot_PD2_LED2.append(signal_PD2_LED2)
                    

            else:
                self.errorNum += 1
                print('Err:'+str(self.errorNum)+'\n')
                continue
            

    def close(self):
        self.isRun = False
        self.thread.join()
        self.serialConnection.close()
        print('Disconnected...')

    def getPlotData(self, frame, ax_PD1,line_PD1_LED1, line_PD1_LED2, ax_PD2, line_PD2_LED1, line_PD2_LED2):
        # Set format
 
        PD1_max, PD1_min, PD2_max, PD2_min = self.mymaxmin()
        #PD1_max, PD1_min, PD2_max, PD2_min = 14000, 12000, 14000, 12000
        
        ax_PD1.set_xlim(self.plot_LED1_timestamp[-1]-self.plot_time,
                        self.plot_LED2_timestamp[-1])                             
        ax_PD1.set_ylim(PD1_min*0.95, PD1_max*1.05)
        ax_PD1.set_ylim(8200, 8400)

        
        # Plot data
        line_PD1_LED1.set_data(self.plot_LED1_timestamp, self.plot_PD1_LED1)
        line_PD1_LED2.set_data(self.plot_LED2_timestamp, self.plot_PD1_LED2)

        ax_PD2.set_xlim(self.plot_LED1_timestamp[-1]-self.plot_time,
                        self.plot_LED2_timestamp[-1])
        ax_PD2.set_ylim(PD2_min*0.95, PD2_max*1.05)
        #ax_PD2.set_ylim(8300, 8500)

        
        # Plot data
        line_PD2_LED1.set_data(self.plot_LED1_timestamp, self.plot_PD2_LED1)
        line_PD2_LED2.set_data(self.plot_LED2_timestamp, self.plot_PD2_LED2)
        

    def mymaxmin(self):
        
        deque_len = self.plot_PD1_LED1.maxlen
        
        PD1_max = 1
        PD1_min = 16000

        PD2_max = 1
        PD2_min = 16000
        
        for i in np.arange(deque_len):
            if self.plot_PD1_LED1[i] > PD1_max:
                PD1_max = self.plot_PD1_LED1[i]
            if self.plot_PD1_LED2[i] > PD1_max:
                PD1_max = self.plot_PD1_LED2[i]
            if self.plot_PD1_LED1[i] < PD1_min:
                PD1_min = self.plot_PD1_LED1[i]
            if self.plot_PD1_LED2[i] < PD1_min:
                PD1_min = self.plot_PD1_LED2[i]
                
            if self.plot_PD2_LED1[i] > PD2_max:
                PD2_max = self.plot_PD2_LED1[i]
            if self.plot_PD2_LED2[i] > PD2_max:
                PD2_max = self.plot_PD2_LED2[i]
            if self.plot_PD2_LED1[i] < PD2_min:
                PD2_min = self.plot_PD2_LED1[i]
            if self.plot_PD2_LED2[i] < PD2_min:
                PD2_min = self.plot_PD2_LED2[i]

        return PD1_max, PD1_min, PD2_max, PD2_min


def main():
    portName = 'COM7'
    baudRate = 115200
    pltInterval = 200 # in ms
    plotTime = 4 # in s
    sampleFreq = 50 
    s = serialPlot(portName, baudRate, plotTime, sampleFreq)
    s.readSerialStart()

    time.sleep(1)

    fig, (ax_PD1, ax_PD2) = plt.subplots(2,1)
    plt.subplots_adjust(hspace=0.5, left=0.15)
    line_PD1_LED1, =ax_PD1.plot([],[],'b-', label = 'LED1')
    line_PD1_LED2, =ax_PD1.plot([],[],'g-', label = 'LED2')
    
    ax_PD1.set_title('PD1')
    ax_PD1.set_xlabel('Time(s)')
    ax_PD1.set_ylabel('ADC(a.u.)')
    ax_PD1.set_ylim(12000, 14000)

    line_PD2_LED1, =ax_PD2.plot([],[],'b-', label = 'LED1')
    line_PD2_LED2, =ax_PD2.plot([],[],'g-', label = 'LED2')
    
    ax_PD2.set_title('PD2')
    ax_PD2.set_xlabel('Time(s)')
    ax_PD2.set_ylabel('ADC(a.u.)')
    ax_PD2.set_ylim(12000, 14000)
    
    anim = animation.FuncAnimation(fig, s.getPlotData, fargs=(ax_PD1,line_PD1_LED1, line_PD1_LED2, ax_PD2, line_PD2_LED1, line_PD2_LED2), interval=pltInterval)
    plt.show()    

    s.close()
        
if __name__ == '__main__':
    main()
