import numpy as np
import serial
import numpy
import serial.tools.list_ports
import time
from faceRec import load_img_from_vector, init, check

ports = list(serial.tools.list_ports.comports())
for i in ports:
    print(i.name)

ser = serial.Serial('COM3', 115200, timeout=0.01, bytesize=8, stopbits=1)

i = 0
pic_buffer = numpy.zeros((76800,1), dtype=int)
H = 240
W = 320
C = 3
no_recv = 0
save_path = './pic.npy'
init()
print("Ready!")
while True:
    recv = ser.readline()
    if recv != b'':
        if len(recv) > 8:
            print(recv.decode('gbk'))
        else:
            if i%10000 == 0:
                print("No.", i, ", receive:", recv.decode())
            pic_buffer[i, 0] = recv
            i += 1
            no_recv = 0
    else:
        # print("No.", i, ", empty_", no_recv)
        no_recv += 1
        if no_recv%1000 == 0 :
            print("empty_", no_recv)
    if i == 76800:
        np.save("./pic.npy", pic_buffer)
        i = 0
        img = load_img_from_vector(H, W, C, save_path)
        res, score = check(img)
        data = b'11111'
        if res == -1:
            data = b'00000'
        for i in range(10):
            ser.write(data)
    if no_recv == 10000:
        break

ser.close()
