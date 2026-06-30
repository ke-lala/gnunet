.. _dev-style-workflow:

*******************
Style and Workflow
*******************

This document contains normative rules for writing GNUnet
code and naming conventions used throughout the project.

Coding style
============

This project follows the GNU Coding Standards.

Indentation is done with two spaces per level, never with tabs.
Specific (though incomplete) indentation rules are defined in an ``uncrustify``
configuration file (in ``contrib/uncrustify.cfg``) and are enforced by Git hooks.

C99-style struct initialisation is acceptable and generally encouraged.

As in all good C code, we care about symbol space pollution and thus use
:code:`static` to limit the scope where possible, even in the compilation
unit that contains :code:`main`.

Only one variable should be declared per line:

.. code-block:: c

   // bad
   int i,j;

   // good
   int i;
   int j;

This helps keep diffs small and forces developers to think precisely about
the type of every variable.

Note that :c:`char *` is different from :c:`const char*` and
:c:`int` is different from :c:`unsigned int` or :c:`uint32_t`.
Each variable type should be chosen with care.


While ``goto`` should generally be avoided, having a ``goto`` to the
end of a function to a block of clean up statements (free, close,
etc.) can be acceptable.

Conditions should be written with constants on the left (to avoid
accidental assignment) and with the ``true`` target being either the
``error`` case or the significantly simpler continuation. For
example:

::

      if (0 != stat ("filename,"
                     &sbuf))
      {
        error();
      }
      else
      {
        /* handle normal case here */
      }

instead of

::

      if (stat ("filename," &sbuf) == 0) {
        /* handle normal case here */
       } else {
        error();
       }

If possible, the error clause should be terminated with a ``return``
(or ``goto`` to some cleanup routine) and in this case, the ``else``
clause should be omitted:

::

      if (0 != stat ("filename",
                     &sbuf))
      {
        error();
        return;
      }
      /* handle normal case here */

This serves to avoid deep nesting. The 'constants on the left' rule
applies to all constants (including. ``GNUNET_SCHEDULER_NO_TASK``),
NULL, and enums). With the two above rules (constants on left, errors
in 'true' branch), there is only one way to write most branches
correctly.

Combined assignments and tests are allowed if they do not hinder code
clarity. For example, one can write:

::

      if (NULL == (value = lookup_function()))
      {
        error();
        return;
      }

Use ``break`` and ``continue`` wherever possible to avoid deep(er)
nesting. Thus, we would write:

::

      next = head;
      while (NULL != (pos = next))
      {
        next = pos->next;
        if (! should_free (pos))
          continue;
        GNUNET_CONTAINER_DLL_remove (head,
                                     tail,
                                     pos);
        GNUNET_free (pos);
      }

instead of

::

      next = head; while (NULL != (pos = next)) {
        next = pos->next;
        if (should_free (pos)) {
          /* unnecessary nesting! */
          GNUNET_CONTAINER_DLL_remove (head, tail, pos);
          GNUNET_free (pos);
         }
        }

We primarily use ``for`` and ``while`` loops. A ``while`` loop is
used if the method for advancing in the loop is not a straightforward
increment operation. In particular, we use:

::

      next = head;
      while (NULL != (pos = next))
      {
        next = pos->next;
        if (! should_free (pos))
          continue;
        GNUNET_CONTAINER_DLL_remove (head,
                                     tail,
                                     pos);
        GNUNET_free (pos);
      }

to free entries in a list (as the iteration changes the structure of
the list due to the free; the equivalent ``for`` loop does no longer
follow the simple ``for`` paradigm of ``for(INIT;TEST;INC)``).
However, for loops that do follow the simple ``for`` paradigm we do
use ``for``, even if it involves linked lists:

::

      /* simple iteration over a linked list */
      for (pos = head;
           NULL != pos;
           pos = pos->next)
      {
         use (pos);
      }

The first argument to all higher-order functions in GNUnet must be
declared to be of type ``void *`` and is reserved for a closure. We
do not use inner functions, as trampolines would conflict with setups
that use non-executable stacks. The first statement in a higher-order
function, which unusually should be part of the variable
declarations, should assign the ``cls`` argument to the precise
expected type. For example:

::

      int
      callback (void *cls,
                char *args)
      {
        struct Foo *foo = cls;
        int other_variables;

         /* rest of function */
      }

As shown in the example above, after the return type of a function
there should be a break. Each parameter should be on a new line.

It is good practice to write complex ``if`` expressions instead of
using deeply nested ``if`` statements. However, except for addition
and multiplication, all operators should use parens. This is fine:

