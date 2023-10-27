#include "tcp_sender.hh"
#include "tcp_config.hh"

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
    /* Send the lowest seqno */
    timer_expire = false;
    return outstand_segment.begin()->second;
  }

  if (segment_.empty())
    return {};
  /* Send and pop */
  TCPSenderMessage seg = segment_.front();
  segment_.pop();
  timer_start = true;
  return seg;
}

void TCPSender::push( Reader& outbound_stream )
{
  // Your code here.
  uint16_t abs_window_size = (window_size > 0) ? window_size : 1;
  if ( abs_window_size <= bytes_in_flight || state & END_STREAM) 
    return;

  bool is_SYN = (state == IDLE);
  state |= START_STREAM;

  bool is_FIN = outbound_stream.is_finished();
  /* No thing to send*/
  if (outbound_stream.bytes_buffered() == 0 && !is_SYN && !is_FIN)
    return;

  while(abs_window_size > bytes_in_flight) {
    TCPSenderMessage seg;
    uint64_t byte_remain = min(abs_window_size - bytes_in_flight, outbound_stream.bytes_buffered());
    uint64_t payload_len = min(byte_remain, TCPConfig::MAX_PAYLOAD_SIZE);
    string str { outbound_stream.peek() };

    if (!str.empty()) {
      seg.payload = Buffer(str.substr(0, payload_len));
      outbound_stream.pop(payload_len);
    }    
    seg.seqno = isn_ + abs_seq_no;
    seg.SYN = is_SYN;
    /* Not FIN if window is full */
    seg.FIN = (payload_len > 0 && payload_len == (abs_window_size - bytes_in_flight) ) ? false : outbound_stream.is_finished();
    if (seg.FIN)
      state |= END_STREAM;

    abs_seq_no += seg.sequence_length();
    bytes_in_flight += seg.sequence_length();
    /* Create and store outstanding segment */
    segment_.push( seg );
    outstand_segment.insert( {abs_seq_no, seg });
    if (outbound_stream.bytes_buffered() == 0)
      break;
  }
  return;
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

  if (abs_ack_no < curr_ack_no || abs_ack_no > abs_seq_no)
    return;
  /* Got ACK - reset timer*/
  window_size = msg.window_size;
  if (abs_ack_no > 1) {
    time_tx = 0;
    consecutive_retrans = 0;
    tx_time_out = initial_RTO_ms_;
  }
  curr_ack_no = abs_ack_no;
  /* All segment acknoleged - stop timer */
  if (abs_ack_no == abs_seq_no) {
    bytes_in_flight = 0;
    timer_start = false;
    outstand_segment.clear();
    return;
  }
  /* Clear segment acked */
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
  if ( !timer_start || outstand_segment.empty() ) {
    timer_expire = false;
    time_tx = 0;
    consecutive_retrans = 0;
    return;
  }
  
  time_tx += ms_since_last_tick;
  timer_expire = (time_tx >= tx_time_out);
  if (timer_expire) {
    time_tx = 0;
    consecutive_retrans++;
    /* Double RTO if window > 0 */
    if (window_size > 0)
      tx_time_out = 2 * tx_time_out;     
  }
  return;
}
