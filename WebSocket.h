/*
        Websocketduino, a websocket implementation for webduino
	Copyright 2010 Ben Swanson

	Based on a previous implementation:
	Copyright 2010 Randall Brewer
	and
	Copyright 2010 Oliver Smith
	
        Some code and concept based off of Webduino library
        Copyright 2009 Ben Combee, Ran Talbott

        Permission is hereby granted, free of charge, to any person obtaining a copy
        of this software and associated documentation files (the "Software"), to deal
        in the Software without restriction, including without limitation the rights
        to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
        copies of the Software, and to permit persons to whom the Software is
        furnished to do so, subject to the following conditions:

        The above copyright notice and this permission notice shall be included in
        all copies or substantial portions of the Software.

        THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
        IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
        FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
        AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
        LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
        OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
        THE SOFTWARE.
	
        -------------
	Now based off
	http://www.whatwg.org/specs/web-socket-protocol/
	
      - OLD -
	  Currently based off of "The Web Socket protocol" draft (v 75):
        http://tools.ietf.org/html/draft-hixie-thewebsocketprotocol-75
 */

 #include "MD5.c"
 
#ifndef WEBSOCKET_H_
#define WEBSOCKET_H_
// CRLF characters to terminate lines/handshakes in headers.
#define CRLF "\r\n"
// Include '#define DEBUGGING true' before '#include <WebSocket.h>' in code to
// enable serial debugging output.
#ifndef DEBUGGING
#define DEBUGGING false
#endif
// Amount of time (in ms) a user may be connected before getting disconnected 
// for timing out (i.e. not sending any data to the server).
#define TIMEOUT_IN_MS 2000
#define BUFFER 32
// ACTION_SPACE is how many actions are allowed in a program. Defaults to 
// 5 unless overwritten by user.
#ifndef ACTION_SPACE
#define ACTION_SPACE 5
#endif
#define SIZE(array) (sizeof(array) / sizeof(*array))
#include <stdlib.h>

class WebSocket {
public:
    // Constructor for websocket class.
    WebSocket(uint8_t *ip, const char *urlPrefix = "/", int port = 8080);
    // Processor prototype. Processors allow the websocket server to
    // respond to input from client based on what the client supplies.
    typedef void Action(WebSocket &socket, String &socketString);
    // Start the socket listening for connections.
    void begin();
    // Handle connection requests to validate and process/refuse
    // connections.
    void connectionRequest();
    // Loop to read information from the user. Returns false if user
    // disconnects, server must disconnect, or an error occurs.
    void socketStream(int socketBufferLink);
    // Adds each action to the list of actions for the program to run.
    void addAction(Action *socketAction);
    // Custom write for actions.
    void actionWrite(const char *str);
		
private:
    Server socket_server;
    Client socket_client;

    const char *socket_urlPrefix;
    byte ipAddress[4];
    int port;

    bool socket_reading;
	
	String origin;

    struct ActionPack {
        Action *socketAction;
        // String *socketString;
    } socket_actions[ACTION_SPACE];
	
    int socket_actions_population;

    // Discovers if the client's header is requesting an upgrade to a
    // websocket connection.
    bool analyzeRequest(int bufferLength);
    // Disconnect user gracefully.
    void disconnectStream();
    // Send handshake header to the client to establish websocket
    // connection.
    void sendHandshake();
    // Essentially a panic button to close all sockets currently open.
    // Ideal for use with an actual button or as a safetey measure.
    // void socketReset();
    // Returns true if the action was executed. It is up to the user to
    // write the logic of the action.
    void executeActions(String socketString);

    String processString(char hand[]);
};

WebSocket::WebSocket(byte ip[], const char *urlPrefix, int inPort) :
socket_server(inPort),
socket_client(255),
socket_actions_population(0),
socket_urlPrefix(urlPrefix)
 {
    ipAddress[0]=ip[0];
    ipAddress[1]=ip[1];
    ipAddress[2]=ip[2];
    ipAddress[3]=ip[3];
    port=inPort;
}

void WebSocket::begin() {
    socket_server.begin();
}

