/*
   Copyright (C) 2003, 2005, 2006, 2008 MySQL AB, 2008, 2009 Sun Microsystems, Inc.
    All rights reserved. Use is subject to license terms.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA
*/

//**************************************************************************** 
// 
//  AUTHOR 
//      �sa Fransson 
// 
//  NAME 
//      TransporterCallback 
// 
// 
//***************************************************************************/ 
#ifndef TRANSPORTER_CALLBACK_H 
#define TRANSPORTER_CALLBACK_H 
 
#include <kernel_types.h> 
#include "TransporterDefinitions.hpp" 
 
 
#define TE_DO_DISCONNECT 0x8000

enum TransporterError {
  TE_NO_ERROR = 0,
  /**
   * TE_ERROR_CLOSING_SOCKET
   *
   *   Error found during closing of socket
   *
   * Recommended behavior: Ignore
   */
  TE_ERROR_CLOSING_SOCKET = 0x1,

  /**
   * TE_ERROR_IN_SELECT_BEFORE_ACCEPT
   *
   *   Error found during accept (just before)
   *     The transporter will retry.
   *
   * Recommended behavior: Ignore
   *   (or possible do setPerformState(PerformDisconnect)
   */
  TE_ERROR_IN_SELECT_BEFORE_ACCEPT = 0x2,

  /**
   * TE_INVALID_MESSAGE_LENGTH
   *
   *   Error found in message (message length)
   *
   * Recommended behavior: setPerformState(PerformDisconnect)
   */
  TE_INVALID_MESSAGE_LENGTH = 0x3 | TE_DO_DISCONNECT,

  /**
   * TE_INVALID_CHECKSUM
   *
   *   Error found in message (checksum)
   *
   * Recommended behavior: setPerformState(PerformDisonnect)
   */
  TE_INVALID_CHECKSUM = 0x4 | TE_DO_DISCONNECT,

  /**
   * TE_COULD_NOT_CREATE_SOCKET
   *
   *   Error found while creating socket
   *
   * Recommended behavior: setPerformState(PerformDisonnect)
   */
  TE_COULD_NOT_CREATE_SOCKET = 0x5,

  /**
   * TE_COULD_NOT_BIND_SOCKET
   *
   *   Error found while binding server socket
   *
   * Recommended behavior: setPerformState(PerformDisonnect)
   */
  TE_COULD_NOT_BIND_SOCKET = 0x6,

  /**
   * TE_LISTEN_FAILED
   *
   *   Error found while listening to server socket
   *
   * Recommended behavior: setPerformState(PerformDisonnect)
   */
  TE_LISTEN_FAILED = 0x7,

  /**
   * TE_ACCEPT_RETURN_ERROR
   *
   *   Error found during accept
   *     The transporter will retry.
   *
   * Recommended behavior: Ignore
   *   (or possible do setPerformState(PerformDisconnect)
   */
  TE_ACCEPT_RETURN_ERROR = 0x8

  /**
   * TE_SHM_DISCONNECT
   *
   *    The remote node has disconnected
   *
   * Recommended behavior: setPerformState(PerformDisonnect)
   */
  ,TE_SHM_DISCONNECT = 0xb | TE_DO_DISCONNECT

  /**
   * TE_SHM_IPC_STAT
   *
   *    Unable to check shm segment
   *      probably because remote node
   *      has disconnected and removed it
   *
   * Recommended behavior: setPerformState(PerformDisonnect)
   */
  ,TE_SHM_IPC_STAT = 0xc | TE_DO_DISCONNECT

  /**
   * Permanent error
   */
  ,TE_SHM_IPC_PERMANENT = 0x21

  /**
   * TE_SHM_UNABLE_TO_CREATE_SEGMENT
   *
   *    Unable to create shm segment
   *      probably os something error
   *
   * Recommended behavior: setPerformState(PerformDisonnect)
   */
  ,TE_SHM_UNABLE_TO_CREATE_SEGMENT = 0xd

