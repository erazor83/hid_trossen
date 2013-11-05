/**
 * hid-Trossen tool0
 * __author__		= Alexander Krause <alexander.krause@ed-solutions.de>
 * __version__	= 0.1.0
 * __date__			= 2013-10-26
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
#define		BUTTON_L4						(1<<0)
#define		BUTTON_L5						(1<<1)
#define		BUTTON_L6						(1<<2)

#define		BUTTON_R3						(1<<3)
#define		BUTTON_R2						(1<<4)
#define		BUTTON_R1						(1<<5)

#define		BUTTON_LT						(1<<6)
#define		BUTTON_RT						(1<<7)

//13 -RT lower
//12 -LT lower
#define		HID_BUTTON_RT				15
#define		HID_BUTTON_R1				17
#define		HID_BUTTON_R2				16
#define		HID_BUTTON_R3				19

#define		HID_BUTTON_L4				9
#define		HID_BUTTON_L5				8
#define		HID_BUTTON_L6				11
#define		HID_BUTTON_LT				14

#define		HID_BUTTON_ON_VALUE	5000
#define		INPUT_TIMEOUT				100

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


	const uint8_t hid_button_map[8][2]={
		{HID_BUTTON_L4,BUTTON_L4},
		{HID_BUTTON_L5,BUTTON_L5},
		{HID_BUTTON_L6,BUTTON_L6},
		{HID_BUTTON_R3,BUTTON_R3},
		{HID_BUTTON_R2,BUTTON_R2},
		{HID_BUTTON_R1,BUTTON_R1},
		{HID_BUTTON_LT,BUTTON_LT},
		{HID_BUTTON_RT,BUTTON_RT}
	};

}


int main(int argc, char** argv) {
	// === program parameters ===
	std::string zmq_uri="tcp://*:5555";
	std::string hid_device="/dev/hid1";
	unsigned int interval=30;
	bool debug=false;
	bool hid_check=false;

	namespace po = boost::program_options;

  // Setup options.
  po::options_description desc("Options");
	desc.add_options()
		("help", "produce help message")
		("uri", po::value< std::string >( &zmq_uri ),					"ZeroMQ server uri | default: tcp://*:5555" )
		("dev", po::value< std::string >( &hid_device ),			"hid device        | default: /dev/hid1 ")
		("check", "retry until hid can be opened")
		("interval", po::value< unsigned int >( &interval ),			"interval in ms    | default: 30 ")
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
		if (vm.count("check")) {
			hid_check=true;
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
	std::vector<int> otx_vect(6,0);
	tx_vect[0]=TROSSEN_COMMANDER;
	otx_vect[0]=TROSSEN_COMMANDER;
	
	int hid_fd;
	/* Open the Device with non-blocking reads. In real life,
	   don't use a hard coded path; use libudev instead. */

	while (true) {
		hid_fd = open(hid_device.c_str(), O_RDWR);

		if (hid_fd < 0) {
			if (hid_check) {
				 std::cout << "Unable to open hid device. Retrying in 1s..." << std::endl;
				 sleep(1);
			} else {
				std::cerr << "ERROR: unable to open hid device"  << std::endl << std::endl; 
				return 1;
			}
		} else {
			break;
		}
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
	for (int i=0; i<20;i++) {
		js_values[i]=0;
	}
	int16_t input_timer=0;
	tx_vect[0]=TROSSEN_COMMANDER;

	while (1) {

		while (read(hid_fd, &js, sizeof(struct js_event)) == sizeof(struct js_event))  {
			input_timer=0;
			if (js.type==2) {
				js_values[js.number]=js.value;
			}
#if 0
				if (js.number>6) {
				printf(
					"Event: type %d, time %d, number %d, value %d\n",
					js.type, js.time, js.number, js.value
				);
			}
			if (js.number==HID_BUTTON_RT) {
				printf(
					"Event: type %d, time %d, number %d, value %d\n",
					js.type, js.time, js.number, js.value
				);
			}
#endif
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
			tx_vect[1]=127-(js_values[HID_EVENT_NO_LEFT_Y]/250);
			tx_vect[2]=127+(js_values[HID_EVENT_NO_LEFT_X]/250);
			tx_vect[3]=127-(js_values[HID_EVENT_NO_RIGHT_Y]/250);
			tx_vect[4]=127+(js_values[HID_EVENT_NO_RIGHT_X]/250);
			tx_vect[5]=0;

			for (uint8_t i=0;i<8;i++) {
				if (js_values[hid_button_map[i][0]]>HID_BUTTON_ON_VALUE) {
					tx_vect[5]|=hid_button_map[i][1];
				}
			}
			/*
			if (js_values[hid_button_map[0][0]]>HID_BUTTON_ON_VALUE) {
				tx_vect[5]|=BUTTON_RT;
			}*/
			for (uint8_t i=1;i<5;i++) {
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
		usleep(interval*1000);
		
		bool update_required=false;
		for (uint8_t i=1;i<5;i++) {
			if (otx_vect[i]!=tx_vect[i]) {
				otx_vect[i]=tx_vect[i];
				update_required=true;
			}
		}
		if (update_required) {
			msgpack::sbuffer tx_msg;
			/* send vector via zmq*/
			zmq::message_t rx_zmq;
		
			msgpack::pack(&tx_msg, tx_vect);
			zmq::message_t tx_zmq(tx_msg.size());
		
			memcpy(static_cast<char*>(tx_zmq.data()), tx_msg.data(), tx_msg.size());
			socket.send(tx_zmq);
			socket.recv (&rx_zmq);
		}

	}	
	close(hid_fd);
	return 0;
}

