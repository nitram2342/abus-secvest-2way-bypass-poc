#include "helper.h"
#include "SecvestDatagram.h"
#include <stdio.h>
#include <boost/foreach.hpp>



SecvestDatagram::SecvestDatagram(std::string const & hex_str, bool parse) :
  Datagram(hex_str), 
  valid_crc(false) {

  if(parse) {
    vec = differential_manchester_decoding(vec);
  
    for(size_t i=2; i < vec.size()-1; i++) {      
      uint16_t crc = calc_crc(vec, i);
      if(vec.at(i) == (crc >> 8) &&
	 vec.at(i+1) == (crc & 0xff)) {
	valid_crc = true;
	crc16 = crc;
	vec.resize(i+2);
	return;
      }	 
    }
  }

}

SecvestDatagram::~SecvestDatagram() {}


std::vector<uint8_t> SecvestDatagram::differential_manchester_decoding(std::vector<uint8_t> const & vec) const {
  std::vector<uint8_t> ret;

  for(size_t j = 0; j < vec.size(); j+=2) {
    
    uint16_t v = (vec.at(j) << 8);
    if(j+1 < vec.size()) v |= vec.at(j+1);

    uint8_t dst = 0;

    for(int i = 15; i > 0; i-=2) {
      int first_bit = v & (1 << i) ? 1 : 0;
      int next_bit = v & (1 << (i-1)) ? 1 : 0;

      dst <<= 1;
      
      if(first_bit == next_bit) // phasensprung
	;    
      else 
	dst |= 1;

    }

    ret.push_back(dst);
  }

  return ret;
}

uint16_t SecvestDatagram::calc_crc(std::vector<uint8_t> const& vec, size_t len) const {
  uint16_t crc = 0;

  uint16_t i, k;
  int polynomial = 0x1021;

  for(k = 0; k < len; k++ ) {

    for(i = 0; i < 8; i++ ) {
      int bit = ((vec.at(k) >> (7-i) & 1) == 1);
      int c15 = ((crc >> 15    & 1) == 1);
      crc <<= 1;
      if (c15 ^ bit) crc ^= polynomial;
    }
  }

  return crc;
}



void SecvestDatagram::print_analysis(std::ostream& stream) const {

  stream << "     " << print_hex_lsbfirst();

  unsigned int msg_type = lsb_first(vec.at(0));
  unsigned int counter = lsb_first(vec.at(1));

  stream << "  [lsb-first]\n"
	 << "\tType                  : " << std::hex << msg_type << "\n"
	 << "\tmessage number        : " << std::hex << (counter & 0xf) << "\n"
	 << "\tRolling 4-bit counter : " << std::hex << (counter >> 4) << "\n"
	 << "\tCRC16                 : " << std::hex << (int)(crc16) << "\n"
    ;
}

std::ostream& operator<<(std::ostream& stream, SecvestDatagram const& m) {

  stream << m.to_hex_string(true, true, true);

  if(!m.valid_crc) {
    stream << " [crc-error]";
  }

  return stream;
}

