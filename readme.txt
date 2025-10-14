.. mode: -*- rst -*-



=============================
Fork from MPS
=============================
:author: Joshua Scholar
:date: 2025-10-14

I'm working on a jit and AOT compiler for a language designed for writing computer languages with unusual features.
After a few minutes looking for an acronym that isn't already a computer language I settled on Compiler Engineering System.
I.e. The CES Computer Language.

It is going to be a high level language that has compiler primitives covering embarrassingly configurable 
components so that it can embody any computer language without having to unnecessarily delve into low level details. 

Instead lowering your program into a low level IR, you just convert it to this language - where the details of what can be a 
variable, parameter, function etc. how they can be accessed, called, specialized, visibility across threads etc. can be specified
into every excruciating detail I thought might be useful to someone sometime.

Since the purpose is to enable unusual features and unusual optimizations you CAN and may need to get to a low level 
at some point in using it to implement your language, but the idea is that only new features need any low level code 
and once they're implemented, they can be accessed through a high level language.

I'm hoping I can get other programming language nerds excited and we can implement features and libraries etc.

Note, this system will be useful general purpose language.  But since it's designed as the most comprehensive engineering tool 
it is the opposite of languages designed to control complexity in organizations.  

You have the maximum possible rope with which to hang yourself, on purpose.

Now let me be clear, writing computer languages is easy and doesn't need a system like this, as long as you don't need your
programs to be optimized. Writing an interpreter that implements any semantics is trivial.  But what fun is that?  You want
programs in your shiny new language to be competitive with programs written in anything.

Sure, every year computers get faster, and your interpreter that's 100 times slower than an optimizing compiler is still useful because your 
computer is 100 times faster than one 20 years ago... But no engineer thinks that's fun.

MPS doesn't currently support 32 bit arm, so that simplifies my targets which will be 64 bit arm and 64 bit x86.  If the project goes somewhere, then maybe there will eventually be RISC-V support. 

=============================

So what changes am I planning for MPS? 

I already have one ready to fold in.  A stack scanner that accepts nan-boxed pointers that are encoded in a somewhat different way than MPS 
is already set up for.   Given the way user space pointers work in modern operating systems, the one's complement of a 64 bit user space pointer is a 
NaN and there couldn't be an easier, faster way to mask/unmask such a pointer.  More on that later.  There's more details (I also flip the high bit for reasons) and this all about details.

Planned extensions:

Pools for objects that will only be accessed and collected from a single thread so that in the same program the overhead associated for cross thread synchronization within the GC isn't paid for objects that never leave their thread while the main GC still works across threads.

Allocators for objects that can be extended as much as needed (until you run out of memory) without ever having to be moved - accomplished by reserving space in the huge 47 bit space we have without committing pages until needed.  More than just calls to mmap or VirtualAlloc if these collections are to be scanned by the GC. 

The ability to build MPS without support for compacting, so that the check for whether a new object got invalidated by compaction before being committed can be optimized out in that case.  To be clear I will write this code so that CES can be built both ways.  Google thought that it made sense for go to have a non-compacting collector and I want you to be able to make the same choice without having to pay the cost for a feature you're not using.  

=============================

A note on what MPS is, just to have it right here at the beginning.  MPS is a mature garbage collection system.  Since its roots are old, it may not be perfectly tuned for large memory spaces, but I do know of one supercomputer project that relies on it. Clasp is a Common Lisp with an LLVM backend that's used for molecular design that uses MPS as its garbage collector. 

One advantage that MPS has over the naive collectors (or Boehm) that most young language use is that it has short pause times.  It's apparently tuned to run incrementally in short increments. 

Another advantage it has over the enterprise collectors from the JVM or .Net is that it the code you write doesn't need explicit write barriers or read barriers. That makes your code more straightforward and probably faster. When the system needs a barrier it temporarily locks a page or stops all the threads.  I know that sounds scary, but it works. 

Unlike garbage collectors for the JVM, you can send different objects to different kinds of garbage collection pools within the same program and they will interoperate. You can pick the garbage collection algorithm and settings appropriate to given object or subsystem individually.

And the system is designed to work with C and C++ programs.  Since it's a mostly exact collector, it's not seamless like the Boehm-Demers-Weiser collector.  If you want your objects collected, you have to supply routines that show the system how to scan each object.  But the stacks are scanned in a conservative collector mode. If an object is refered to from the stack or from a register, then that object is pinnned and will not be compacted.

