#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream )
{
  // Your code here.
  if ( message.SYN && tcp_state == IDLE ) {
    zero_point = message.seqno;
    abs_ackno++;
    tcp_state |= START_STREAM;
  }

  if ( message.FIN && ( tcp_state & START_STREAM ) ) {
    tcp_state |= END_STREAM;
  }

  if ( tcp_state & START_STREAM && message.sequence_length() > 0 ) {
    uint64_t abs_seqno = message.seqno.unwrap( zero_point, reassembler.get_first_unassembled() ) + message.SYN;
    reassembler.insert( abs_seqno - 1, message.payload, message.FIN, inbound_stream );
  }

  if ( tcp_state & END_STREAM && reassembler.bytes_pending() == 0 )
    abs_ackno++;

  return;
}

TCPReceiverMessage TCPReceiver::send( const Writer& inbound_stream ) const
{
  // Your code here.
  TCPReceiverMessage recv;
  recv.window_size
    = ( inbound_stream.available_capacity() < UINT16_MAX ) ? inbound_stream.available_capacity() : UINT16_MAX;
  if ( tcp_state & START_STREAM )
    recv.ackno = zero_point + inbound_stream.bytes_pushed() + abs_ackno;

  return recv;
}
