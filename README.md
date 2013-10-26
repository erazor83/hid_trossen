hid_trossen
===========

a simple service which listens on a hid device and generates motion vectors for Trossen Bots like the Phantom AX

Requirements
======================
The service connects to a dynamixel_zmq instance via ZeroMQ/MessagePack and sends "Commander" requests. So you'll need 
erazor83/dynamixel_zmq with Trossen-Commander enabled (WITH_TROSSEN).

install
-------------------------

... on gentoo
------------
You'll need:
  * dev-libs/boost
  * net-libs/zeromq
  * net-libs/cppzmq
  * dev-libs/msgpack

<pre>
git clone https://github.com/erazor83/hid_trossen
cd hid_trossen
cmake .
make
</pre>


license
-------------------------
GNU General Public License, version 2
