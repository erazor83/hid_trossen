hid_trossen
===========

a simple service which listens on a hid device and generates motion vectors for Trossen Bots like the [Phantom AX](http://www.trossenrobotics.com/phantomx-ax-hexapod.aspx).

Requirements
======================
The service connects to a dynamixel_zmq instance via ZeroMQ/MessagePack and sends "Commander" requests. So you'll need 
[erazor83/dynamixel_zmq](https://github.com/erazor83/dynamixel_zmq) with Trossen-Commander enabled (WITH_TROSSEN).


...a typical session
======================
<pre>
Aria25 ~ # dynamixel_zmq --speed 38400 --port /dev/ttyS3 &

Aria25 ~ # /etc/init.d/bluetooth start

Aria25 ~ # sixad -s &
sixad-bin[1259]: started
sixad-bin[1259]: sixad started, press the PS button now
sixad-sixaxis[1263]: started
sixad-sixaxis[1263]: Connected 'PLAYSTATION(R)3 Controller (34:C7:31:AE:3A:FE)' [Battery 04]

Aria25 src # ./hid_trossen --uri tcp://localhost:5555 --dev /dev/input/js0 
</pre>

install
-------------------------

... on gentoo
------------
You'll need:
  * dev-libs/boost
  * net-libs/zeromq
  * net-libs/cppzmq
  * dev-libs/msgpack

Normally you should already have these since dynamixel_zmq requires them too.

<pre>
git clone https://github.com/erazor83/hid_trossen
cd hid_trossen
cmake .
make
</pre>


license
-------------------------
GNU General Public License, version 2
