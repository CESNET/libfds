============= =============
Master branch |BuildMaster|
------------- -------------
Devel branch  |BuildDevel|
============= =============

Flow Data Storage library
=========================

The library provides components for parsing and processing IPFIX Messages.

Available components:

- IPFIX Data structure parsers
- IPFIX Data Record iterators (with Biflow support)
- IPFIX Template manager
- IPFIX Data type coverters (e.g. getters, setters, and to string)
- Manager of Information Elements (i.e. id, name, type, etc. of IPFIX fields)
- Simple XML parser with data type check (implemented as a wrapper over libxml2)
- Flow file storage (beta)

The library doesn't contain support for receiving IPFIX over network and
processing IPFIX Messages. However, all these features are provided by
`IPFIXcol2 <https://github.com/CESNET/ipfixcol2/>`_ (high-performance collector),
which is built on this library and is easily extensible by plugins.
Therefore, if you want to, for example, process or convert flow records
for your application, it's recommended to use any of available plugins or
write a new output plugin for the collector.

How to build
------------

First of all, install build dependencies of the library

**RHEL/CentOS:**

.. code-block::

    yum install gcc gcc-c++ cmake make libxml2-devel lz4-devel libzstd-devel
    # Optionally: doxygen

* Note: latest systems (e.g. Fedora) use ``dnf`` instead of ``yum``.
* Note: if ZSTD library (``libzstd-devel``) is not available, you can try to
  add official EPEL repository (``yum -y install epel-release``) and install
  it again. Alternatively, it's also possible to use embedded version
  of the library by passing additional option ``-DUSE_SYSTEM_ZSTD=off``
  to ``cmake ..`` while building.
* Note: if LZ4 library (``lz4-devel``) is not available for your distribution
  or you prefer to use libfds embedded version, you can pass additional
  option ``-DUSE_SYSTEM_LZ4=off`` to ``cmake ..``

**Debian/Ubuntu:**

.. code-block::

    apt-get install gcc g++ cmake make libxml2-dev liblz4-dev libzstd-dev
    # Optionally: doxygen pkg-config

* Note: if ZSTD library (``libzstd-dev``) or LZ4 library (``liblz4-dev``),
  is not available for your distribution or you prefer to use embedded
  versions of these libraries, you can pass additional option
  ``-DUSE_SYSTEM_LZ4=off``, resp. ``-DUSE_SYSTEM_ZSTD=off`` to ``cmake ..``
  while building.

Finally, build and install the library:

.. code-block:: bash

    $ git clone https://github.com/CESNET/libfds.git
    $ cd libfds
    $ mkdir build && cd build
    $ cmake .. -DCMAKE_INSTALL_PREFIX=/usr
    $ make
    # make install

.. |BuildMaster| image:: https://github.com/CESNET/libfds/workflows/Build%20and%20tests/badge.svg?branch=master
   :target: https://github.com/CESNET/libfds/tree/master
.. |BuildDevel| image:: https://github.com/CESNET/libfds/workflows/Build%20and%20tests/badge.svg?branch=devel
   :target: https://github.com/CESNET/libfds/tree/devel