void WebSocket::connectionRequest() {
    // This pulls any connected client into an active stream.
    socket_client = socket_server.available();
    int bufferLength = BUFFER;
    int socketBufferLength = BUFFER;

    // If there is a connected client.
    if (socket_client.connected()) {
        // Check and see what kind of request is being sent. If an upgrade
        // field is found in this function, the function sendHanshake(); will
        // be called.
		#if DEBUGGING
        Serial.println("*** Client connected. ***");
		#endif
        if (analyzeRequest(bufferLength)) {
            // Websocket listening stuff.
            // Might not work as intended since it may execute the stream
            // continuously rather than calling the function once. We'll see.
		#if DEBUGGING
            Serial.println("*** Analyzing request. ***");
		#endif
            // while(socket_reading) {
		#if DEBUGGING
            Serial.println("*** START STREAMING. ***");
		#endif
            socketStream(socketBufferLength);
		#if DEBUGGING
            Serial.println("*** DONE STREAMING. ***");
		#endif
            // }
        } else {
            // Might just need to break until out of socket_client loop.
		#if DEBUGGING
            Serial.println("*** Stopping client connection. ***");
		#endif
            disconnectStream();
        }
    }
}

bool WebSocket::analyzeRequest(int bufferLength) {
    // Use TextString ("String") library to do some sort of read() magic here.
   // String headerString = String(bufferLength);
	String temp = String(60);
	//unionchal uc;
    char bite;
	bool foundupgrade = false;
	String key[2];
	unsigned long intkey[2];
	#if DEBUGGING
		Serial.println("*** Building header. ***");
	#endif
    while ((bite = socket_client.read()) != -1) {
        //headerString += bite;
       
		temp += bite;
		
		if(bite == '\n'){
			#if DEBUGGING
				Serial.println("Got Line" + temp);
			#endif
			
			if(!foundupgrade && temp.startsWith("Upgrade: WebSocket")) {
				foundupgrade = true;	
			#if DEBUGGING
				Serial.println("foundupgrade");
			#endif				
			}
			
			if(temp.startsWith("Origin: ")) {
				#if DEBUGGING
				Serial.println("found Origin!");
				#endif		
				origin=temp.substring(8,temp.length());
			}
			
			if(temp.startsWith("Sec-WebSocket-Key1")) {
				#if DEBUGGING
				Serial.println("found key1!");
				#endif		
				  key[0]=temp.substring(20,temp.length());
			}
			
			if(temp.startsWith("Sec-WebSocket-Key2")) {
				#if DEBUGGING
				Serial.println("found key2!");
				#endif	
				key[1]=temp.substring(20,temp.length());
			}
			temp = "";		
		}
    }
	
	temp+= 0;//Null character needed for wstring lib

	
	char key3[9] = {0};


	temp.toCharArray(key3, 9);
	
	//processkeys
		
	for (int i=0;i<=1;i++){
		unsigned int spaces =0;
		String numbers;
		
		#if DEBUGGING
			Serial.println("Process: "+key[i]);
		#endif	
		
		for(int c=0;c<key[i].length();c++){

			char ac = key[i].charAt(c);
			if(ac >= '0' && ac <= '9'){
				numbers+=ac;
			}
			
			if(ac == ' '){
				spaces++;
			}
	
		}

		
		char numberschar[numbers.length()+1];
		
		numbers.toCharArray(numberschar, numbers.length()+1);
		#if DEBUGGING
		Serial.println(strtoul(numberschar, NULL, 10));
		Serial.println(spaces);
		#endif	
		
		if (spaces > 0 && (strtoul(numberschar, NULL, 10) % spaces) == 0){
			#if DEBUGGING
				Serial.println("GOOD");
			#endif	
		}else{
			#if DEBUGGING
				Serial.println("BAD");
			#endif	
		}
		
		
		intkey[i] = strtoul(numberschar, NULL, 10) / spaces;		
		#if DEBUGGING
			Serial.println(intkey[i], DEC);
		#endif	
	}
	
	int x=0;

	unsigned char challenge[16] = {0};
	
	//Big Endian
	
	challenge[0] = (unsigned char)((intkey[0] >> 24) & 0xFF);
	challenge[1] = (unsigned char)((intkey[0] >> 16) & 0xFF);
	challenge[2] = (unsigned char)((intkey[0] >>  8) & 0xFF);
	challenge[3] = (unsigned char)((intkey[0]      ) & 0xFF);	
	
	challenge[4] = (unsigned char)((intkey[1] >> 24) & 0xFF);
	challenge[5] = (unsigned char)((intkey[1] >> 16) & 0xFF);
	challenge[6] = (unsigned char)((intkey[1] >>  8) & 0xFF);
	challenge[7] = (unsigned char)((intkey[1]      ) & 0xFF);

	memcpy(challenge + 8, key3, 8);

	#if DEBUGGING
	Serial.println("Response");
	for(x=0;x<16;x++){
		Serial.print(challenge[x],HEX);
	}
	 Serial.println("");
	#endif	
	MD5(challenge, 16); 
	#if DEBUGGING  
	for(x=0;x<16;x++){
		Serial.print(MD5Digest[x],HEX);
	}
			  Serial.println("");	
	#endif	
    if(foundupgrade) {
	#if DEBUGGING
		Serial.println("*** Upgrade connection! ***");
	#endif
		sendHandshake();
	#if DEBUGGING
		Serial.println("*** SETTING SOCKET READ TO TRUE! ***");
	#endif
		socket_reading = true;
		return true;
    }else {
		#if DEBUGGING
    		Serial.println("Header did not match expected headers. Disconnecting client.");
    	#endif
    	return false;
    }
}

