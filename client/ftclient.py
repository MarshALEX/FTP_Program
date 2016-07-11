#!/user/bin/python

#  Alex Marsh
# 05/24/2016
# CS 372 Project 2
# written in python

#usage: python ftclient [hostname] [server\'s port number] [-l | -g] [client\'s port number]

#Code taken from resources: 
#
#   Beej's Guide to Network Programming
#            http://beej.us/guide/bgnet/output/html/multipage/index.html
#   Kurose and Ross, Computer Networking: A Top-Down Approach, 6th Edition, Pearson
#            Chapter 2.3 and 2.7 Application Layer Socket Programming

#This is a client program simulating the FTP that will connect to a specified server on a specific port number that is entered in the command line.  The client will request the directory listings (-l) ora specific file to be sent (-g). After one request the client program terminates.  The client send a request over a control port and then will recieve the directory listings or file contents over a data port specified by the user on the command line

import sys
import time
import random
import json
import math
import string
import socket
import os.path
#from socket import *


#---------validateInput----------
#
#This function takes in the user input from command line
#and validates that the paramaters are met
#This function does not return anything
#
def validateInput():
    answer = 0;
    
    #check number of arguments
    if (len(sys.argv) < 3):
        sys.exit('Expected Format: python ftclient [hostname] [server\'s port number] [-l | -g] [client\'s port number]')
        
        
    #check correct command is requested
    elif( (sys.argv[3] != '-g') & (sys.argv[3] != '-l')):
        print 'Wrong command requested'
        print 'sys.argv[3] is ', sys.argv[3]
        sys.exit( 'Expected Format: python ftclient [hostname] [server\'s port number] [-l | -g] [client\'s port number]')
    
    #check correct command for list
    if(sys.argv[3] == '-l'):
        if (len(sys.argv) < 5):
            sys.exit( 'Expected Format: python ftclient [hostname] [server\'s port number] [-l | -g] [client\'s port number]')
        
    #check correct command for get
    if(sys.argv[3] == '-g'):
        if (len(sys.argv) < 6):
            sys.exit( 'Expected Format: python ftclient [hostname] [server\'s port number] [-l | -g] [client\'s port number]')    
        
        #check if the file exists in current directory 
        if(os.path.isfile(sys.argv[4])):
            #if the file already exists, ask user if they would
            #like to overwrite it's contents
            print("That file already exists in your directory."); 
            print("Would you like to overwrite this file?");
            print("1.) Yes");
            print("2.) No");
            print("Please Enter 1 or 2 for your answer.");
            #get user input
            while((answer != 1) & (answer != 2)):
                answer = input(); 
            #if the user doesn't want to overwrite
            #exit function
            if(answer == 2):
                sys.exit()
    
    return    

#---------setUp----------
#
#This functino sets up a TCP socket to connect to
#the server input from command line
#This function makes the connection and then returns the 
#the connection object 'clientSocket'
def setUp(portnum):
    #get server name user wants to connect to
    serverName = sys.argv[1]
    
    #get port number user wants to connect to
    serverPortNum = int(sys.argv[2])

    #create client socket (use SOCK_STREAM for TCP)
    clientSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    
    #initiate TCP connection
    clientSocket.connect((serverName, portnum))
    
    
    return clientSocket

#---------setUpDataPort----------
#
#This function takes in the dataport number the client
#wishes to create a socket on. The port is set as a socket
#and then is open to listen from connected server
#This function returns the connectionSocket object to the data port
def setUpDataPort(dataportnum):
    dataSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM) #create TCP socket
    dataSocket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    #associate the data port number with this socket
    dataSocket.bind(('',dataportnum)) 
    #listen for someone to connect, will only accept 1 person
    dataSocket.listen(1) 
    #print 'listening on data line...'
    #accept connection and save in connectionSocket
    connectionSocket, addr = dataSocket.accept()
    
    return connectionSocket

