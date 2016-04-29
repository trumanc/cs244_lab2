#include <iostream>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

/* Default constructor */
Controller::Controller( const bool debug) :
  debug_(debug),
  target_rate(1.0), // 1 Mbps starting rate
  min_rtt(100000), // this value quickly shrinks
  panic_mode(false),
  saved_rate(1.0),
  panic_start_time(0)
{}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  /* Our target window size is calculated based on an idealized
   * badnwidth-delay product. We have a target rate that we want
   * to achieve, then multiply it by the minimum rtt we've seen,
   * since that indicates the delay we are trying to acheive.
   * The 15 is to convert from Mb/s and ms and bits into a 
   * window size in packets.
   */
  unsigned int the_window_size = target_rate * min_rtt / 15.0;

  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
	       << " window size is " << the_window_size << endl;
  }

  // Should always have at least one packet in flight
  return the_window_size > 0 ? the_window_size : 1;
}

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
				    /* of the sent datagram */
				    const uint64_t send_timestamp )
                                    /* in milliseconds */
{
  /* Default: take no action */
  if ( debug_ ) {
    cerr << "At time " << send_timestamp
	 << " sent datagram " << sequence_number << endl;
  }
}

/* An ack was received */
void Controller::ack_received( const uint64_t sequence_number_acked,
			       /* what sequence number was acknowledged */
			       const uint64_t send_timestamp_acked,
			       /* when the acknowledged datagram was sent (sender's clock) */
			       const uint64_t recv_timestamp_acked,
			       /* when the acknowledged datagram was received (receiver's clock)*/
			       const uint64_t timestamp_ack_received )
                               /* when the ack was received (by sender) */
{
  int rtt = timestamp_ack_received - send_timestamp_acked;
  
  /* Make sure our globally min rtt is accurate. In a real world scenario
   * where the min rtt changes as you move around (which isn't true in this
   * test case), I would slowly increment the min_rtt so that it eventually
   * ourgrew any old min_rtt that was no longer valid */
  if (rtt < min_rtt) {
    min_rtt = rtt;
  }
  
  
  if (panic_mode && rtt <= min_rtt * 2) {
    if (timestamp_ack_received - panic_start_time < (min_rtt * 2)) {
      // False alarm, slow rtt caused by bursty glitch
      // Reinstate old rate before exiting panic mode
      if ( debug_ ) {
        cerr << "Got a fast rtt quickly. Panic mode was a false alarm." << endl;
      }
      target_rate = saved_rate;
    } else {
      if (debug_) {
        cerr << "Recovered from panic mode after clearing the queue." << endl;
      }
    }
    panic_mode = false;
  }
  
  if (rtt > 2 * min_rtt) {
    if (!panic_mode) {
      // Entering panic mode
      saved_rate = target_rate;
      panic_start_time = timestamp_ack_received;
      panic_mode = true;
      if (debug_) {
        cerr << "Got slow rtt of " << rtt << ". Entering panic mode." << endl;
      }
      
      target_rate *= 0.5;
    }
  }
  
  // Slowly grow your target rate if all is well
  target_rate += 0.1 / target_rate;

  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
	 << endl;
  }
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram. This should only occur
   if the line has gone completely dead. */
unsigned int Controller::timeout_ms( void )
{
  return min_rtt; 
}
