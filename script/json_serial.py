import serial
import time

ser = serial.Serial('COM8', 115200)

json_data = '{"lines":[{"logic":{"type":"CONTACT","pin":"I0","mode":"NC"},"outputs":[{"pin":"Q0","type":"COIL"}]}]}'

message = "<START>" + json_data + "<END>"

ser.write(message.encode())

ser.close()