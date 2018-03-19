#Brandon Lo
#CS372 Project 2
#ftclient.py
#11/26/17
#Create a client that can send files and check what is in a directory of the server
import os
import socket
import sys
import struct
import time
import SocketServer

#gets the server data from the command port
#format taken from here
#https://docs.python.org/2/library/socketserver.html
class MyTCPHandler(SocketServer.BaseRequestHandler):
    action, port, fileName, hostName = "" ,"" ,"" ,""
    #use a handler class to communicate with the server
    #https://docs.python.org/2/library/socket.html
    def handle(self):
        #grabs host by address
        self.hostName = socket.gethostbyaddr(self.client_address[0])[0]
        #if the list command is used then list all the files
        if self.action == "-l":
            #recieve the data from the list. set as same as the size of the list form the server, 1024
            self.data = self.request.recv(1024).strip()
            self.outputDirList()
        else:
            #recieve the data size from the socket and set the maximum amount of data to be recieved
            status = self.request.recv(2).strip()
            #command from the server where if the file is not found then output this to the client
            if (status == "e"):
                print "%s:%d says FILE NOT FOUND" % (self.hostName, self.port)
            #else if the file was found then recieve the file
            else:
                #check to see if the file exists
                #https://stackoverflow.com/questions/82831/how-do-i-check-whether-a-file-exists-using-python
                if (os.path.isfile(self.fileName)):
                    print "File exists, still recieving."
                #else use a try except loop
                try:
                    #recieve the file and output the message
                    self.data = self.getFiles()
                    self.saveTransferedFile()
                    print "File transfer complete."
                except Exception, e:
                    print "Error: In recieving file."
    #gets the directory list from the server and ouputs it
    def outputDirList(self):
        strLen = len(self.data)
        newList = ""
        #bug where \00 character appears, so a new list is created and iterated through
        for char in self.data:
            #iterates through the server's list and doesn't grab the \x00 characters
            if char != '\x00':
                newList += char
        #splits the list with commas
        dirList = newList.split(",")
        #output the directory
        print "Receiving directory structure from %s:%d" % (dirList[0], self.port)
        for i in range(1, len(dirList)):
            print dirList[i]
    #reads the server stream until null is grabbed
    #http://stackoverflow.com/questions/27241804/sending-a-file-over-tcp-sockets-in-python
    #https://stackoverflow.com/questions/29058163/sending-files-between-client-server-through-tcp-socket-in-python
    def getFiles(self):
        print "Receiving \"%s\" from %s:%d" % (self.fileName, self.hostName, self.port)
        data = ''
        currentFiles=[]
        data = self.request.recv(100)
        #while loop, while the data is being sent
        while data:
            currentFiles.append(data)
            data = self.request.recv(100)
        return ''.join(currentFiles)

    #once the data is grabbed then close the file and save it
    #http://stackoverflow.com/questions/27241804/sending-a-file-over-tcp-sockets-in-python
    #https://stackoverflow.com/questions/29058163/sending-files-between-client-server-through-tcp-socket-in-python
    def saveTransferedFile(self):
        saveFile = open(self.fileName, "w")
        saveFile.write(self.data)
        saveFile.close()

#creates the data connection to the server
#format taken from here
#https://docs.python.org/2/library/socketserver.html
def createDataConn(argv):
    MyTCPHandler.action = argv[3]
    MyTCPHandler.port = checkPort(argv)
    MyTCPHandler.fileName = argv[4]
    HOST, PORT = argv[1], checkPort(argv)
    ## Create the server, binding to the inputted host and inputted port
    dataSocket = SocketServer.TCPServer((HOST, PORT), MyTCPHandler)
    return dataSocket

#starts the connectin adnd sends the server the connection information
#format taken from here
#https://docs.python.org/2/library/socketserver.html
def startConnection(argv):
    host = sys.argv[1]
    port = int(sys.argv[2])
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        s.connect((host, port))
    except socket.error:
        print "Error: Either host name/command port number incorrect."
        sys.exit()
        s.close()
    return s

#sends the commands to the server
def makeRequest(s, argv):
    sendString = GenerateServerCommand(argv);
    try:
        s.send(sendString)
    except:
        print "Error: Server disconnected."
        sys.exit()
        s.close()

#creates the format for the server to grab the commands based on the number of arguements grabbed
def GenerateServerCommand(argv):
    if len(argv) == 5:
        return argv[3] + "," + argv[4] + "," + socket.gethostname() + "\n";
    elif len(argv) == 6:
        return argv[3] + "," + argv[5] + "," + argv[4] + "," + socket.gethostname() + "\n";
    else:
        return "";

#checks if the arguements are correctly inputted
def checkArgs(argv):
    port, serverPort, dataPortNumber = 0, 0, 0
    # Check that the correct number of arguments are sent in
    if len(argv) != 5 and len(argv) != 6:
        print 'Error: Invalid input commands, please check README.'
        return False

    if len(argv) == 5:
        port = argv[4]
    if len(argv) == 6:
        port = argv[5]
    #convert inputted string input to integer
    #https://stackoverflow.com/questions/642154/how-to-convert-strings-into-integers-in-python
    #try except loop to catch if a non integer is inputted
    try:
        serverPort = int(argv[2])
        dataPortNumber = int(port)
    except:
        print 'Error: One or more ports invalid.'
    #check if the
    if (argv[3] != '-l') and (argv[3] != '-g'):
        print 'Error: Invalid input commands, please check README.'
        return False
    return True

#checks and grabs the port number depending on the number of arguements given
def checkPort(argv):
    if len(argv) == 5:
        return int(argv[4]);
    elif len(argv) == 6:
        return int(argv[5]);
    else:
        return -1;

#main function
if __name__ == '__main__':
    #checks the system arguements
    if (not checkArgs(sys.argv)):
        sys.exit()
    #set up the connection for commands
    commandSocket = startConnection(sys.argv)
    #sets up the socket for data
    dataSocket = createDataConn(sys.argv)
    #try except loop to allow for commands to run at least once
    try:
        makeRequest(commandSocket, sys.argv)
        dataSocket.handle_request()
        #close the sockets after use
        commandSocket.close()
        dataSocket.server_close()
        sys.exit()
    #close the sockets on KeyboardInterrupt
    #
    except KeyboardInterrupt:
        #close the sockets after use
        commandSocket.close()
        dataSocket.server_close()
        sys.exit()
