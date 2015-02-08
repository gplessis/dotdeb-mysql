/*
   Copyright (c) 2004, 2010, Oracle and/or its affiliates. All rights reserved.

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


#ifndef CLUSTER_CONNECTION_HPP
#define CLUSTER_CONNECTION_HPP
#include <ndb_types.h>

class Ndb_cluster_connection_node_iter
{
  friend class Ndb_cluster_connection_impl;
public:
  Ndb_cluster_connection_node_iter() : scan_state(~0),
				       init_pos(0),
				       cur_pos(0) {};
private:
  unsigned char scan_state;
  unsigned char init_pos;
  unsigned char cur_pos;
};

class Ndb;

/**
 * @class Ndb_cluster_connection
 * @brief Represents a connection to a cluster of storage nodes.
 *
 * Any NDB application program should begin with the creation of a
 * single Ndb_cluster_connection object, and should make use of one
 * and only one Ndb_cluster_connection. The application connects to
 * a cluster management server when this object's connect() method is called.
 * By using the wait_until_ready() method it is possible to wait
 * for the connection to reach one or more storage nodes.
 */
class Ndb_cluster_connection {
public:
  /**
   * Create a connection to a cluster of storage nodes
   *
   * @param connectstring The connectstring for where to find the
   *                      management server
   */
  Ndb_cluster_connection(const char * connectstring = 0);

  /**
   * Create a connection to a cluster of storage nodes
   *
   * @param connectstring The connectstring for where to find the
   *                      management server
   * @param force_api_node The nodeid to use for this API node, will
   *                       override any nodeid=<nodeid> specified in
   *                       connectstring
   */
  Ndb_cluster_connection(const char * connectstring, int force_api_nodeid);

#ifndef DOXYGEN_SHOULD_SKIP_INTERNAL
  Ndb_cluster_connection(const char * connectstring,
                         Ndb_cluster_connection *main_connection);
#endif
  ~Ndb_cluster_connection();

  /**
   * Set a name on the connection, which will be reported in cluster log
   *
   * @param name
   *
   */
  void set_name(const char *name);

  /**
   * Set timeout
   *
   * Used as a timeout when talking to the management server,
   * helps limit the amount of time that we may block when connecting
   *
   * Basically just calls ndb_mgm_set_timeout(h,ms).
   *
   * The default is 30 seconds.
   *
   * @param timeout_ms millisecond timeout. As with ndb_mgm_set_timeout,
   *                   only increments of 1000 are really supported,
   *                   with not to much gaurentees about calls completing
   *                   in any hard amount of time.
   * @return 0 on success
   */
  int set_timeout(int timeout_ms);

  /**
   * Connect to a cluster management server
   *
   * @param no_retries specifies the number of retries to attempt
   *        in the event of connection failure; a negative value 
   *        will result in the attempt to connect being repeated 
   *        indefinitely
   *
   * @param retry_delay_in_seconds specifies how often retries should
   *        be performed
   *
   * @param verbose specifies if the method should print a report of its progess
   *
   * @return 0 = success, 
   *         1 = recoverable error,
   *        -1 = non-recoverable error
   */
  int connect(int no_retries=0, int retry_delay_in_seconds=1, int verbose=0);

#ifndef DOXYGEN_SHOULD_SKIP_INTERNAL
  int start_connect_thread(int (*connect_callback)(void)= 0);
#endif

  /**
   * Wait until the requested connection with one or more storage nodes is successful
   *
   * @param timeout_for_first_alive   Number of seconds to wait until
   *                                  first live node is detected
   * @param timeout_after_first_alive Number of seconds to wait after
   *                                  first live node is detected
   *
   * @return = 0 all nodes live,
   *         > 0 at least one node live,
   *         < 0 error
   */
  int wait_until_ready(int timeout_for_first_alive,
		       int timeout_after_first_alive);

  /**
   * Lock creation of ndb-objects
   *   Needed to iterate over created ndb objects
   */
  void lock_ndb_objects();

  /**
   * Unlock creation of ndb-objects
   */
  void unlock_ndb_objects();

  /**
   * Iterator of ndb-objects
   * @param p Pointer to last returned ndb-object
   *          NULL - returns first object
   * @note lock_ndb_objects should be used before using this function
   *       and unlock_ndb_objects should be used after
   */
  const Ndb* get_next_ndb_object(const Ndb* p);
  
  int get_latest_error() const;
  const char *get_latest_error_msg() const;

  /**
   * Enable/disable auto-reconnect
   * @param value 0 = false, 1 = true
   */
  void set_auto_reconnect(int value);
  int get_auto_reconnect() const;

  /**
   * Collect client statistics for all Ndb objects in this connection
   * Note that this locks the ndb objects while collecting data.
   *
   * See Ndb::ClientStatistics for suggested array size and offset
   * meanings
   * 
   * @param statsArr   Pointer to array of Uint64 values for stats
   * @param szz        Size of array
   * @return Number of stats array values written
   */
  Uint32 collect_client_stats(Uint64* statsArr, Uint32 sz);

#ifndef DOXYGEN_SHOULD_SKIP_INTERNAL
  int get_no_ready();
  const char *get_connectstring(char *buf, int buf_sz) const;
  int get_connected_port() const;
  const char *get_connected_host() const;

  void set_optimized_node_selection(int val);

  unsigned no_db_nodes();
  unsigned max_nodegroup();
  unsigned node_id();
  unsigned get_connect_count() const;
  unsigned get_min_db_version() const;

  void init_get_next_node(Ndb_cluster_connection_node_iter &iter);
  unsigned int get_next_node(Ndb_cluster_connection_node_iter &iter);
  unsigned int get_next_alive_node(Ndb_cluster_connection_node_iter &iter);
  unsigned get_active_ndb_objects() const;
  
  Uint64 *get_latest_trans_gci();
#endif

private:
  friend class Ndb;
  friend class NdbImpl;
  friend class Ndb_cluster_connection_impl;
  friend class SignalSender;
  class Ndb_cluster_connection_impl & m_impl;
  Ndb_cluster_connection(Ndb_cluster_connection_impl&);

  Ndb_cluster_connection(const Ndb_cluster_connection&); // Not impl.
  Ndb_cluster_connection& operator=(const Ndb_cluster_connection&);
};

#endif
