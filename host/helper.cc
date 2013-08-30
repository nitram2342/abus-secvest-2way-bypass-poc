#include "helper.h"
#include <sys/time.h>
#include <iostream>
#include <sstream>
#include <boost/format.hpp>

uint8_t lsb_first(uint8_t v) {
  uint8_t ret = 0;
  for(int i = 0; i < 8; i++) {
    ret <<= 1;
    ret |= (v & (1 << i) ? 1 : 0);
  }
  return ret;
}

int hex_digit_to_int(uint8_t c) {
  if(c >= 'a' && c <= 'f') { return 10 + c - 'a'; }
  else if(c >= 'A' && c <= 'F') { return 10 + c - 'A'; }
  else if(c >= '0' && c <= '9') { return c - '0'; }
  else return -1;
}
 
size_t hexstr_to_vector(std::vector<uint8_t> & ret, std::string const & str) {
  int which_nibble = 0;
  uint8_t current_byte = 0;
  size_t i = 0;
  while(i < str.length()) {

    if(str[i] != ' ' && str[i] != ',') {
      int v = hex_digit_to_int(str[i]);
      if(v < 0) {
	std::cerr << "Failed to convert: [" << str << "]\n";
	return 0;
      }
      else {
	if(which_nibble == 0) {
	  current_byte = v << 4;
	  which_nibble = 1;
	}
	else {
	  current_byte |= v;
	  which_nibble = 0;
	  ret.push_back(current_byte);
	}
      }
    }
    i++;
  }
  return ret.size();
}

std::string val_to_bitstr(uint8_t v) {
  std::string ret;
  for(int i = 7; i >= 0; i--) {
    ret += (v & (1 << i) ? '1' : '0');
  }
  return ret;

}


void timestamp_diff(struct timeval * tv,  struct timeval * rel_tv) {
  if(tv != NULL && rel_tv != NULL) {

    if(tv->tv_usec < rel_tv->tv_usec) {
      int nsec = (rel_tv->tv_usec - tv->tv_usec) / 1000000 + 1;
      rel_tv->tv_usec -= 1000000 * nsec;
      rel_tv->tv_sec += nsec;
    }
    if(tv->tv_usec - rel_tv->tv_usec > 1000000) {
      int nsec = (tv->tv_usec - rel_tv->tv_usec) / 1000000;
      rel_tv->tv_usec += 1000000 * nsec;
      rel_tv->tv_sec -= nsec;
    }
     
    tv->tv_sec -= rel_tv->tv_sec;
    tv->tv_usec -= rel_tv->tv_usec;
  }
}

std::string timestamp(bool enable,  struct timeval * rel_tv) {
  struct timeval tv;
 
  gettimeofday(&tv, NULL);
  timestamp_diff(&tv, rel_tv);
  
  boost::format f("%03d.%-6d s ");
  f % tv.tv_sec % tv.tv_usec;
  return f.str();
}


uint32_t hex_to_uint(std::string const& s) {
  uint32_t x;   

  std::stringstream ss;

  size_t start = s.find("0x") == 0 ? 2 : 0;
  size_t end = s.find_first_not_of("0123456789abcdefABCDEF", start);

  std::cout << "parse [" << s.substr(start, end-start) << "]\n";

  ss << std::hex << s.substr(start, end-start);
  ss >> x;
  return x;
}

/**
 * Parse an incomming message.
 */
std::string parse_rx_msg(std::string const& msg) {

  std::string empty;

  //  RX 35bed56fc86a3ac57da381b6de046f9af8e4ffe5e9cfd278e153
  if(msg.find("RX ", 0, 3) == 0) {
    size_t offs = msg.find_first_not_of(' ', 3);
    if(offs != std::string::npos) {

      size_t end = msg.find_first_of("\r\n", offs+1);

      if(offs != std::string::npos && end != std::string::npos) 
	return msg.substr(offs, end-offs);
    }
  }
  
  return empty;
}