The ability of MPS (and of the language I'm writing) to interop is important.

=============================
Memory Pool System Kit Readme
=============================

:author: Richard Brooksby
:organization: Ravenbrook Limited
:date: 2002-05-20
:revision: $Id$
:confidentiality: public


This is the Memory Pool System Kit -- a complete set of sources for
using, modifying, and adapting the MPS.  This document will give you a
very brief overview and tell you where to find more information.


Overview of the MPS
-------------------

.. IMPORTANT: If you change the paragraph below, also change
   manual/source/guide/overview.rst

The Memory Pool System (MPS) is a very general, adaptable, flexible,
reliable, and efficient memory management system.  It permits the
flexible combination of memory management techniques, supporting manual
and automatic memory management, in-line allocation, finalization,
weakness, and multiple concurrent co-operating incremental generational
garbage collections.  It also includes a library of memory pool classes
implementing specialized memory management policies.

The MPS has been in development since 1994 and deployed in successful
commercial products since 1997. Bugs are almost unknown in production.
It is under continuous development and support by `Ravenbrook
<https://www.ravenbrook.com/>`__.

The MPS is distributed under the BSD 2-clause open source license (see
`<license.txt>`_).


Getting started
---------------

The MPS Kit is a complete set of sources and documentation to enable
you to use, modify, and adapt the MPS: source code, manuals,
procedures, design documentation, and so on.  See the manual_ for an
index.  (If for some reason the manual_ isn't available, you can build
it.  See below.)

.. _manual: https://memory-pool-system.readthedocs.io/

The MPS Kit is distributed in source form.  You need to build it before
using it.  The basic case is straightforward on supported platforms
(see below)::

    cd code
    cc -O2 -c mps.c     Unix / macOS (with Xcode command line)
    cl /O2 /c mps.c     Windows (with Microsoft SDK or Visual Studio 2010)

This will produce an object file you can link with your project.  For
details of how to configure the MPS, build the manual, libraries and
tests, use IDEs, autoconf, etc. see `Building the MPS
<manual/build.txt>`__.

For an example of using the MPS, see the `Scheme interpreter
example <example/scheme/>`_.

Then, to program and integrate the MPS you'll definitely need to read
the manual_.


Supported target platforms
--------------------------

The MPS is currently supported for deployment on:

- Windows Vista or later, on IA-32 and x86-64, using Microsoft Visual
  C/C++;

- Linux 2.6 or later, on IA-32 using GCC and on x86-64 using GCC or
  Clang/LLVM;

- FreeBSD 7 or later, on IA-32 and x86-64, using GCC or Clang/LLVM;

- macOS 10.4 or later, on x86-64, using Clang/LLVM.

.. TODO: Reformat the above as a table.  RB 2023-02-01.

The MPS is highly portable and has run on many other processors and
operating systems in the past (see `Building the MPS
<manual/build.txt>`__). Most of the MPS is written in very pure
ANSI C and compiles without warnings on anything.

.. warning::

    If you are running a multi-threaded 32-bit application on 64-bit
    Windows 7 via the WOW64 emulator, then you must install this
    hotfix from Microsoft:
    `<http://support.microsoft.com/kb/2864432/en-us>`__. See
    `<http://zachsaw.blogspot.co.uk/2010/11/wow64-bug-getthreadcontext-may-return.html>`__
    for a description of the problem.


Getting help
------------

You can obtain expert professional support for the MPS from `Ravenbrook
Limited <https://www.ravenbrook.com/>`__, the developers of the MPS, who
have many years of experience in commercial memory management systems.
Write to us at mps-questions@ravenbrook.com for more information.

You might also want to join the MPS discussion mailing list.  To join,
visit http://mailman.ravenbrook.com/mailman/listinfo/mps-discussion .


Document History
----------------

==========  =====  ======================================================
2002-05-20  RB_    Original author: Richard Brooksby, Ravenbrook Limited.
2002-05-20  RB_    Created based on template from P4DTI project.
2002-06-18  NB_    Minor updates and corrections.
2002-06-18  RB_    Removed obsolete requirement for MASM.
2002-06-19  NB_    Added note on self-extracting archive
2006-01-30  RHSK_  Update from "1.100.1" to "1.106.1".
2006-03-30  RHSK_  Add section 2: What's new.
2006-04-11  RHSK_  Update from "1.106.1" to "1.106.2".
2006-04-14  RHSK_  Merge updates from version/1.106 back to master.
2006-06-29  RHSK_  Note fixed job001421, job001455.
2006-12-13  RHSK_  Release 1.107.0
2007-07-05  RHSK_  Release 1.108.0
2007-12-21  RHSK_  Release 1.108.1
2008-05-01  RHSK_  Release 1.108.2
2010-03-03  RHSK_  Release 1.109.0
2012-08-14  RB_    Updating build instructions for new platforms.
2012-09-05  RB_    Considerably reduced ready for version 1.110.  Now
                   brought to you in glorious reStructuredText.
2014-01-13  GDR_   Updated supported platforms.
2014-07-04  GDR_   Link to hotfix for WOW64 bug.
2016-03-24  RB_    Adding support for FreeBSD with Clang/LLVM.
2020-05-22  PNJ_   Changed to BSD 2-clause licence.
2023-02-02  RB_    Migrating from Ravenbrook Perforce to GitHub.
==========  =====  ======================================================

.. _PNJ: mailto:pnj@ravenbrook.com
.. _GDR: mailto:gdr@ravenbrook.com
.. _NB: mailto:nb@ravenbrook.com
.. _RB: mailto:rb@ravenbrook.com
.. _RHSK: mailto:rhsk@ravenbrook.com


Copyright and Licence
---------------------

Copyright © 2001–2020 `Ravenbrook Limited <https://www.ravenbrook.com/>`_.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the
   distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

.. end
