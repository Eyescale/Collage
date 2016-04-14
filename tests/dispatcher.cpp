
/* Copyright (c) 2012-2013, Stefan Eilemann <eile@eyescale.ch>
 *               2012, Daniel Nachbaur <danielnachbaur@gmail.com>
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

// https://github.com/Eyescale/Equalizer/issues/100
#pragma warning( disable: 4407 )

#include <lunchbox/test.h>
#include <co/buffer.h>
#include <co/bufferCache.h>
#include <co/commandFunc.h>
#include <co/dispatcher.h>
#include <co/iCommand.h>
#include <co/localNode.h>
#include <co/oCommand.h>

size_t calls = 0;

class Foo
{
public:
    Foo() : foo( 42 ) {}
    virtual ~Foo() {}
    virtual uint64_t getFoo() const { return foo; }
    const uint64_t foo;
};

class Bar : public co::LocalNode
{
public:
    Bar() : bar1( 7 ), bar2( 6 )
        {
            registerCommand( co::CMD_NODE_CUSTOM,
                             co::CommandFunc<Bar>( this, &Bar::cmd ), 0 );
        }
    virtual ~Bar() {}
    virtual uint64_t getBar1() const { return bar1; }
    const uint64_t bar1;
    const uint64_t bar2;
    virtual uint64_t getBar2() const { return bar2; }
    uint64_t getBars() const { return bar1 + bar2; }

    bool cmd( co::ICommand& )
        {
            TESTINFO( bar1 == 7, bar1 );
            TESTINFO( bar2 == 6, bar2 );
            TESTINFO( getBar1() == 7, getBar1( ));
            TESTINFO( getBar2() == 6, getBar2( ));
            TESTINFO( getBars() == 13, getBars( ));
            ++calls;
            return true;
        }
};

class FooBar : public Foo, public Bar
{
public:
    FooBar()
        {
            registerCommand( co::CMD_NODE_CUSTOM,
                             co::CommandFunc<FooBar>( this, &FooBar::cmd ), 0 );
        }

    bool cmd( co::ICommand& )
        {
            TESTINFO( foo == 42, foo );
            TESTINFO( bar1 == 7, bar1 );
            TESTINFO( bar2 == 6, bar2 );
            TESTINFO( getFoo() == 42, getFoo( ));
            TESTINFO( getBar1() == 7, getBar1( ));
            TESTINFO( getBar2() == 6, getBar2( ));
            TESTINFO( getBars() == 13, getBars( ));
            ++calls;
            return true;
        }
};

class BarFoo : public Bar, public Foo
{
public:
    BarFoo()
        {
            registerCommand( co::CMD_NODE_CUSTOM,
                             co::CommandFunc<BarFoo>( this, &BarFoo::cmd ), 0 );
        }

    bool cmd( co::ICommand& )
        {
            TESTINFO( foo == 42, foo );
            TESTINFO( bar1 == 7, bar1 );
            TESTINFO( bar2 == 6, bar2 );
            TESTINFO( getFoo() == 42, getFoo( ));
            TESTINFO( getBar1() == 7, getBar1( ));
            TESTINFO( getBar2() == 6, getBar2( ));
            TESTINFO( getBars() == 13, getBars( ));
            ++calls;
            return true;
        }
};

int main( int, char** )
{
    co::BufferCache cache( 10 );
    co::LocalNodePtr node = new co::LocalNode;

    const uint64_t size = co::OCommand::getSize();
    co::BufferPtr buffer = cache.alloc( co::COMMAND_ALLOCSIZE );
    buffer->resize( size );
    reinterpret_cast< uint64_t* >( buffer->getData( ))[ 0 ] = size;

    co::ICommand command( node, node, buffer, false );
    command.setType( co::COMMANDTYPE_NODE );
    command.setCommand( co::CMD_NODE_CUSTOM );

    Bar bar;
    FooBar fooBar;
    BarFoo barFoo;

    bar.dispatchCommand( command );
    fooBar.dispatchCommand( command );
    barFoo.dispatchCommand ( command );
    TESTINFO( calls == 3, calls );

    return EXIT_SUCCESS;
}
