// -*-c++-*-

#ifndef __DATAGRAM_H__
#define __DATAGRAM_H__

#include <vector>
#include <stdint.h>
#include <iostream>

class Datagram {

protected:
  std::vector<uint8_t> vec;

public:
  friend std::ostream& operator<<(std::ostream& stream, Datagram const& m);

  Datagram(std::string const & hex_str);
  virtual ~Datagram();

  std::string print_bits() const;
  std::string print_hex_lsbfirst() const;
  std::string to_hex_string(bool append_space = false, bool prepend_zero = false, bool _lsb_first = false) const;

  uint8_t at(size_t index) const { return vec.at(index); }
  void set(size_t index, uint8_t val) { vec.at(index) = val; }
  size_t size() const { return vec.size(); }
};

std::ostream& operator<<(std::ostream& stream, Datagram const& m);

typedef Datagram RadioDatagram;

#endif
