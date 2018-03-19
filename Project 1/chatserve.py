#Brandon Lo
#CS372 Project 1
#chatserve.cpp
#10/29/17
#Create a server for the chatclient to communicate with. Can create sockets and terminate the client
from socket import *
import sys

#creates the client and starts the chat
def createClient(connection,handle,serverName):
    # initialize the variable
    chatBuffer = ""
    #use while 1 code taken from bottom for simple server
    #https://docs.python.org/2/library/socket.html
    while 1:
        #recieve the message
        #[:-1] is used in order to not grab the \n of the message, so there are no spaces in between the message and the server
        message = connection.recv(501)[:-1]
        if message == "":
            print "Connection Closed"
            break
        #once the message was recieved then print out what was grabbed
        print handle + "> " + message

        #create the buffer for the server to talk with
        chatBuffer = ""
        #check if the maximum message size was reached and if it is empty
        while len(chatBuffer) == 0 or len(chatBuffer) > 500:
            chatBuffer = raw_input(serverName + "> ")
        #once input recieved check if the server wants to quit the connection
        if chatBuffer == "\quit":
            print "Connection Closed, Now Waiting For A Response"
            break
        #send the message
        connection.send(chatBuffer)

#create the hand shake in order send and recieve the handles and serverNames
#https://docs.python.org/2/library/socket.html
#Hostname is the client handle, while handle is the server's name
def createHandshake(connection,handle):
    hostName = connection.recv(1024)
    connection.send(handle)
    return hostName

#use sys to check if the correct number of arguements used and creation of the ports is correct
#how to use sys to read arguments
#ceoncepts taken from there
#http://www.pythonforbeginners.com/system/python-sys-argv
def checkArguments():
    if len(sys.argv) != 2:
        print "Proper usage: python chatserver.py [PORT NUMBER]"
        exit(1)
    if int(sys.argv[1]) < 1 or int(sys.argv[1]) > 65535:
        print "Please select a valid port number between 1-65535."
        exit(1)

#main function
if __name__ == '__main__':
    checkArguments()
    #grab the port since argv[0] is the file name
    port = int(sys.argv[1])
    #binding the socket and listen for the client
    #socket code taken from in order to figure out how to create, bind, and listen for messages with sockets
    #https://docs.python.org/2/howto/sockets.html
    s = socket(AF_INET, SOCK_STREAM)
    s.bind(('', port))
    s.listen(1)
    #hard coding the server's handle since it is allowed
    handle = "serverBot"
    print "Waiting for Response"
    #create the chat
    while 1:
        connection, address = s.accept()
        print "Connection established with " + str(address[0])
        createClient(connection, createHandshake(connection, handle),handle)
        connection.close()
