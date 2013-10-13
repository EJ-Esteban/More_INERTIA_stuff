'''
Created on Jul 19, 2012

@author: duganje
'''
import tkinter as tk
from tkinter import messagebox
from tkinter import ttk
import struct
import json
import os
import time

COM_LIST = ('Com 1', 'Com 2', 'Com 3', 'Com 4', 'Com 5', 'Com 6',
            'Com 7', 'Com 8')
SAMPLING_RATES = ('1', '2', '4', '8', '16', '32', '64', '128', '256', 'Custom')

import serial
import threading
import gzip
import queue

def openPort(portName):
    port=serial.Serial(portName,115200)
    port.setTimeout(0.1);
    return port
    
def scanPorts():
    # scan for available ports. return a list of names
    available = []
    for i in range(256):
        try:
            test = serial.Serial(i)
            available.append(test.portstr)
            test.close()
        except serial.SerialException:
            pass
    return available

def getNodeID(port):
    
    cmd = tuple(struct.pack('<ssssH', b'C', b'I', b'X', b'X', fletcher(b'CIXX')))
     
    for i in range(5):
        port.flushInput()
        port.write(cmd)
        try:
            resp = port.read(2)
            retval = struct.unpack(">H", resp)[0]
        except struct.error:
            retval = 0
        if retval != 0:
            break
            
    return retval
        
def setNodeID(port, nodeID): 
    
    payload = struct.pack('<ssH', b'C', b'N', nodeID)
    checksum = struct.pack('<H',fletcher(payload))
    cmd = tuple(payload + checksum)
    
    if port.read(2) == b'OK':
        return True
    else:
        return False

def getSessSerial(port):
    
    cmd = tuple(b'CGXX') + (0x3b, 0xeb)
    
    for i in range(5):
        port.flushInput()
        port.write(cmd)
        try:
            resp = port.read(2)
            retval = struct.unpack(">H", resp)[0]         
        except struct.error:
            retval = 0
        if retval != 0:
            break
    
    return retval
        

def setSessSerial(port, serNum):
    payload = (0x43, 0x53) + tuple(serNum.to_bytes(2, 'little'))
    checksum = struct.pack('<H', fletcher(payload))
    cmd = payload + checksum
    port.flushInput()
    port.write(payload)
    if port.read(2) == b'OK':
        return True
    else:
        return False
         
         

def testPort(port):
    #send an intentionally bogus init to check for expected "F"
    #default test is 10, but throw in a second int to increase/decrese tests
    
    cmd = tuple(struct.pack('<ssssH', b'S', b'T', b'X', b'X', fletcher(b'STXX')))
    
    for i in range(10):
        
        port.flushInput()
        port.write(cmd)
        #print(str(i)+":"+str(cmd))
        
        if port.read(1)!= b'F':
            return False
       
    return True        #wrong device failure


def fletcher(data):
    
    sum1 = sum2 = 0
    
    for i in data:
        sum1 = (sum1 + i) % 255
        sum2 = (sum1 + sum2) % 255
        
    return (sum1 << 8)|sum2

class GSS_reader(threading.Thread):
    
    def __init__(self, comm, filename, init_state):
        threading.Thread.__init__(self)
        self.com_port = comm
        self.datafileout = gzip.open(filename, "wb")
        self.cmd_que = queue.Queue()
        self.cmd_que.put(init_state)
        
    def run(self):
        while(True):
            try:
                temp_cmd = self.cmd_que.get_nowait()
            except queue.Empty:
                pass
            else:
                cmd = temp_cmd
            if cmd == 'RUN':
                toRead = self.com_port.inWaiting()
                if(toRead > 0):
                    data = self.com_port.read(toRead)
                    self.datafileout.write(data)
#                    self.update_plot(data)
            elif cmd == 'PAUSE':
                if self.com_port.inWaiting() > 0:
                    self.com_port.write(b'P')
                self.com_port.flushInput()
            elif cmd == 'KILL':
                self.datafileout.close()
                break
            
    
    
#    def update_plot(self, plot, data):
#        for i in range(data.len):
#            self.index += 1
#            self.plot.plot(self.index, data)
#            self.plot.update()
#        
            
    def kill(self):
        self.cmd_que.put('KILL')
        
    def pause(self):
        self.cmd_que.put('PAUSE')
        
    def resume(self):
        self.cmd_que.put('RUN')

