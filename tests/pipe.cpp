
/* Copyright (c) 2006-2012, Stefan Eilemann <eile@equalizergraphics.com>
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

#include <lunchbox/test.h>

#include <co/buffer.h>
#include <co/init.h>
#include <lunchbox/thread.h>

#include <iostream>

#include <co/pipeConnection.h> // private header

class Server : public lunchbox::Thread
{
public:
    void launch(co::ConnectionPtr connection)
    {
        _connection = connection;
        lunchbox::Thread::start();
    }

protected:
    virtual void run()
    {
        TEST(_connection.isValid());
        TEST(_connection->getState() == co::Connection::STATE_CONNECTED);

        co::Buffer buffer;
        _connection->recvNB(&buffer, 5);

        co::BufferPtr syncBuffer;
        TEST(_connection->recvSync(syncBuffer));
        TEST(syncBuffer == &buffer);
        TEST(strcmp("buh!", (char *)buffer.getData()) == 0);

        _connection->close();
        _connection = 0;
    }

private:
    co::ConnectionPtr _connection;
};

int main(int argc, char **argv)
{
    co::init(argc, argv);
    co::PipeConnectionPtr connection = new co::PipeConnection;
    TEST(connection->connect());

    Server server;
    server.launch(connection->acceptSync());

    const char message[] = "buh!";
    const size_t nChars = strlen(message) + 1;

    TEST(connection->send(message, nChars));

    server.join();

    connection->close();
    connection = 0;

    co::exit();
    return EXIT_SUCCESS;
}