::

      if ( (1 == foo) ||
           ( (0 == bar) &&
             (x != y) ) )
        return x;

However, this is not:

::

      if (1 == foo)
        return x;
      if (0 == bar && x != y)
        return x;

Note that splitting the ``if`` statement above is debatable as the
``return x`` is a very trivial statement. However, once the logic
after the branch becomes more complicated (and is still identical),
the \"or\" formulation should be used for sure.

There should be two empty lines between the end of the function and
the comments describing the following function. There should be a
single empty line after the initial variable declarations of a
function. If a function has no local variables, there should be no
initial empty line. If a long function consists of several complex
steps, those steps might be separated by an empty line (possibly
followed by a comment describing the following step). The code should
not contain empty lines in arbitrary places; if in doubt, it is
likely better to NOT have an empty line (this way, more code will fit
on the screen).

When command-line arguments become too long (and would result in some
particularly ugly ``uncrustify`` wrapping), we start all arguments on
a new line. As a result, there must never be a new line within an
argument declaration (i.e. between ``struct`` and the struct's name)
or between the type and the variable). Example:

::

      struct GNUNET_TRANSPORT_CommunicatorHandle *
      GNUNET_TRANSPORT_communicator_connect (
        const struct GNUNET_CONFIGURATION_Handle *cfg,
        const char *config_section_name,
        const char *addr_prefix,
        ...);

Note that for short function names and arguments, the first argument
does remain on the same line. Example:

::

      void
      fun (short i,
           short j);


Naming conventions
==================

Header files
------------
.. Not sure if "include" and "header" files are synonymous.


For header files, the following suffixes should be used:

============= ======================================
Suffix        Usage
============= ======================================
``_lib``      Libraries without associated processes
``_service``  Libraries using service processes
``_plugin``   Plugin definition
``_protocol`` structs used in network protocol
============= ======================================

There exist a few exceptions to these rules within the codebase:

* ``gnunet_config.h`` is automatically generated.
* ``gnunet_common.h``, which defines fundamental routines.
* ``platform.h``, a collection of portability functions and macros.
* ``gettext.h``, the internationalization configuration for gettext in GNUnet.


Binaries
--------

For binary files, the following convention should be used:

=============================== =========================================
Name format                     Usage
=============================== =========================================
``gnunet-service-xxx``          Service processes (with listen sockets)
``gnunet-daemon-xxx``           Daemon processes (without listen sockets)
``gnunet-helper-xxx[-yyy]``     SUID helper for module xxx
``gnunet-yyy``                  End-user command line tools
``libgnunet_plugin_xxx_yyy.so`` Plugin for API xxx
``libgnunetxxx.so``             Library for API xxx
=============================== =========================================

Logging
-------

The convention is to define a macro on a per-file basis to manage logging:

.. code-block:: c

   #define LOG(kind,...)
   [logging_macro] (kind, "[component_name]", __VA_ARGS__)

The table below indicates the substitutions which should be made
for ``[component_name]`` and ``[logging_macro]``.

======================== ========================================= ===================
Software category        ``[component_name]``                      ``[logging_macro]``
======================== ========================================= ===================
Services and daemons     Directory name in ``GNUNET_log_setup``    ``GNUNET_log``
Command line tools       Full name in ``GNUNET_log_setup``         ``GNUNET_log``
Service access libraries ``[directory_name]``                      ``GNUNET_log_from``
Pure libraries           Library name (without ``lib`` or ``.so``) ``GNUNET_log_from``
Plugins                  ``[directory_name]-[plugin_name]``        ``GNUNET_log_from``
======================== ========================================= ===================

.. todo:: Clear up terminology within the style guide (_lib, _service mapped to appropriate software categories)

.. todo:: Interpret and write configuration style

Symbols
-------

Exported symbols must be prefixed with ``GNUNET_[module_name]_`` and be
defined in ``[module_name].c``. There are exceptions to this rule such as
symbols defined in ``gnunet_common.h``.

Private symbols, including ``struct``\ s and macros, must not be prefixed.
In addition, they must not be exported in a way that linkers could use them
or other libraries might see them via headers. This means that they must
**never** be declared in ``src/include``, and only declared or defined in
C source files or headers under ``src/[module_name]``.


Tests
-----

In the core of GNUnet, we restrict new testcases to a small subset of
languages, in order of preference:

1. C

2. Portable Shell Scripts

3. Python (3.7 or later)

We welcome efforts to remove our existing Python 2.7 scripts to replace
them either with portable shell scripts or, at your choice, Python 3.7
or later.