  /**
   * TE_SHM_UNABLE_TO_ATTACH_SEGMENT
   *
   *    Unable to attach shm segment
   *      probably invalid group / user
   *
   * Recommended behavior: setPerformState(PerformDisonnect)
   */
  ,TE_SHM_UNABLE_TO_ATTACH_SEGMENT = 0xe

  /**
   * TE_SHM_UNABLE_TO_REMOVE_SEGMENT
   *
   *    Unable to remove shm segment
   *
   * Recommended behavior: Ignore (not much to do)
   *                       Print warning to logfile
   */
  ,TE_SHM_UNABLE_TO_REMOVE_SEGMENT = 0xf

  ,TE_TOO_SMALL_SIGID = 0x10
  ,TE_TOO_LARGE_SIGID = 0x11
  ,TE_WAIT_STACK_FULL = 0x12 | TE_DO_DISCONNECT
  ,TE_RECEIVE_BUFFER_FULL = 0x13 | TE_DO_DISCONNECT

  /**
   * TE_SIGNAL_LOST_SEND_BUFFER_FULL
   *
   *   Send buffer is full, and trying to force send fails
   *   a signal is dropped!! very bad very bad
   *
   */
  ,TE_SIGNAL_LOST_SEND_BUFFER_FULL = 0x14 | TE_DO_DISCONNECT

  /**
   * TE_SIGNAL_LOST
   *
   *   Send failed for unknown reason
   *   a signal is dropped!! very bad very bad
   *
   */
  ,TE_SIGNAL_LOST = 0x15

  /**
   * TE_SEND_BUFFER_FULL
   *
   *   The send buffer was full, but sleeping for a while solved it
   */
  ,TE_SEND_BUFFER_FULL = 0x16

  /**
   * TE_SCI_UNABLE_TO_CLOSE_CHANNEL
   *
   *  Unable to close the sci channel and the resources allocated by
   *  the sisci api.
   */
  ,TE_SCI_UNABLE_TO_CLOSE_CHANNEL = 0x22

  /**
   * TE_SCI_LINK_ERROR
   *
   *  There is no link from this node to the switch.
   *  No point in continuing. Must check the connections.
   * Recommended behavior: setPerformState(PerformDisonnect)
   */
  ,TE_SCI_LINK_ERROR = 0x0017

  /**
   * TE_SCI_UNABLE_TO_START_SEQUENCE
   *
   *  Could not start a sequence, because system resources
   *  are exumed or no sequence has been created.
   *  Recommended behavior: setPerformState(PerformDisonnect)
   */
  ,TE_SCI_UNABLE_TO_START_SEQUENCE = 0x18 | TE_DO_DISCONNECT

  /**
   * TE_SCI_UNABLE_TO_REMOVE_SEQUENCE
   *
   *  Could not remove a sequence
   */
  ,TE_SCI_UNABLE_TO_REMOVE_SEQUENCE = 0x19 | TE_DO_DISCONNECT

  /**
   * TE_SCI_UNABLE_TO_CREATE_SEQUENCE
   *
   *  Could not create a sequence, because system resources are
   *  exempted. Must reboot.
   *  Recommended behavior: setPerformState(PerformDisonnect)
   */
  ,TE_SCI_UNABLE_TO_CREATE_SEQUENCE = 0x1a | TE_DO_DISCONNECT

  /**
   * TE_SCI_UNRECOVERABLE_DATA_TFX_ERROR
   *
   *  Tried to send data on redundant link but failed.
   *  Recommended behavior: setPerformState(PerformDisonnect)
   */
  ,TE_SCI_UNRECOVERABLE_DATA_TFX_ERROR = 0x1b | TE_DO_DISCONNECT

  /**
   * TE_SCI_CANNOT_INIT_LOCALSEGMENT
   *
   *  Cannot initialize local segment. A whole lot of things has
   *  gone wrong (no system resources). Must reboot.
   *  Recommended behavior: setPerformState(PerformDisonnect)
   */
  ,TE_SCI_CANNOT_INIT_LOCALSEGMENT = 0x1c | TE_DO_DISCONNECT