class GUI:
    def __init__(self):
        self.open_comm_GUI()
        
    def open_comm_GUI(self):
        
        root = tk.Tk()
        root.title("Open COM")
        root.columnconfigure(0, weight=1)
        root.rowconfigure(0, weight=1)
        
        #main frame
        mainframe = ttk.Labelframe(root, padding="10 10 12 12",
                                   text="COM Port Select")
        mainframe.grid(column=0, row=0, sticky=tk.NSEW)
        mainframe.columnconfigure(0, weight=1)
        mainframe.columnconfigure(1, weight=1)
        mainframe.rowconfigure(0, weight=1)
        mainframe.rowconfigure(1, weight=1)
        mainframe.rowconfigure(2, weight=1)
        
        #open, test and next buttons
        openButton = ttk.Button(mainframe, text='Open', command=self.on_open)
        self.testButton = ttk.Button(mainframe, text='Test', command=self.on_test)
        self.nextButton = ttk.Button(mainframe, text='>>', command=self.on_next)
       
        openButton.grid(column=1, row=0, sticky=tk.EW)
        self.testButton.grid(column=0, row=1, sticky=tk.EW)
        self.nextButton.grid(column=1, row=2, sticky=tk.EW)
        
        self.testButton.state(['disabled'])
        self.nextButton.state(['disabled'])
        
        #drop down menu
        self.com_name_var = tk.StringVar()
        comm_ports = ttk.Combobox(mainframe, textvariable=self.com_name_var)
        comm_ports.grid(column=0, row=0, sticky=tk.EW)
        comm_ports['values'] = scanPorts()
        comm_ports.state(['readonly'])
        
        self.test_result = tk.StringVar()
        test_status = ttk.Label(mainframe)
        test_status.grid(column=1, row=1)
        test_status['textvariable'] = self.test_result
                  
        # add padding around widgets          
        for child in mainframe.winfo_children(): 
            child.grid_configure(padx=10, pady=5)
            
        # treat the com window as the root
        self._root = root
    
    def open_config_GUI(self):
        
        root = tk.Toplevel()
        root.title("Configuration")
        root.columnconfigure(0, weight=1)
        root.rowconfigure(0, weight=1)
        
        root.protocol('WM_DELETE_WINDOW', self.config_close_handler) 
        
        #set up main frame
        mainframe = ttk.Frame(root, padding="10 10 12 12")
        mainframe.grid(column=0, row=0, sticky=tk.NSEW)
        mainframe.columnconfigure(0, weight=1)
        mainframe.columnconfigure(1, weight=1)
        mainframe.rowconfigure(0, weight=1)
        mainframe.rowconfigure(1, weight=1)
        mainframe.rowconfigure(2, weight=1)
        
        #set up channel select frame
        checkbox_frame = ttk.Labelframe(mainframe, padding="10 10 12 12",
                                        text="Channel Select")
        checkbox_frame.grid(column=0, row=3, sticky=tk.NSEW)
        
        #sampling rate frame
        SR_frame = ttk.Labelframe(mainframe, padding="5 5 5 5",
                                   text="Sampling Rate (Hz)")  
        SR_frame.grid(column=0, row=2)  
        
        #bits of representation frame
        BOR_frame = ttk.Labelframe(mainframe, padding="5 5 5 5",
                                   text="Bits of representation")    
        BOR_frame.grid(column=0, row=1, sticky=tk.NSEW)
        
        #save handles to widget variables
        self.start_collection = tk.StringVar()
        self.A0 = tk.StringVar()
        self.A1 = tk.StringVar()
        self.A2 = tk.StringVar()
        self.A3 = tk.StringVar()
        self.A4 = tk.StringVar()
        self.A5 = tk.StringVar()
        self.A6 = tk.StringVar()
        self.A7 = tk.StringVar()
        self.BOR = tk.StringVar()
        self.custom = tk.StringVar()
        
        ttk.Checkbutton(mainframe, variable=self.start_collection, text=
                'Start Collection on Send').grid(column=0, row=0, sticky=tk.W)
        self.start_collection.set('1')
                  
        self.sampling_rate = tk.StringVar()
        SR_dropdown = ttk.Combobox(SR_frame, textvariable=self.sampling_rate)
        SR_dropdown.bind('<<ComboboxSelected>>', self.update_SR)
        SR_dropdown.grid(column=0, row=0, sticky=tk.EW)
        SR_dropdown['values'] = SAMPLING_RATES
        SR_dropdown.state(['readonly'])
        self.sampling_rate.set('128')
        
        self.custom_SR_entry = ttk.Entry(SR_frame, width=12, textvariable=self.custom)
        self.custom_SR_entry.grid(column=0, row=1, sticky=tk.EW)
        self.custom_SR_entry.state(['disabled'])
        self.custom.set("custom rate")
    
       
        BOR_spinbox = tk.Spinbox(BOR_frame, from_=1, to=12, increment=1,
                    textvariable=self.BOR, width=2)
        BOR_spinbox.grid(column=0, row=0)
        self.BOR.set('12')
    
        
        ttk.Checkbutton(checkbox_frame, variable=self.A0, text='A0').grid(
                                                column=0, row=0, sticky=tk.EW)
        ttk.Checkbutton(checkbox_frame, variable=self.A1, text='A1').grid(
                                                column=0, row=1, sticky=tk.EW)
        ttk.Checkbutton(checkbox_frame, variable=self.A2, text='A2').grid(
                                                column=0, row=2, sticky=tk.EW)
        ttk.Checkbutton(checkbox_frame, variable=self.A3, text='A3').grid(
                                                column=0, row=3, sticky=tk.EW)
        ttk.Checkbutton(checkbox_frame, variable=self.A4, text='A4').grid(
                                                column=1, row=0, sticky=tk.EW)
        ttk.Checkbutton(checkbox_frame, variable=self.A5, text='A5').grid(
                                                column=1, row=1, sticky=tk.EW)
        ttk.Checkbutton(checkbox_frame, variable=self.A6, text='A6').grid(
                                                column=1, row=2, sticky=tk.EW)
        ttk.Checkbutton(checkbox_frame, variable=self.A7, text='A7').grid(
                                                column=1, row=3, sticky=tk.EW)
        
        self.A0.set(b'0')
        self.A1.set(b'0')
        self.A2.set(b'0')
        self.A3.set(b'0')
        self.A4.set(b'0')
        self.A5.set(b'0')
        self.A6.set(b'0')
        self.A7.set(b'0')
        
        file_frame = ttk.LabelFrame(mainframe, padding = "5 5 5 5", text = "File Options")
        file_frame.grid(column = 0, row = 4)
        ttk.Label(file_frame, text = "Output file name:").grid(column = 0, row = 0, stick = tk.W)
        self.filename = tk.StringVar();
        self.fname_field = ttk.Entry(file_frame, textvariable = self.filename).grid(column = 1, row = 0, sticky = tk.W)
        ttk.Button(file_frame, text='Auto Detect', command = self.autofile).grid(\
                                        column = 0, row = 1)
       
        if testPort(self.com_port):
            self.sess_serial = getSessSerial(self.com_port)
            self.filename.set('data/out%(sessNum)i.dat.gz' % {"sessNum":self.sess_serial})
        else:
            self.filename.set('data/out.dat.gz')
        

        
        #send button
        send_frame = ttk.Frame(mainframe, padding="5 5 5 5")
        send_frame.grid(column=0, row=5, sticky=tk.NSEW)
        
        self.conn_status = tk.StringVar()
        
        ttk.Button(send_frame, text = 'Test COM', command=self.test_conn).grid(\
                                            column = 0, row = 0, stick=tk.SE)
        ttk.Label(send_frame, textvariable = self.conn_status).grid(column=1,\
                                            row = 0, sticky = tk.SE)
        ttk.Button(send_frame, text='Send', command=self.on_send).grid(
                                            column=2, row=0, sticky=tk.SE)
        
        self.conn_status.set('            ')       
        
        # add padding around widgets
        for child in mainframe.winfo_children(): 
            child.grid_configure(padx=10, pady=5)
            
        for child in checkbox_frame.winfo_children(): 
            child.grid_configure(padx=30, pady=5)
            
        for child in SR_frame.winfo_children(): 
            child.grid_configure(padx=40, pady=5)
            
        self.active_frame = root
        
    def open_play_pause_GUI(self, play_pause):
        
        root = tk.Toplevel()
        root.title("Data Collection")
        root.columnconfigure(0, weight=1)
        root.rowconfigure(0, weight=1)
        
        root.protocol("WM_DELETE_WINDOW", self.collect_close_handler)
        
        #mainframe
        mainframe = ttk.Frame(root, padding="10 10 12 12")
        mainframe.grid(column=0, row=0, sticky=tk.NSEW)
        mainframe.columnconfigure(0, weight=1)
        mainframe.columnconfigure(1, weight=1)
        mainframe.rowconfigure(0, weight=1)
        mainframe.rowconfigure(1, weight=1)
        mainframe.rowconfigure(2, weight=1)
        
        #buttons
        self.play_button = ttk.Button(mainframe, text=play_pause,
                    command=self.on_play)
        self.play_button.grid(column=0, row=0, sticky=tk.EW)
        
