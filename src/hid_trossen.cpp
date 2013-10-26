/**
 * hid-Trossen tool0
 * __author__		= Alexander Krause <alexander.krause@ed-solutions.de>
 * __version__	= 0.0.1
 * __date__			= 2013-09-17
 */

#include <unistd.h>

#include "boost/program_options.hpp"
#include <iostream>
#include <string>
#include <fstream>

#include <msgpack.hpp>
#include <vector>
#include <zmq.hpp>

#include <dynamixel.h>
#include <dynamixel-rtu.h>

#include "config.h"

#define DESCRIPTION "hid_trossen - Remote Controll Trossen bots via hid device"

#define 	TROSSEN_COMMANDER		0x200


/* hid stuff */
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>

#include <linux/input.h>
#include <linux/joystick.h>


#define HID_EVENT_NO_LEFT_X 2
#define HID_EVENT_NO_LEFT_Y 3

#define HID_EVENT_NO_RIGHT_X 0
#define HID_EVENT_NO_RIGHT_Y 1
namespace { 
	const size_t ERROR_IN_COMMAND_LINE = 1; 
	const size_t SUCCESS = 0; 
	const size_t ERROR_UNHANDLED_EXCEPTION = 2; 
}


int main(int argc, char** argv) {
	// === program parameters ===
	std::string zmq_uri="tcp://*:5555";
	std::string hid_device="/dev/hid1";
	bool debug=false;

	namespace po = boost::program_options;

  // Setup options.
  po::options_description desc("Options");
	desc.add_options()
		("help", "produce help message")
		("uri", po::value< std::string >( &zmq_uri ),					"ZeroMQ server uri | default: tcp://*:5555" )
		("dev", po::value< std::string >( &hid_device ),			"hid device        | default: /dev/hid1 ")
		("debug", "print out debugging info")
	;

  po::variables_map vm;
	try {
		po::store(po::parse_command_line(argc, argv, desc), vm); // can throw 

		po::notify(vm);  
			
		if (vm.count("help")) {
			std::cout << DESCRIPTION << " (version "<< VERSION << ")" << std::endl << desc << std::endl;
			//std::cout << "Features:" << std::endl;

			
			return SUCCESS; 
		}
		
		if (vm.count("debug")) {
			debug=true;
		}
	} catch(po::error& e) {
		std::cerr << "ERROR: " << e.what() << std::endl << std::endl; 
		std::cerr << desc << std::endl; 
		return ERROR_IN_COMMAND_LINE; 
	} 

	
	zmq::context_t context(1);
	zmq::socket_t socket(context, ZMQ_REQ);

	if (debug) {
		std::cout << "uri   = " << zmq_uri << std::endl; 
	}
	socket.connect (zmq_uri.c_str());
	
	std::vector<int> tx_vect(6,0);
	tx_vect[0]=TROSSEN_COMMANDER;
	
	int hid_fd;
	/* Open the Device with non-blocking reads. In real life,
	   don't use a hard coded path; use libudev instead. */
	hid_fd = open(hid_device.c_str(), O_RDWR);

	if (hid_fd < 0) {
		std::cerr << "ERROR: unable to open hid device"  << std::endl << std::endl; 
		return 1;
	}
	
	
	#define NAME_LENGTH 128
 
	unsigned char axes = 2;
	unsigned char buttons = 2;
	int version = 0x000800;
	char name[NAME_LENGTH] = "Unknown";

	ioctl(hid_fd, JSIOCGVERSION, &version);
	ioctl(hid_fd, JSIOCGAXES, &axes);
	ioctl(hid_fd, JSIOCGBUTTONS, &buttons);
	ioctl(hid_fd, JSIOCGNAME(NAME_LENGTH), name);
	
	printf("Driver version is %d.%d.%d.\n",
					version >> 16, (version >> 8) & 0xff, version & 0xff);

	struct js_event js;

	fcntl(hid_fd, F_SETFL, O_NONBLOCK);

	int16_t js_values[20];
	int16_t input_timer=0;
#define INPUT_TIMEOUT	100
	while (1) {

		while (read(hid_fd, &js, sizeof(struct js_event)) == sizeof(struct js_event))  {
			input_timer=0;
			if (js.type==2) {
				js_values[js.number]=js.value;
				/*
				if (js.number==HID_EVENT_NO_LEFT_Y) {
					printf("Event: type %d, time %d, number %d, value %d\n",
					js.type, js.time, js.number, js.value);
					
				}
				*/
			}
		}

		if (input_timer>INPUT_TIMEOUT) {
			printf("Input Timeout reached!");
			tx_vect[1]=127;
			tx_vect[2]=127;
			tx_vect[3]=127;
			tx_vect[4]=127;
			tx_vect[5]=0;
		} else {
			input_timer++;
			tx_vect[1]=127-(js_values[HID_EVENT_NO_LEFT_Y]/255);
			tx_vect[2]=127+(js_values[HID_EVENT_NO_LEFT_X]/255);
			tx_vect[3]=127-(js_values[HID_EVENT_NO_RIGHT_Y]/255);
			tx_vect[4]=127+(js_values[HID_EVENT_NO_RIGHT_X]/255);
			for (uint8_t i=0;i<4;i++) {
				if (tx_vect[i]<0) {
					tx_vect[i]=0;
				}
				if (tx_vect[i]>255) {
					tx_vect[i]=255;
				}
			}
		}
		
		if (errno != EAGAIN) {
			perror("\njstest: error reading");
			return 1;
		}
		if (debug) {
			printf(
				"right_V=%03i | right_H=%03i | left_V=%03i| right_V=%03i | buttons=%03i\n",
				tx_vect[1],tx_vect[2],tx_vect[3],tx_vect[4],tx_vect[5]
			);
		}
		usleep(33*1000);

		msgpack::sbuffer tx_msg;
		/* send vector via zmq*/
		zmq::message_t rx_zmq;
		
		msgpack::pack(&tx_msg, tx_vect);
		zmq::message_t tx_zmq(tx_msg.size());
		
		memcpy(static_cast<char*>(tx_zmq.data()), tx_msg.data(), tx_msg.size());
		socket.send(tx_zmq);
		socket.recv (&rx_zmq);

	}	
	close(hid_fd);
	return 0;
}

