#include "reassembler.hh"
#include <map>
// #define MAX_SEGMENT_SIZE

using namespace std;

Reassembler::Reassembler() : substr {}, str( "" ), first_unassembled( 0 ), first_unacceptable( 0 ), close( false )
{}

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring, Writer& output )
{
  // Your code here.
  u_int64_t last_data_index = first_index + data.size();
  first_unassembled = output.bytes_pushed();
  first_unacceptable = first_unassembled + output.available_capacity();  
  close |= is_last_substring;
  /* Invalid data */
  if ( last_data_index < first_unassembled || first_index > first_unacceptable || last_data_index == first_index ) {
    if ( close && substr.empty() )
      output.close();
    return;
  }
  /* Remove ignore data */
  last_data_index = (last_data_index > first_unacceptable) ? first_unacceptable : last_data_index;
  if ( first_index > first_unassembled )
    str = data.substr( 0, last_data_index - first_index );
  else
    str = data.substr( first_unassembled - first_index );
  /* Reassemble with the older data */
  for ( auto itr = substr.begin(); itr != substr.end(); ) {
    /* Not overlap */
    if ( last_data_index < itr->first )
      break;

    uint64_t last_substr_index = itr->first + itr->second.size();
    if ( first_index > last_substr_index ) {
      itr++;
      continue;
    }
    /* Data already exist */
    if ( first_index >= itr->first && last_data_index <= last_substr_index ) {
      str.clear();
      break;
    }
    /* Overlap - merge with older data to the new one */
    if ( first_index < itr->first && last_data_index < last_substr_index ) {
      str.append( itr->second, last_data_index - itr->first );
      last_data_index = last_substr_index;
    }

    if ( first_index > itr->first && last_data_index > last_substr_index ) {
      str = itr->second.append( str, last_substr_index - first_index );
      first_index = itr->first;
    }

    substr.erase( itr++ );
  }
  /* Insert if data not enough, output when ressenable successfull */
  if ( first_index > first_unassembled )
    substr.insert( { first_index, str } );
  else {
    output.push( str );
    first_unassembled += str.size();
  }

  if ( close && substr.empty() )
    output.close();
  str.clear();
  return;
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  uint64_t bytes_pending = 0;

  for ( auto itr = substr.begin(); itr != substr.end(); ++itr ) {
    bytes_pending += itr->second.size();
  }
  return bytes_pending;
}
