/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2020 Operant Networks, Incorporated.
 * @author: Van Jacobson <info@pollere.net>
 *
 * This works is based substantially on previous work as listed below:
 *
 * Original file: daemon/table/fib-entry.hpp
 * Original repository: https://github.com/named-data/NFD
 *
 * Summary of Changes: Fix delay in communication involving ephemeral processes. https://github.com/pollere/NDNpatches
 *
 * which was originally released under the LGPL license with the following rights:
 *
 * Copyright (c) 2014-2019,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NFD_DAEMON_TABLE_FIB_ENTRY_HPP
#define NFD_DAEMON_TABLE_FIB_ENTRY_HPP

#include "fib-nexthop.hpp"

namespace nfd {

namespace name_tree {
class Entry;
} // namespace name_tree

namespace fib {

/** \class nfd::fib::NextHopList
 *  \brief Represents a collection of nexthops.
 *
 *  This type has the following member functions:
 *  - `iterator<NextHop> begin()`
 *  - `iterator<NextHop> end()`
 *  - `size_t size()`
 */
using NextHopList = std::vector<NextHop>;

/** \brief represents a FIB entry
 */
class Entry : noncopyable
{
public:
  explicit
  Entry(const Name& prefix);

  const Name&
  getPrefix() const
  {
    return m_prefix;
  }

  const NextHopList&
  getNextHops() const
  {
    return m_nextHops;
  }

  /** \return whether this Entry has any NextHop record
   */
  bool
  hasNextHops() const
  {
    return !m_nextHops.empty();
  }

  /** \return whether there is a NextHop record for \p face with the given \p endpointId
   */
  bool
  hasNextHop(const Face& face, EndpointId endpointId) const;

  /** \brief adds a NextHop record
   *
   *  If a NextHop record for \p face and \p endpointId already exists,
   *  its cost is updated.
   */
  void
  addOrUpdateNextHop(Face& face, EndpointId endpointId, uint64_t cost);

  /** \brief removes the NextHop record for \p face with the given \p endpointId
   */
  void
  removeNextHop(const Face& face, EndpointId endpointId);

  /** \brief removes all NextHop records on \p face for any \p endpointId
   */
  void
  removeNextHopByFace(const Face& face);

private:
  /** \brief forward pending interests to a newly added hop
   *
   *  All the pending interests associated with the name tree
   *  entry \p nte and its children are sent to \p newHop's
   *  \p face and \p endpointId.
   */
  void
  sendPendingInterests(const name_tree::Entry* nte, const NextHop& newHop);

  /** \note This method is non-const because mutable iterators are needed by callers.
   */
  NextHopList::iterator
  findNextHop(const Face& face, EndpointId endpointId);

  /** \brief sorts the nexthop list by cost
   */
  void
  sortNextHops();

private:
  Name m_prefix;
  NextHopList m_nextHops;

  name_tree::Entry* m_nameTreeEntry = nullptr;

  friend class name_tree::Entry;
};

} // namespace fib
} // namespace nfd

#endif // NFD_DAEMON_TABLE_FIB_ENTRY_HPP