// This will probably have args eventually to facilitate different needs.

void WebSocket::sendHandshake() {


#if DEBUGGING  
    Serial.println("*** Sending handshake. ***");
#endif
    socket_client.write("HTTP/1.1 101 Web Socket Protocol Handshake");
    socket_client.write(CRLF);
    socket_client.write("Upgrade: WebSocket");
    socket_client.write(CRLF);
    socket_client.write("Connection: Upgrade");
    socket_client.write(CRLF);
    socket_client.write("Sec-WebSocket-Origin: ");
	char corigin[origin.length() -1];
	origin.toCharArray(corigin, origin.length() -1);
		
    socket_client.write(corigin);
    socket_client.write(CRLF);
    socket_client.write("Sec-WebSocket-Location: ws://");

    //Assign buffer for conversions
    char buf5[10];
	
    //Write the ip address
    for(int i=0; i<4; i++)
    {
		socket_client.write(itoa(ipAddress[i],buf5,10));
		if(i!=3)
		{
			socket_client.write('.');
		}
    }
    socket_client.write(":");
    socket_client.write(itoa(port,buf5,10));
    socket_client.write("/");
	socket_client.write(CRLF);
	socket_client.write("Server: Arduino");
    socket_client.write(CRLF);
    socket_client.write(CRLF);
	socket_client.write((const uint8_t *)MD5Digest, 16);
	#if DEBUGGING
		Serial.println("*** Handshake done. ***");
	#endif
}

void WebSocket::socketStream(int socketBufferLength) {
    while (socket_reading) {
	#if DEBUGGING
       Serial.println("*** READ ***");
	#endif
        char bite;

        unsigned long timeoutTime = millis() + TIMEOUT_IN_MS;
		
		if(!socket_client.connected()) {
		
			#if DEBUGGING
			Serial.println("*** DISCONNECT ***");
			#endif
			socket_reading = false;
			return;
		}

		String socketString = "";
		
        // While there is a client stream to read...
        while ((bite = socket_client.read()) && socket_reading) {
            // Append everything that's not a 0xFF byte to socketString
			
			if((uint8_t) bite != 0xFF) {
                socketString += bite;
            } else {
				
				if(socketString.length() > 0){
					break;
				}
			
                // Timeout check.			
                unsigned long currentTime = millis();
                if ((currentTime > timeoutTime) && !socket_client.connected()) {
					#if DEBUGGING
                    Serial.println("*** CONNECTION TIMEOUT! ***");
					#endif
                    disconnectStream();
                }
            }
        }
        // Assuming that the client sent 0xFF, we need to process the String.
        // NOTE: Removed streamWrite() in favor of executeActions(socketString);
        executeActions(socketString);
    }
}

void WebSocket::addAction(Action *socketAction) {
#if DEBUGGING
    Serial.println("*** ADDING ACTIONS***");
#endif
    if (socket_actions_population <= SIZE(socket_actions)) {
        socket_actions[socket_actions_population++].socketAction = socketAction;
    }
}

void WebSocket::disconnectStream() {
#if DEBUGGING
    Serial.println("*** TERMINATING SOCKET ***");
#endif
    socket_reading = false;
    socket_client.flush();
    socket_client.stop();
    socket_client = false;
#if DEBUGGING
    Serial.println("*** SOCKET TERMINATED! ***");
#endif
}

void WebSocket::executeActions(String socketString) {
    int i;
#if DEBUGGING
    Serial.print("*** EXECUTING ACTIONS ***");
    Serial.print(socket_actions_population);
    Serial.print("\n");
#endif
    for (i = 0; i < socket_actions_population; ++i) {
#if DEBUGGING
        Serial.print("* Action ");
        Serial.print(i);
        Serial.print("\n");
#endif

        socket_actions[i].socketAction(*this, socketString);
    }
}

void WebSocket::actionWrite(const char *str) {
#if DEBUGGING
    Serial.println(str);
#endif
    if (socket_client.connected()) {
        socket_client.write((uint8_t) 0x00);
        socket_client.write(str);
        socket_client.write((uint8_t) 0xFF);
    }

}
#endif