If you contribute new python based testcases, we advise you to not
repeat our past misfortunes and write the tests in a standard test
framework like for example pytest.

For writing portable shell scripts, these tools are useful:

* `Shellcheck <https://github.com/koalaman/shellcheck>`__, for static
   analysis of shell scripts.
* http://www.etalabs.net/sh_tricks.html,
* ``bash``-``dash`` (``/bin/sh`` on Debian) interoperability
   * `checkbashisms <https://salsa.debian.org/debian/devscripts/blob/master/scripts/checkbashisms.pl>`__,
   * https://wiki.ubuntu.com/DashAsBinSh, and
   * https://mywiki.wooledge.org/Bashism

Test cases and performance tests should follow the naming conventions
``test_[module-under-test]_[test_description].c`` and
``perf_[module-under-test]_[test_description].c``, respectively.

In either case, if there is only a single test, ``[test_description]``
may be omitted.



Continuous integration
======================

The continuous integration buildbot can be found at
https://buildbot.gnunet.org. Repositories need to be enabled by a
buildbot admin in order to participate in the builds.

The buildbot can be configured to process scripts in your repository
root under ``.buildbot/``:

The files ``build.sh``, ``install.sh`` and ``test.sh`` are executed in
order if present. If you want a specific worker to behave differently,
you can provide a worker specific script, e.g. ``myworker_build.sh``. In
this case, the generic step will not be executed.

For the ``gnunet.git`` repository, you may use \"!tarball\" or
\"!coverity\" in your commit messages. \"!tarball\" will trigger a
``make dist`` of the gnunet source and verify that it can be compiled.
The artifact will then be published to
https://buildbot.gnunet.org/artifacts. This is a good way to create a
tarball for a release as it verifies the build on another machine.

The \"!coverity\" trigger will trigger a coverity build and submit the
results for analysis to coverity: https://scan.coverity.com/. Only
developers with accounts for the GNUnet project on coverity.com are able
to see the analysis results.


Developer access and commit messages
====================================

Major changes **SHOULD** be developed in a developer branch.
A developer branch is a branch which matches a developer-specific
prefix. As a developer with git access, you should have a git username.
If you do not know it, please ask an admin. A developer branch has the
format:

::

   dev/<username>/<branchname>

Assuming the developer with username \"jdoe\" wants to create a new
branch for an i18n fix, the branch name could be:

::

   dev/jdoe/i18n_fx

You will be able to force push to and delete branches under
your prefix. It is highly recommended to work in your own developer
branches.
Most developers only have access to developer branches anyway.
Code which conforms to the commit message guidelines and
coding style, is tested and builds may be merged to the master branch.
Preferably, you would then\...

-  (optional) \...squash your commits,

-  rebase to master and then

-  ask that your branch is merged into master.

-  (optional) Delete the branch.

In general, you may want to follow the rule \"commit often, push tidy\":
You can create smaller, succinct commits with limited meaning on the
commit messages. In the end and before you push or ask your branch to be
merged, you can then squash the commits or rename them.
You can ask the project maintainer to merge your branch through any channel,
but the gnunet-developers@gnu.org mailing list is the preferred method.

Commit messages are required to convey what changes were
made in a self-contained fashion. Commit messages such as \"fix\" or
\"cleanup\" are not acceptable. You commit message should ideally start
with the subsystem name and be followed by a succinct description of the
change. Where applicable a reference to a bug number in the bugtracker
may also be included. Example:

