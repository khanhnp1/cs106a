#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"
#include <map>

enum tcp_state {
  IDLE          = 0,
  START_STREAM  = (1 << 0),
  END_STREAM    = (1 << 1) 
};

class TCPSender
{
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;
private:
  uint64_t abs_seq_no = 0;
  uint64_t curr_ack_no = 0;
  uint64_t bytes_in_flight = 0;
  uint64_t consecutive_retrans = 0;
  uint16_t window_size = 1;
  uint16_t state = IDLE;
  /* Timer */
  uint64_t tx_time_out = initial_RTO_ms_;
  uint64_t time_tx = 0;
  bool timer_start = false;
  bool timer_expire = false;
  /* Segment from bytestream and storage outstanding */
  std::queue<TCPSenderMessage> segment_ {};
  std::map<uint64_t, TCPSenderMessage> outstand_segment {}; 
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( uint64_t initial_RTO_ms, std::optional<Wrap32> fixed_isn );

  /* Push bytes from the outbound stream */
  void push( Reader& outbound_stream );

  /* Send a TCPSenderMessage if needed (or empty optional otherwise) */
  std::optional<TCPSenderMessage> maybe_send();

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage send_empty_message() const;

  /* Receive an act on a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called. */
  void tick( uint64_t ms_since_last_tick );

  /* Accessors for use in testing */
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
};
