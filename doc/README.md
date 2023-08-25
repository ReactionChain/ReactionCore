Reaction Core
==========

This is the official reference wallet for Reaction digital currency and comprises the backbone of the Reaction peer-to-peer network. You can [download Reaction Core](https://www.reaction.network/downloads/) or [build it yourself](#building) using the guides below.

Running
---------------------
The following are some helpful notes on how to run Reaction on your native platform.

### Unix

Unpack the files into a directory and run:

- `bin/reaction-qt` (GUI) or
- `bin/reactiond` (headless)

### Windows

Unpack the files into a directory, and then run reaction-qt.exe.

### OS X

Drag Reaction-Qt to your applications folder, and then run Reaction-Qt.

### Need Help?

* See the [Reaction documentation](https://docs.reaction.network)
for help and more information.
* See the [Reaction Developer Documentation](https://reaction-docs.github.io/) 
for technical specifications and implementation details.
* Ask for help on [Reaction Nation Discord](http://reactionchat.org)
* Ask for help on the [Reaction Forum](https://reaction.network/forum)

Building
---------------------
The following are developer notes on how to build Reaction Core on your native platform. They are not complete guides, but include notes on the necessary libraries, compile flags, etc.

- [OS X Build Notes](build-osx.md)
- [Unix Build Notes](build-unix.md)
- [Windows Build Notes](build-windows.md)
- [OpenBSD Build Notes](build-openbsd.md)
- [Gitian Building Guide](gitian-building.md)

Development
---------------------
The Reaction Core repo's [root README](/README.md) contains relevant information on the development process and automated testing.

- [Developer Notes](developer-notes.md)
- [Release Notes](release-notes.md)
- [Release Process](release-process.md)
- Source Code Documentation ***TODO***
- [Translation Process](translation_process.md)
- [Translation Strings Policy](translation_strings_policy.md)
- [Travis CI](travis-ci.md)
- [Unauthenticated REST Interface](REST-interface.md)
- [Shared Libraries](shared-libraries.md)
- [BIPS](bips.md)
- [Dnsseed Policy](dnsseed-policy.md)
- [Benchmarking](benchmarking.md)

### Resources
* Discuss on the [Reaction Forum](https://reaction.network/forum), in the Development & Technical Discussion board.
* Discuss on [Reaction Nation Discord](http://reactionchat.org)

### Miscellaneous
- [Assets Attribution](assets-attribution.md)
- [Files](files.md)
- [Fuzz-testing](fuzzing.md)
- [Reduce Traffic](reduce-traffic.md)
- [Tor Support](tor.md)
- [Init Scripts (systemd/upstart/openrc)](init.md)
- [ZMQ](zmq.md)

License
---------------------
Distributed under the [MIT software license](/COPYING).
This product includes software developed by the OpenSSL Project for use in the [OpenSSL Toolkit](https://www.openssl.org/). This product includes
cryptographic software written by Eric Young ([eay@cryptsoft.com](mailto:eay@cryptsoft.com)), and UPnP software written by Thomas Bernard.
