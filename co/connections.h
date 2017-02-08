
/* Copyright (c) 2011-2017, Stefan Eilemann <eile@eyescale.ch>
 *
 * This file is part of Collage <https://github.com/Eyescale/Collage>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 2.1 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef CO_CONNECTIONS_H
#define CO_CONNECTIONS_H

#include <co/connection.h>
#include <co/node.h>
#include <co/types.h>
#include <lunchbox/hash.h>

#include <set>

namespace co
{
/** @internal
 * Collect all connections of a set of nodes.
 *
 * Gives priority to multicast connections if a multicast connection is used
 * more than once. Connections are added to the result vector. Multicast
 * connections are added at most once. The result vector should be empty on
 * entry. The order of connections may not match the order of nodes.
 *
 * @param nodes the nodes to send to.
 * @param result the connection vector receiving new connections.
 */
inline Connections gatherConnections(const Nodes& nodes)
{
    Connections result;
    typedef std::unordered_map<
        ConstConnectionDescriptionPtr, NodePtr,
        lunchbox::hashRefPtr<const ConnectionDescription> >
        MCNodes;
    MCNodes mcNodes; // first node using a multicast connection

    typedef std::set<ConstConnectionDescriptionPtr> MCSet;
    MCSet mcSet; // multicast connection is added

    for (Nodes::const_iterator i = nodes.begin(); i != nodes.end(); ++i)
    {
        NodePtr node = *i;
        ConnectionPtr connection = node->getConnection(true /* preferMC */);
        LBASSERT(connection);
        if (!connection)
            continue;

        if (connection->isMulticast())
        {
            ConstConnectionDescriptionPtr desc = connection->getDescription();
            if (mcSet.find(desc) != mcSet.end()) // already added
                continue;

            MCNodes::iterator j = mcNodes.find(desc);
            if (j == mcNodes.end())
            {
                // first appearance of multicast connection
                mcNodes[desc] = node;
                continue;
            }

            mcSet.insert(desc); // mark as added
            mcNodes.erase(j);
        }

        result.push_back(connection);
    }

    // Add unicast connections for multicast node connections seen only once
    for (MCNodes::iterator i = mcNodes.begin(); i != mcNodes.end(); ++i)
    {
        ConnectionPtr connection = i->second->getConnection();
        LBASSERT(connection.isValid());

        if (connection.isValid())
            result.push_back(connection);
    }
    return result;
}
}

#endif // CO_CONNECTIONS_H