  /**
   * TE_SCI_CANNOT_MAP_REMOTESEGMENT
   *
   *  Cannot map remote segment. No system resources are left.
   *  Must reboot system.
   *  Recommended behavior: setPerformState(PerformDisonnect)
   */
  ,TE_SCI_CANNOT_MAP_REMOTESEGMENT = 0x1d | TE_DO_DISCONNECT

  /**
   * TE_SCI_UNABLE_TO_UNMAP_SEGMENT
   *
   *  Cannot free the resources used by this segment (step 1).
   *  Recommended behavior: setPerformState(PerformDisonnect)
   */
  ,TE_SCI_UNABLE_TO_UNMAP_SEGMENT = 0x1e | TE_DO_DISCONNECT

  /**
   * TE_SCI_UNABLE_TO_REMOVE_SEGMENT
   *
   *  Cannot free the resources used by this segment (step 2).
   *  Cannot guarantee that enough resources exist for NDB
   *  to map more segment
   *  Recommended behavior: setPerformState(PerformDisonnect)
   */
  ,TE_SCI_UNABLE_TO_REMOVE_SEGMENT = 0x1f  | TE_DO_DISCONNECT

  /**
   * TE_SCI_UNABLE_TO_DISCONNECT_SEGMENT
   *
   *  Cannot disconnect from a remote segment.
   *  Recommended behavior: setPerformState(PerformDisonnect)
   */
  ,TE_SCI_UNABLE_TO_DISCONNECT_SEGMENT = 0x20 | TE_DO_DISCONNECT

  /* Used 0x21 */
  /* Used 0x22 */
};

/**
 * The TransporterCallback class encapsulates those aspects of the transporter
 * code that is specific to particular upper layer (NDB API, single-threaded
 * kernel, or multi-threaded kernel).
 */
class TransporterCallback {
public:
  /**
   * This method is called to deliver a signal to the upper layer.
   *
   * The method may either execute the signal immediately (NDB API), or
   * queue it for later execution (kernel).
   */
  virtual void deliver_signal(SignalHeader * const header,
                              Uint8 prio,
                              Uint32 * const signalData,
                              LinearSectionPtr ptr[3]) = 0;

  /**
   * This method is called regularly (currently after receive from each
   * transporter) by the transporter code.
   *
   * It provides an opportunity for the upper layer to interleave signal
   * handling with signal reception, if so desired, so as to not needlessly
   * overflow the received signals job buffers. Ie. the single-threaded
   * kernel implementation currently executes received signals if the
   * job buffer reaches a certain percentage of occupancy.
   *
   * The method should return non-zero if signals were execute, zero if not.
   */
  virtual int checkJobBuffer() = 0;

  /**
   * The transporter periodically calls this method, indicating the number
   * of sends done to one NodeId, as well as total bytes sent.
   *
   * For multithreaded cases, this is only called while the send lock for the
   * given node is held.
   */
  virtual void reportSendLen(NodeId nodeId, Uint32 count, Uint64 bytes) = 0;

  /**
   * Same as reportSendLen(), but for received data.
   *
   * For multithreaded cases, this is only called while holding the global
   * receive lock.
   */
  virtual void reportReceiveLen(NodeId nodeId, Uint32 count, Uint64 bytes) = 0;

  /**
   * Transporter code calls this method when a connection to a node has been
   * established (state becomes CONNECTED).
   *
   * This is called from TransporterRegistry::update_connections(), which only
   * runs from the receive thread.
   */
  virtual void reportConnect(NodeId nodeId) = 0;

  /**
   * Transporter code calls this method when a connection to a node is lost
   * (state becomes DISCONNECTED).
   *
   * This is called from TransporterRegistry::update_connections(), which only
   * runs from the receive thread.
   */
  virtual void reportDisconnect(NodeId nodeId, Uint32 errNo) = 0;

  /**
   * Called by transporter code to report error
   *
   * This is called from TransporterRegistry::update_connections(), which only
   * runs from the receive thread.
   */
  virtual void reportError(NodeId nodeId, TransporterError errorCode,
                           const char *info = 0) = 0;

