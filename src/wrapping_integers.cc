#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  return zero_point + ( n & 0xFFFFFFFF );
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Your code here.
  uint64_t high_check = ( checkpoint >> 32 ) & 0xFFFFFFFF;
  uint64_t low_check = checkpoint & 0xFFFFFFFF;
  uint64_t low_seqno = static_cast<uint64_t>( raw_value_ - zero_point.raw_value_ );
  uint32_t t = ( low_seqno > low_check ) ? ( low_seqno - low_check ) : ( low_check - low_seqno );

  if ( t & 0x80000000 && low_seqno > low_check && high_check > 0 )
    return low_seqno | ( ( high_check - 1 ) << 32 );

  if ( t & 0x80000000 && low_seqno < low_check )
    return low_seqno | ( ( high_check + 1 ) << 32 );

  return low_seqno | ( high_check << 32 );
}