#        self.plot = SimplePlot(mainframe)
#        self.plot.grid(row = 1, column = 0)
#        self.plot.pack(fill = "both", expand = 1)
#        self.plot.update()
        
        ttk.Button(mainframe, text='Stop', command=self.on_stop).grid(
                                            column=1, row=0, sticky=tk.EW)
        
        for child in mainframe.winfo_children(): 
            child.grid_configure(padx=5, pady=20)
        
        if(play_pause == "Play"):
            self.reader = GSS_reader(self.com_port, self.filename.get(), 'PAUSE')
        else:
            self.reader = GSS_reader(self.com_port, self.filename.get(), 'RUN')
            
        self.reader.start()
        self.active_frame = root;
         
    def run(self):
        self._root.mainloop()
             
#   Call backs for Open COM GUI (root) 
    def on_open(self, *args):
        if(self.com_name_var.get() == ''):
            tk.messagebox.showerror('No COM Selected', 'Please select a COM port before continuing')
            return;
        
        self.com_port = openPort(self.com_name_var.get())
        self.testButton.state(['!disabled'])
        self.nextButton.state(['!disabled'])
    def on_test(self, *args):
        if(testPort(self.com_port)):
            self.test_result.set('Test Success')
        else:
            self.test_result.set('Test Failed')         
    def on_next(self, *args):        
        self.open_config_GUI()
        self._root.withdraw()    
        
