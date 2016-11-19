import socket
import sys

HOST = '192.168.1.104'      # Symbolic name meaning the local host
PORT = 10000            # Arbitrary non-privileged port

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.bind((HOST, PORT))
s.listen(1)

while True:
    # Wait for a connection
    print >>sys.stderr, 'waiting for a connection'
    connection, client_address = s.accept()
    try:
        print >>sys.stderr, 'connection from', client_address

        # Receive the data in small chunks and retransmit it
        while True:
            data = connection.recv(1024)
            print >>sys.stderr, 'received "%s"' % data
            if data:
                print >>sys.stderr, 'sending data back to the client'
                # connection.sendall(data)
                connection.send("$IREMP*44")
            else:
                print >>sys.stderr, 'no more data from', client_address
                break
    finally:
        # Clean up the connection
        connection.close()

# while 1:
# 	# print "Wait for incoming data..."
# 	data = conn.recv(1024)
# 	if data:
# 		print "Receive:{0}".format(data)
# 		conn.send("$IREMP*44")
# conn.close()