#---------reqToServer----------
#
#This function takes in the control socket object
#   and sends an initial request to the server depending
#   on the command given -l (list) or -g(get)
#This function does not return anything
def reqToServer(clientSocket):
    
    if(sys.argv[3] == '-l'):
        contrlPort = sys.argv[2] #get control port for server
        dataPort = sys.argv[4]  #get data port for sending files
        userCmd = sys.argv[3]  #get command from user

        #make message (-l[dataport])
        userReq = userCmd + dataPort 
        
        
    if(sys.argv[3] == '-g'):   
        contrlPort = sys.argv[2] #get control port for server
        userCmd = sys.argv[3]  #get command from user
        fileName = sys.argv[4] #get file name requested
        dataPort = sys.argv[5]  #get data port for sending files
        
        #make message (-g[dataport][fileName]
        userReq = userCmd + dataPort + fileName
        
    clientSocket.send(userReq) #send request to server
    
    return

#---------getListMsg----------
#
#This function recieves a list of items in the server's
#   directory from the data port and prints them to the screen.
#This function does not return anything
def getListMsg(dataSocket):
    
    print 'Receiving directory structure from ' + sys.argv[1] + ':' + sys.argv[4]
    #recv message
    #recvMsg = dataSocket.recv(1024)
    #print 'Expecting ' + recvMsg + ' items. '
    
    while 1:
        #recv message
        recvMsg = dataSocket.recv(1024)
        print recvMsg + ' '
        if not recvMsg: break
        
    return

#---------reqTgetFileoServer----------
#
#This function recieves a file from the server on the data connection and stores it in 
#  the clients current directory
#If the file requested does not exist in the servers directory, an error message will be 
#   recieved over the control port.
#This function returns nothing 
def getFile(dataSocket, controlSocket):
    errorFlag = False; 
           
    #if error recieve on control socket
    while 1:
        #recv message
        recvMsg = controlSocket.recv(1024)
        if not recvMsg: break #if no message then break from loop
        print sys.argv[1] + ':' + sys.argv[2]+ ' says ' + recvMsg 
        errorFlag = True;
    
    #if no error message from server open file
    if(errorFlag == False): 
        print 'Receiving "'+ sys.argv[4] + '" from ' + sys.argv[1] + ':' + sys.argv[5]
        file = open(sys.argv[4], "w")
        
    #recieve on data socket
    while 1:
        recvMsg = dataSocket.recv(1024)
        if not recvMsg: break #if no message then break from loop
        #write message to screen for testing
        #sys.stdout.write(recvMsg)
         #write message in to file
        file.write(recvMsg)
        
    #if error wasn't recieved after file transfer print complete message    
    if(errorFlag == False):
        print ' '
        print 'File transfer complete.'
        file.close()
        
    return
    
    
##-MAIN BODY--##
def mainFunc():
    
    
    validateInput(); #validate user input
    
    #get port number user wants to connect to
    portnum = int(sys.argv[2])
    #print 'serverPortNum is ', portnum
    
    #get data port number user requests
    if(sys.argv[3] == '-l'):
        #get port number user wants to connect to
        dataportnum = int(sys.argv[4])
        
    if(sys.argv[3] == '-g'):
        #get port number user wants to connect to
        dataportnum = int(sys.argv[5])
        
        
    #print 'Client Data PortNum is:* ', dataportnum
    
    clientSocket = setUp(portnum);  #set up the control connection
    
    #send command line input 
    reqToServer(clientSocket);
    
    #set up listening connection
    dataSocket = setUpDataPort(dataportnum)
    
    #check correct command for list
    if(sys.argv[3] == '-l'):
        getListMsg(dataSocket);
    
    if(sys.argv[3] == '-g'):
        getFile(dataSocket, clientSocket);
        
        
        
    
    #close socket to server
    dataSocket.close()
    clientSocket.close()
    
    return
mainFunc()


