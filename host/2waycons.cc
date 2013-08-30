/*
 * Interface for the 2way sniffer/injector
 *
 * Author: Martin Schobert <schobert@sitsec.net>
 *
 */
#include <sys/time.h>
#include <iostream>
#include <fstream>
#include <list>
#include <string>
#include <stdexcept>
#include <tr1/memory>
#include <boost/program_options.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>

#include <SerialStream.h>
#include "Datagram.h"
#include "SecvestDatagram.h"
#include "helper.h"
#include <boost/date_time/posix_time/posix_time.hpp>

namespace po = boost::program_options;

using namespace LibSerial;



/* --------------------------------------------------------------------------

     Main

   --------------------------------------------------------------------------
*/



int main(int argc, char *argv[]) {

  std::string serial_port("/dev/ttyACM0");
  bool mode_dump_raw = false;
  bool mode_dump_sv2 = false;
  bool mode_valid_crc = false;

  bool mode_raw_show_hex = false;
  bool mode_raw_show_lsbfirst = false;
  bool mode_raw_show_bits = false;

  bool mode_sv2_show_hex = false;
  bool mode_sv2_show_lsbfirst = false;
  bool mode_sv2_show_bits = false;

  bool mode_timestamp = false;
  bool debug = false;

  struct timeval tv;
  memset(&tv, 0, sizeof(struct timeval));

  // definition of program options
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help", "produce help message")
    ("serial", po::value<std::string>(), "the serial device")
    ("tx-sv2", po::value<std::string>(), "transmit sv2")
    ("dump-raw", "read messages and dump in raw format")
    ("dump-raw-hex", "  show in hex")
    ("dump-raw-lsbfirst", "  show in hex with LSB-first")
    ("dump-raw-bits", "  show as bits")
    ("dump-sv2", "read messages and dump in sv2 format")
    ("dump-sv2-hex", "  show in hex")
    ("dump-sv2-lsbfirst", "  show in hex with LSB-first")
    ("dump-sv2-bits", "  show as bits")
    ("valid-crc", "process only messages with a valid CRC")
    ("show-hex", "display messages in hex")
    ("show-lsbfirst", "display messages in hex with lsbfirst")
    ("show-bits", "display messages as bits")
    ("timestamp", "add a timestamp")
    ("exit", "exit after tx")
    ("register", po::value<std::vector<std::string> >(), "initialize a register")
    ("debug", "additional logging")
    ;

  // parse programm options
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm); 

  
  if(vm.count("serial")) serial_port = vm["serial"].as<std::string>();
  if(vm.count("dump-raw")) mode_dump_raw = true;
  if(vm.count("dump-sv2")) mode_dump_sv2 = true;
  if(vm.count("valid-crc")) mode_valid_crc = true;
  if(vm.count("timestamp")) mode_timestamp = true;

  if(vm.count("show-hex")) {
    mode_raw_show_hex = true;
    mode_sv2_show_hex = true;
  }

  if(vm.count("show-lsbfirst")) {
    mode_raw_show_lsbfirst = true;
    mode_sv2_show_lsbfirst = true;
  }
  if(vm.count("show-bits")) {
    mode_raw_show_bits = true;
    mode_sv2_show_bits = true;
  }


  if(vm.count("dump-raw-hex")) {
    mode_dump_raw = true;
    mode_raw_show_hex = true;
  }
  else if(vm.count("dump-raw-lsbfirst")) {
    mode_dump_raw = true;
    mode_raw_show_lsbfirst = true;
  }
  else if(vm.count("dump-raw-bits")) {
    mode_dump_raw = true;
    mode_raw_show_bits = true;
  }

  if(vm.count("dump-sv2-hex")) {
    mode_dump_sv2 = true;
    mode_sv2_show_hex = true;
  }
  else if(vm.count("dump-sv2-lsbfirst")) {
    mode_dump_sv2 = true;
    mode_sv2_show_lsbfirst = true;
  }
  else if(vm.count("dump-sv2-bits")) {
    mode_dump_sv2 = true;
    mode_sv2_show_bits = true;
  }


  if(vm.count("debug")) debug = true;

  if(vm.count("help") || !(mode_dump_raw || mode_dump_sv2 || vm.count("tx-sv2"))) {
    std::cout << "\n"
	      << "Usage: " << argv[0] << " [options]\n\n"
	      << desc << "\n";
    return 1;
  }



  // open serial
  SerialPort s(serial_port);
  try {
    s.Open(//SerialPort::BAUD_115200, 
	   SerialPort::BAUD_9600,
	   SerialPort::CHAR_SIZE_8,
	   SerialPort::PARITY_NONE,
	   SerialPort::STOP_BITS_1,
	   SerialPort::FLOW_CONTROL_NONE);
  }
  catch(SerialPort::OpenFailed const & e) {
    std::cerr << "Error: Can't open serial port " << serial_port << ": " << e.what() << std::endl;
    return 1;
  }

  //s.Write("set_rx_max_length 26\r\n");
  //std::cout << "got " << s.ReadLine() << "\n";

  if(vm.count("register")) {
    std::vector<std::string> reg_values = vm["register"].as<std::vector<std::string> >();
    BOOST_FOREACH(std::string const& reg_val_str, reg_values) {
      boost::format f("set_register %08x\r\n");
      f % hex_to_uint(reg_val_str);
      s.Write(f.str());

      std::cout << "write " << f.str()
		<< "got " << s.ReadLine() << "\n";
    }

    s.Write("reset_radio\r\n");
    std::cout << "reset\n"
	      << "got " << s.ReadLine() << "\n";
    
  }

  if(vm.count("tx-sv2")) {

    std::string str = vm["tx-sv2"].as<std::string>();
    std::cout << "[" << str << "]\n";
    Datagram d(str);

    boost::format f("tx_sv2 %1%\r\n");
    f % d.to_hex_string(false, true, true);
    s.Write(f.str());

    std::cout << "write " << f.str()
	      << "got " << s.ReadLine() << "\n";

    if(vm.count("exit")) exit(1);
  }


  std::string log_prefix_raw, log_prefix_sv2;
  if(mode_dump_raw && mode_dump_sv2) {
    log_prefix_raw = "raw: ";
    log_prefix_sv2 = "sv2: ";
  }

  try {
    if(mode_dump_raw || mode_dump_sv2) {
      
      struct timeval tv_last_msg;
      
      while(1) {
	std::string raw_str = s.ReadLine();
	std::string str = parse_rx_msg(raw_str); 
	if(debug) {
	  std::cout << "serial: " << raw_str;
	  std::cout << "parsed : " << str << "\n";
	}

	//      if(str.size() % 2 == 1) str += "0";
	
	if(tv.tv_sec == 0 && tv.tv_usec == 0) {
	  gettimeofday(&tv, NULL);
	  gettimeofday(&tv_last_msg, NULL);
	}
	
	try {
	RadioDatagram raw(str);
	SecvestDatagram sv2(str);
	
	if(debug) std::cout << "raw: " << raw << "\n";
	bool process = true;
	
	
	if(process) {
	  if(mode_dump_raw && mode_dump_sv2) std::cout << "\n"; // add a new line to better group corresponding messages
	  
	  /*
	      struct timeval _tv;
	      gettimeofday(&_tv, NULL);
	      timestamp_diff(&_tv, &tv_last_msg);

	      gettimeofday(&tv_last_msg, NULL);
	      boost::format f("%03d.%-6d s ");
	      f % _tv.tv_sec % (_tv.tv_usec);
	      //if(!(_tv.tv_sec == 0 && _tv.tv_usec < 100)) std::cout << "--------------------------------------["<< f.str() << "]\n";
	      */
	}


	if(process && mode_dump_raw) {
	  
	  std::string color = MAKE_RED;
	  
	  if(raw.at(0) == 0b01010010)  color = MAKE_LRED;
	    
	  if(!mode_valid_crc || (mode_valid_crc && sv2.crc_ok())) {
	    
	    if(mode_raw_show_lsbfirst) {
	      std::cout << MAKE_GREEN
			<< timestamp(mode_timestamp, &tv) 
			<< log_prefix_raw
			<< color
			<< raw.print_hex_lsbfirst()
			<< RESET_COLOR		      
			<< "\n";
	    }
	    else if(mode_raw_show_hex) 
	      std::cout  << MAKE_GREEN
			 << timestamp(mode_timestamp, &tv)
			 << log_prefix_raw 
			 << color
			 << raw 
			 << RESET_COLOR
			 << "\n";
	    if(mode_raw_show_bits) {
	      std::cout << MAKE_GREEN
			<< timestamp(mode_timestamp, &tv)
			<< log_prefix_raw
			<< color
			<< raw.print_bits()
			<< RESET_COLOR
			<< "\n";
	    }
	    
	  }
	} // end of raw dump

	if(process && mode_dump_sv2) {
	  if(!mode_valid_crc || (mode_valid_crc && sv2.crc_ok())) {
	    
	    std::string color = MAKE_LGREEN;
	    std::string crc_color = sv2.crc_ok() ? MAKE_LGREEN : MAKE_GREEN;
	    	    
	    std::string crc_status(" "); 
	    crc_status += crc_color + (sv2.crc_ok() ? "[crc-ok]" : "[mismatch]" );

	    if(mode_sv2_show_lsbfirst) {
	      std::cout << MAKE_GREEN
			<< timestamp(mode_timestamp, &tv) 
			<< log_prefix_sv2
			<< color
			<< sv2.print_hex_lsbfirst()
			<< crc_status
			<< RESET_COLOR		      
			<< "\n";
	    }
	    else if(mode_sv2_show_hex) 
	      std::cout << MAKE_GREEN
			<< timestamp(mode_timestamp, &tv) 
			<< log_prefix_sv2
			<< color
			<< sv2 
			<< crc_status
			<< RESET_COLOR
			<< "\n";
	    if(mode_sv2_show_bits) {
	      std::cout << MAKE_GREEN
			<< timestamp(mode_timestamp, &tv) 
			<< log_prefix_sv2
			<< color
			<< sv2.print_bits()
			<< crc_status
			<< RESET_COLOR		      
			<< "\n";
	    }
	    
	    //	  sv2.print_analysis();
	  
	  }
	} // end of sv2 dump

	}
	catch(std::out_of_range const & e) {
	  std::cout << "failed to parse string\n";
	}
	
      }// en of while(1)
    }
  }
  catch(const char * e) {
    std::cerr << "Got an exception: " << e << "\n";
  }

  s.Close();

  return 0;
}


