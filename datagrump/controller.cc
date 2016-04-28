#include <iostream>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

/* Default constructor */
Controller::Controller( const bool debug, int window_size)
{
   debug_ = debug;
   win_size = window_size;
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
  
  if (rtt > 3 * min_rtt) {
    target_rate = 1;
  }
  
  cerr << "ONEPACKRTT, " << send_timestamp_acked << ", "
       << timestamp_ack_received - send_timestamp_acked << endl;

  if (sequence_number_acked % 20 == 0) {
    cerr << "SUPERACKDIFF, " << send_timestamp_acked << ", "
                             << recv_timestamp_acked - last_super_ack_timestamp << endl;
    last_super_ack_timestamp = recv_timestamp_acked;
  }
  
  ack_diff_average  =  (1 - alpha)*ack_diff_average +
                       (recv_timestamp_acked - lask_ack_timestamp)*alpha;
                       
  cerr << "ACKDIFF, " << send_timestamp_acked << ", "
                      << recv_timestamp_acked - lask_ack_timestamp << ", "
                      << ack_diff_average 
                      << endl;
                       
  
  lask_ack_timestamp = recv_timestamp_acked;                     
  
  uint64_t curr_rtt = timestamp_ack_received - send_timestamp_acked;
  //cerr << timestamp_ack_received << ", " << curr_rtt << endl;
  target_rate += 0.1 / target_rate;
  win_size = min(win_size, 60.0); // TODO: Not sure this is helping much
  if (curr_rtt > 100) { // TODO: Play with this trigger
    
    if ( debug_ || true) {
      cerr << "Slow rtt. MD-ing window size from " << win_size << endl;
    }
    
    //win_size *= (1.0/5.0);
    //if (win_size < 1.0) win_size = 1.0;
    // NUKE THE WINDOW, CLEAR THE QUEUE
    win_size = 1;
  }
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
  return 50; /* timeout of one second */
}
