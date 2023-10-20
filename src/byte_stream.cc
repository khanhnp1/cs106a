#include <stdexcept>

#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity )
  : capacity_( capacity )
  , used( 0 )
  , _data( "" )
  , byte_writen( 0 )
  , byte_read( 0 )
  , state( BYTE_STREAM_INPUT | BYTE_STREAM_POPED )
{}

void Writer::push( string data )
{
  // Your code here.
  if ( state & ( BYTE_STREAM_CLOSE | BYTE_STREAM_ERROR ) || data.empty() )
    return;

  state = BYTE_STREAM_INPUT;
  size_t append_size = ( available_capacity() < data.length() ) ? available_capacity() : data.length();

  _data.append( data, 0, append_size );
  used += append_size;
  byte_writen += append_size;
}

void Writer::close()
{
  // Your code here.
  state |= BYTE_STREAM_CLOSE;
}

void Writer::set_error()
{
  // Your code here.
  state |= BYTE_STREAM_ERROR;
}

bool Writer::is_closed() const
{
  // Your code here.
  return ( state & BYTE_STREAM_CLOSE );
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return capacity_ - used;
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return byte_writen;
}

string_view Reader::peek() const
{
  // Your code here.
  string_view str = _data;
  return str;
}

bool Reader::is_finished() const
{
  // Your code here.
  return ( ( state & BYTE_STREAM_CLOSE ) && ( state & BYTE_STREAM_POPED ) );
}

bool Reader::has_error() const
{
  // Your code here.
  return state & BYTE_STREAM_ERROR;
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  size_t remove_len = ( len < used ) ? len : used;
  _data = _data.substr( remove_len, used - remove_len );
  used -= remove_len;
  byte_read += remove_len;
  if ( used == 0 )
    state |= BYTE_STREAM_POPED;
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return used;
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return byte_read;
}
