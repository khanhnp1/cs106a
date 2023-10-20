#pragma once

#include "reassembler.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

enum tcp_state
{
  IDLE = 0,
  START_STREAM = ( 1 << 0 ),
  END_STREAM = ( 1 << 1 )
};

class TCPReceiver
{
private:
  Wrap32 zero_point { 0 };
  uint64_t abs_ackno = 0;
  uint32_t tcp_state = IDLE;

public:
  /*
   * The TCPReceiver receives TCPSenderMessages, inserting their payload into the Reassembler
   * at the correct stream index.
   */
  void receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream );

  /* The TCPReceiver sends TCPReceiverMessages back to the TCPSender. */
  TCPReceiverMessage send( const Writer& inbound_stream ) const;
};
