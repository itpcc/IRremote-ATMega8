# Socket server in python using select function
# http://www.binarytides.com/python-socket-server-code-example/
import socket, select
import re
# telnet program example
import socket, select, string, sys

def NMEA_CRC(InString):
    crc = ord(mycopy[0:1])
    for n in range(1,len(mycopy)-1):
        crc = crc ^ ord(mycopy[n:n+1])
    return '%X' % crc

def nmea_format(sentence):
    return "${0}*{1}\r\n".format(sentence, NMEA_CRC(sentence))
 
#!/usr/bin/python
import socket
import select
import sys

__author__ = 'Athanasios Garyfalos'

# Tcp Chat server


def check_argument_input(argument_input_list):

    if len(argument_input_list) != 2:
        print 'Usage : python {} [IP:PORT]' .format(argument_input_list[0])
        sys.exit(1)
    elif ":" not in argument_input_list[1]:
        print 'Usage : [IP:PORT] ({})' .format(argument_input_list[1])
        sys.exit(1)
    elif argument_input_list[1].count(':') > 1:
        print 'Usage : [IP:PORT] ({})' .format(argument_input_list[1])
        sys.exit(1)

    host_name_and_port = argument_input_list[1].split(":")

    if not host_name_and_port[0] or \
            not host_name_and_port[1]:
        print 'Usage : python {} [IP:PORT]' .format(argument_input_list[0])
        sys.exit(1)

    if not host_name_and_port[1].isdigit() or \
        int(host_name_and_port[1]) < 0 or \
            int(host_name_and_port[1]) > 65535:
        print 'Please enter a valid port number : ({})' .format(host_name_and_port[1])
        sys.exit(1)

    try:
        socket.inet_aton(host_name_and_port[0])
    except socket.error:
        print 'Please use a valid IP syntax: {}' .format(host_name_and_port[0])
        sys.exit(1)

    return host_name_and_port[0], int(host_name_and_port[1])
    
if __name__ == "__main__":

    BUFFER_RCV = 512
    MAX_BUFFER_RCV = 255

    host, port = check_argument_input(sys.argv)

    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # this has no effect, why ?
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind((host, port))
    server_socket.listen(10)

    # List to keep track of socket descriptors
    CONNECTION_LIST = list([])
    # Add server socket to the list of readable connections
    CONNECTION_LIST.append(server_socket)

    print "Chat server started on port " + str(port)

    # Initialization to bind the port with the name
    dictionary = {}

    while 1:
        # Get the list sockets which are ready to be read through select
        read_sockets, write_sockets, error_sockets = select.select(CONNECTION_LIST,
                                                                   [],
                                                                   [])

        for sock in read_sockets:
            # New connection
            if sock == server_socket:
                # Handle the case in which there is a new connection received through server_socket
                socket_fd, address = server_socket.accept()
                CONNECTION_LIST.append(socket_fd)
                socket_fd.send('HELLO My Friend\n')
                print "Client ({}, {}) connected" .format(address[0], address[1])
            # Some incoming message from a client
            else:
                # Data received from client, process it
                data = sock.recv(BUFFER_RCV)
                data = data.rstrip('\r\n')
                broadcastingHost, broadcastingSocket = sock.getpeername()
                if len(data) > MAX_BUFFER_RCV:
                    sock.send('ERROR please send less than 256 characters!\n')
                else:
                    print "Send: "+data
                    sock.send("\r\n" + "You send: " + data + "\n")

    server_socket.close()
    sys.exit(0)
