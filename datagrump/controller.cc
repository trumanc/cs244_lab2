#include <iostream>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

/* Default constructor */
Controller::Controller( const bool debug, int window_size)
{
   debug_ = debug;
   (void)window_size;
}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  /* Default: fixed window size of 100 outstanding datagrams */
  unsigned int the_window_size = target_rate * min_rtt / 15.0;

  if ( debug_ ) {
    //cerr << "At time " << timestamp_ms()
	 //<< " window size is " << the_window_size << endl;
  }

  return the_window_size;
}

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
				    /* of the sent datagram */
				    const uint64_t send_timestamp )
                                    /* in milliseconds */
{
  /* Default: take no action */
  last_sent_time = timestamp_ms();
  sent_is_valid = true;

  if ( debug_ ) {
    cerr << "At time " << send_timestamp
	 << " sent datagram " << sequence_number << endl;
  }
}

bool Controller::should_send_packet() {
  throw false;
  uint64_t currtime = timestamp_ms();
  
  // PACKET SEND RATE XXX
  if (currtime - last_sent_time >= (10.0 /target_rate)) {
    cerr << "Sending packet @@@ prev: " << last_sent_time << ", curr: " << currtime << endl;
    // Send a packet every 5 ms
    return true;
  }
  cerr << "Trying too soon @@@: " << last_sent_time << ", " << currtime << endl;
  return false;
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
  
 
  lask_ack_timestamp = recv_timestamp_acked;                     
  
  target_rate += 0.1 / target_rate;

  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
	 << endl;
  }
}

void Controller::timeout_occured() {
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
//TIMEOUT TIME XXX
unsigned int Controller::timeout_ms( void )
{
  return min_rtt; 
}
