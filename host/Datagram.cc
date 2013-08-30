#include "helper.h"
#include "Datagram.h"
#include <boost/foreach.hpp>
#include <stdio.h>

Datagram::Datagram(std::string const & hex_str) {
  if(hex_str.size() > 0 && hexstr_to_vector(vec, hex_str) == 0) throw "failed to convert hex string";
}

Datagram::~Datagram() {}



std::string Datagram::print_bits() const {
  std::string ret;
  char buf[50];

  BOOST_FOREACH(int v, vec) {
    ret += val_to_bitstr(v);

    //snprintf(buf, sizeof(buf), "[%2x] ", lsb_first(v));
    //ret += buf;
    //stream << buf;
  }
  return ret;
}

std::string Datagram::print_hex_lsbfirst() const {
  std::string ret;
  char buf[20];

  BOOST_FOREACH(int v, vec) {
    snprintf(buf, sizeof(buf), "%2x ", lsb_first(v));
    ret += buf;
  }
  return ret;
}


std::string Datagram::to_hex_string(bool append_space, bool prepend_zero, bool _lsb_first) const {
  std::string ret;
  char buf[20];

  BOOST_FOREACH(int v, vec) {
    
    snprintf(buf, sizeof(buf), 
	     prepend_zero ? "%02x%s" : "%2x%s", 
	     (_lsb_first ? lsb_first(v) : v), 
	     //v, 
	     (append_space ? " " : ""));
    ret += buf;
  }
  return ret;
}


std::ostream& operator<<(std::ostream& stream, Datagram const& m) {

  stream << m.to_hex_string(true, true, false);

  return stream;
}