.. code-block:: text

   # <subsystem>: <description>. (#XXXX)
   IDENTITY: Fix wrong key construction for anonymous ECDSA identity. (Fixes #12344)

We do not maintain a separate ChangeLog file anymore as the changes are
documented in the git repositories.
If you edit files that change user-facing behaviour (e.g. header files in src/include)
you will have to add at least one line in the commit message starting with \"NEWS:\".
If there is a special reason why you deem this unnecessary, you can add the line
\"NEWS: -\".
Any \"NEWS\" lines from commit messages will on release be put into the "NEWS" file in the tarball
to inform users and packages of noteworthy changes.

.. code-block:: text

   # NEWS
   NEWS: The API for foo has changed: lorem ipsum.

Note that if you run \"./bootstrap\" hooks will be installed that add message hints
to your commit message template when you run \"git commit\".

If you need to modify a commit you can do so using:

.. code-block:: console

   $ git commit --amend

If you need to modify a number of successive commits you will have to
rebase and squash. Note that most branches are protected. This means
that you can only fix commits as long as they are not pushed. You can
only modify pushed commits in your own developer branches.




Releases
========

Packaging a release can only be done by a maintainer.

In this order do:

0. Update your local tree

1. Update the submodule ``contrib/handbook``

2. Run ``./bootstrap`` to regenerate files

3. *Optional*: Update bootstrap HELLO file and update GANA generated files using ``./scripts/gana_update.sh``

4. Update NEWS file, debian/changelog (adding an entry for the new version), update the PO files and commit

5. Tag the release (Named commit!)

6. Create and test tarballs

7. Build and publish Debian and Ubuntu packages using taler-deployment.git/packaging/ng/ (see README there)

8. Upload tarball

9. Write release announcement


**(1)** Make sure the ``contrib/handbook`` submodule
is up to date.
In the **handbook repository**, make sure you have a folder that
contains the ``prebuilt`` branch as a worktree:

.. code-block:: console

   $ git worktree add _build prebuilt

Then compile the current handbook:

.. code-block:: console

   $ sphinx-multiversion --dump-metadata . _build

Commit the changes (if any) and update the submoduce in the
gnunet repository under ``doc/handbook`` and execute:

.. code-block:: console

   $ git pull origin prebuilt

If the submodule was updated, commit the new submodule.

**(2)** You should now re-run bootstrap::

.. code-block:: console

   $ ./bootstrap

**(3)** If this is a major release, it makes sense to update the bootstrap peer

HELLO. For this, you need access to the bootstrap peer (``GKZS``) and create
the hello file. Make sure that you update the bootstrap peer first such that
any changes in the identity format are actually refrected!
It is recommended to have it expire within a year:

.. code-block:: console

   $ gnunet-hello -e -E "1 year" -b -c gnunet.conf > GKZSYTE4H3N4RMYGFYEM95RV1CDRPH8QNJNTPMEB7C4QJGGR3KEG

You can then update the file in ``data/hellos``.

**(4)** Then, in order to pre-populate the news file with news hints
from commit messages (see above), we execute

.. code-block:: console

   $ sh contrib/scripts/update_news.sh v0.22.0

``v0.22.0`` is the placeholder for the version number you want to release.
Now edit the ``NEWS`` file to include all the noteworthy changes
you want to add.

Then, update the po files:

.. code-block:: console

   $ meson setup build-release && meson compile -C build-release gnunet-update-po

Commit all changes in the source tree now, and push the changes.

**(5)** You can now tag the release. Please **always** use named tags (important for our
nightly tarball build trigger).

.. code-block:: console

   $ git tag v0.22.0 -m "GNUnet release v0.22.0"

**(6)** Then, we can re-run ``./bootstrap`` and make the tarball:

.. code-block:: console

   $ ./bootstrap && meson setup --reconfigure build-release && meson dist -C build-release --formats gztar


Meson will automatically run the tests.

**(7)** To distribute the tarballs create a release triplet as defined in the GNU guidelines and
upload them to the FTP (https://www.gnu.org/prep//maintain/html_node/Automated-FTP-Uploads.html).

The file ``gnunet-0.22.0.tar.gz`` will be found inside ``build/meson-dist``.
The file must be signed by a maintainer and the signature and tarball uploaded to
https://buildbot.gnunet.org/releases/.

**(8)** After the tarballs are uploaded, we need to write the announcement.
For minor releases, we only write a brief statement:

.. code-block:: text

  See also: https://www.gnunet.org/en/news/2024-06-0.21.2.html

  This is a bugfix release for gnunet 0.21.1. It primarily addresses some
  connectivity issues introduced with our new transport subsystem.

  Links

    Source: https://ftpmirror.gnu.org/gnunet/gnunet-0.21.2.tar.gz (https://ftpmirror.gnu.org/gnunet/gnunet-0.21.2.tar.gz.sig)
    Detailed list of changes: https://git.gnunet.org/gnunet.git/log/?h=v0.21.2
    NEWS: https://git.gnunet.org/gnunet.git/tree/NEWS?h=v0.21.2
    The list of closed issues in the bug tracker: https://bugs.gnunet.org/changelog_page.php?version_id=440

  The GPG key used to sign is: 3D11063C10F98D14BD24D1470B0998EF86F59B6A

  Note that due to mirror synchronization, not all links may be
  functional early after the release. For direct access try
  https://ftp.gnu.org/gnu/gnunet/

The announcement is posted to ``gnunet-developers@gnu.org`` as well as
``help-gnunet@gnu.org``.

For major releases, we use a full release announcement.
You can find the template in the ``www.git``.
The full release announcement is sent to
``info-gnu@gnu.org`` as well.
