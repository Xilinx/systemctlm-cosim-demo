import sys
import socket
import select

print("HELLO")
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.settimeout(2)
try :
	s.connect(("0.0.0.0", 5001))
except :
	print("Unable to connect")
	sys.exit()
s.send("test")