  /**
   * Called from transporter code after a successful receive from a node.
   *
   * Used for heartbeat detection by upper layer.
   */
  virtual void transporter_recv_from(NodeId node) = 0;


  /**
   * Locking (no-op in single-threaded VM).
   *
   * These are used to lock/unlock the transporter for connect and disconnect
   * operation.
   *
   * Upper layer must implement these so that between return of
   * lock_transporter() and call of unlock_transporter(), no thread will be
   * running simultaneously in performSend() (for that node) or
   * performReceive().
   *
   * See src/common/transporter/trp.txt for more information.
   */
  virtual void lock_transporter(NodeId node) { }
  virtual void unlock_transporter(NodeId node) { }

  /**
   * ToDo: In current patch, these are not used, instead we use default
   * implementations in TransporterRegistry.
   */

  /**
   * Ask upper layer to supply a list of struct iovec's with data to
   * send to a node.
   *
   * The call should fill in data from all threads (if any).
   *
   * The call will write at most MAX iovec structures starting at DST.
   *
   * Returns number of entries filled-in on success, -1 on error.
   *
   * Will be called from the thread that does performSend(), so multi-threaded
   * use cases must be prepared for that and do any necessary locking.
   */
  virtual Uint32 get_bytes_to_send_iovec(NodeId, struct iovec *dst, Uint32) = 0;

  /**
   * Called when data has been sent, allowing to free / reuse the space. Passes
   * number of bytes sent.
   *
   * Note that this may be less than the sum of all iovec::iov_len supplied
   * (in case of partial send). In particular, one iovec entry may have been
   * partially sent, and may not be freed until another call to bytes_sent()
   * which covers the rest of its data.
   *
   * Returns total amount of unsent data in send buffers for this node.
   *
   * Like get_bytes_to_send_iovec(), this is called during performSend().
   */
  virtual Uint32 bytes_sent(NodeId node, Uint32 bytes) = 0;

  /**
   * Called to check if any data is available for sending with doSend().
   *
   * Like get_bytes_to_send_iovec(), this is called during performSend().
   */
  virtual bool has_data_to_send(NodeId node) = 0;

  /**
   * Called to completely empty the send buffer for a node (ie. disconnect).
   *
   * Can be called to check that no one has written to the sendbuffer
   * since it was reset last time by using the "should_be_emtpy" flag
   */
  virtual void reset_send_buffer(NodeId node, bool should_be_empty=false) = 0;

  virtual ~TransporterCallback() { };
};


/**
 * This interface implements send buffer access for the
 * TransporterRegistry::prepareSend() method.
 *
 * It is used to allocate send buffer space for signals to send, and can be
 * used to do per-thread buffer allocation.
 *
 * Reading and freeing data is done from the TransporterCallback class,
 * methods get_bytes_to_send_iovec() and bytes_send_iovec().
 */
class TransporterSendBufferHandle {
public:
  /**
   * Get space for packing a signal into, allocate more buffer as needed.
   *
   * The max_use parameter is a limit on the amount of unsent data (whether
   * delivered through get_bytes_to_send_iovec() or not) for one node; the
   * method must return NULL rather than allow to exceed this amount.
   */
  virtual Uint32 *getWritePtr(NodeId node, Uint32 lenBytes, Uint32 prio,
                              Uint32 max_use) = 0;
  /**
   * Called when new signal is packed.
   *
   * Returns number of bytes in buffer not yet sent (this includes data that
   * was made available to send with get_bytes_to_send_iovec(), but has not
   * yet been marked as really sent from bytes_sent()).
   */
  virtual Uint32 updateWritePtr(NodeId node, Uint32 lenBytes, Uint32 prio) = 0;

  /**
   * Called during prepareSend() if send buffer gets full, to do an emergency
   * send to the remote node with the hope of freeing up send buffer for the
   * signal to be queued.
   */
  virtual bool forceSend(NodeId node) = 0;

  virtual ~TransporterSendBufferHandle() { };
};

#endif
