#include "tcp_sender.hh"
#include "tcp_config.hh"
#include <iostream>
#include <random>

using namespace std;

/* TCPSender constructor (uses a random ISN if none given) */
TCPSender::TCPSender( uint64_t initial_RTO_ms, optional<Wrap32> fixed_isn )
  : isn_( fixed_isn.value_or( Wrap32 { random_device()() } ) ), initial_RTO_ms_( initial_RTO_ms )
{}

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
  return bytes_in_flight;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return consecutive_retrans;
}

optional<TCPSenderMessage> TCPSender::maybe_send()
{
  // Your code here.
  if (timer_expire) {
    timer_start = true;
    consecutive_retrans++;
    return outstand_segment.begin()->second;
  }

  if (segment_.empty())
    return {};

  TCPSenderMessage seg = segment_.front();
  segment_.pop();
  timer_start = true;
  return seg;
}

void TCPSender::push( Reader& outbound_stream )
{
  // Your code here.
  // nothing to send
  if ( (window_size <= bytes_in_flight || outbound_stream.bytes_buffered() == 0) && (state & START_STREAM) ) {
    return;
  }

  uint64_t byte_remain = window_size - bytes_in_flight;
  TCPSenderMessage seg;
  uint64_t seg_len = min(byte_remain, outbound_stream.bytes_buffered());
  string str { outbound_stream.peek() };
  
  if (!str.empty())
    seg.payload = Buffer(str.substr(0, seg_len));
  
  seg.seqno = isn_ + abs_seq_no;  
  if (state == IDLE) {
    seg.SYN = true;    
    state |= START_STREAM;
  }
  abs_seq_no += seg.sequence_length();
  bytes_in_flight += seg.sequence_length();
  
  segment_.push( seg );
  outstand_segment.insert( {abs_seq_no, seg });
  outbound_stream.pop(seg_len);
}

TCPSenderMessage TCPSender::send_empty_message() const
{
  // Your code here.
  TCPSenderMessage seg;
  seg.seqno = isn_ + abs_seq_no;
  return seg;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.
  Wrap32 ackno = *std::move(msg.ackno);
  uint64_t abs_ack_no = ackno.unwrap(isn_, abs_seq_no);

  if (abs_ack_no < curr_ack_no)
    return;

  curr_ack_no = abs_ack_no;
  window_size = msg.window_size;
  if (abs_ack_no == abs_seq_no) {
    tx_time_out = initial_RTO_ms_;
    timer_start = false;
    bytes_in_flight = 0;
    consecutive_retrans = 0;
    outstand_segment.clear();
    return;
  }

  for (auto itr = outstand_segment.begin(); itr != outstand_segment.end();) {
    if (itr->first > abs_ack_no)
      break;
    bytes_in_flight -= itr->second.sequence_length();
    outstand_segment.erase(itr++);
  }
  return;
}

void TCPSender::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  static uint64_t time_tx = 0;
  if (!timer_start || outstand_segment.empty()) {
    timer_expire = false;
    time_tx = 0;
    return;
  }
  
  time_tx += ms_since_last_tick;
  timer_expire = (time_tx >= tx_time_out);
  if (timer_expire) {
    time_tx = 0;
    timer_start = false;
    if (window_size > 0)
      tx_time_out = 2 * initial_RTO_ms_;
  }
  return;
}
