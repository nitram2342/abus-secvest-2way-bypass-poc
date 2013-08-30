// -*-c++-*-

#ifndef __SECVEST_DATAGRAM_H__
#define __SECVEST_DATAGRAM_H__

#include <vector>
#include <stdint.h>
#include <iostream>

#include "Datagram.h"

class SecvestDatagram : public Datagram {

private:

  bool valid_crc;
  uint16_t crc16;

public:

  friend std::ostream& operator<<(std::ostream& stream, SecvestDatagram const& m);

  SecvestDatagram(std::string const & hex_str, bool parse = true);
  ~SecvestDatagram();

  bool crc_ok() const { return valid_crc; }

  void print_analysis(std::ostream& stream = std::cout) const;

private:

  std::vector<uint8_t> differential_manchester_decoding(std::vector<uint8_t> const & vec) const;
  uint16_t calc_crc(std::vector<uint8_t> const& vec, size_t len) const;

};

std::ostream& operator<<(std::ostream& stream, SecvestDatagram const& m);

#endif