#     Call backs for Configurations GUI
    def on_send(self):
        axis_field = int(self.A0.get())*0x01
        axis_field += int(self.A1.get())*0x02
        axis_field += int(self.A2.get())*0x04
        axis_field += int(self.A3.get())*0x08
        axis_field += int(self.A4.get())*0x10
        axis_field += int(self.A5.get())*0x20
        axis_field += int(self.A6.get())*0x40
        axis_field += int(self.A7.get())*0x80
 
        if self.sampling_rate.get() == 'Custom' :
            self.sampling_rate.set(self.custom.get())
            
        SR = int(self.sampling_rate.get())
        BOR = int(self.BOR.get())
        
        payload = (0x53, axis_field, SR, BOR);
        checksum = fletcher(payload)
        #print(payload[0], payload[1], payload[2],payload[3], checksum)
        cmd = tuple(struct.pack('<BBBBH', payload[0], payload[1], payload[2],\
                                payload[3], checksum))
        #print("sending init:"+str(cmd))

        
        # TODO: write JSON file output here
        self.sess_serial = getSessSerial(self.com_port)
        self.node_id = getNodeID(self.com_port)
        metadata_dict = {'Node ID':self.node_id, 'Sess Serial':self.sess_serial,\
                         'Channel Selection':axis_field, 'Sampling Rate':SR, \
                         'Bits of Representation':BOR, 'Timestamp': time.asctime(time.gmtime())}
        metafilename = 'data/metaN'+str(self.node_id)+'S'+str(self.sess_serial)+'.json'
        metafile = open(metafilename, 'w')
        json.dump(metadata_dict, metafile)
        metafile.close()
        
        self.com_port.flushInput()
        self.com_port.write(cmd)
        
        if self.com_port.read(1) != b'S':
            os.remove(metafilename)
            return
        
        if(self.start_collection.get() == "1"):
            play_state = "Pause"
        else:
            play_state = "Play"
        
        self.active_frame.destroy()
        self.open_play_pause_GUI(play_state)
      
        
#callbacks for the pause/play/stop GUI    
    def test_conn(self):
        if testPort(self.com_port):
            self.conn_status.set('Connected')
        else:
            self.conn_status.set('Disconnected')
    def update_SR(self, rate):
        if(self.sampling_rate.get() == 'Custom'):
            self.custom.set('')
            self.custom_SR_entry.state(['!disabled'])
        else:
            self.custom_SR_entry.state(['disabled'])
            self.custom.set('custom rate')
    def autofile(self):
        serNum = getSessSerial(self.com_port)
        if serNum == 0:
            self.filename.set('data/out.dat.gz')
        else:
            self.filename.set('data/out%(sessNum)i.dat.gz' % {"sessNum":self.sess_serial})
    def config_close_handler(self):
        self._root.quit()
        
#   Call backs for Collection GUI
    def on_play(self):       
        if (self.play_button['text'] == "Play"):
            self.play_button['text'] = "Pause"
            self.com_port.flushInput()
            self.com_port.write(b'C')
            self.reader.resume()
        else:
            self.play_button['text'] = "Play"
            self.com_port.write(b'P')
            self.reader.pause()
        pass
    def on_stop(self):
        self.com_port.write(b'R')
        self.reader.kill()
        self.active_frame.destroy()
        self.open_config_GUI()
    def collect_close_handler(self):
        if tk.messagebox.askokcancel('End Collection', 'Are you sure you want to quit?'):
            self.reader.kill()
            self._root.quit()
        else:
            pass          

if __name__ == "__main__":
    app = GUI()
    app.run()          



        
        
                
        
    

        
    
    
        
        
        
