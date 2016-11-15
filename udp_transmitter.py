import socket, logging, struct, time
import matplotlib.pyplot as plt
import numpy as np

while 1:
	# create socket
	s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)

	# set socket broadcast options
	s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, True)

	# send data to socket
	s.sendto('wifi-serial:\r\n'.encode(), ('192.168.1.255', 6000))

	# # try to get response
	# re = s.recv(1024)

	print "Sent!"

	# if (re):
	# 	print 'Response after UDP broadcast transmission: {}'%str(re)
	# close socket
	s.close()
	time.delay(1000)