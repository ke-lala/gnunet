.. _GNUnet-Contributors-Handbook:

****************************
Contributing
****************************

Licenses of contributions
=========================

GNUnet is a `GNU <https://www.gnu.org/>`__ package. All code
contributions must thus be put under the `GNU Affero Public License
(AGPL) <https://www.gnu.org/licenses/agpl.html>`__. All documentation
should be put under FSF approved licenses (see
`fdl <https://www.gnu.org/copyleft/fdl.html>`__).

By submitting documentation, translations, and other content to GNUnet
you automatically grant the right to publish code under the GNU Public
License and documentation under either or both the GNU Public License or
the GNU Free Documentation License. When contributing to the GNUnet
project, GNU standards and the `GNU philosophy <https://www.gnu.org/philosophy/philosophy.html>`__ should be
adhered to.

Minor contributions
===================

Smaller contributions should be provided as patches and not in
developer branches that require git access (see below).
You may post patches on https://bugs.gnunet.org in a new or existing
(relevant) issue or on the mailing list gnunet-developers@gnu.org.
You may use `git send-email` to send patches to the mailing list.
In general, you may consult https://docs.kernel.org/process/email-clients.html
for best practices with respect to mailing patches.


Major contributions
===================

You can find the GNUnet project repositories at https://git.gnunet.org.
The following applies to all repositories, but access policies are only
enforced for the main gnunet repository.
To gain write access to the repository, you will have to sign the
Copyright Assignment (see below).
Note that all commits **MUST** be signed with your GPG key.
The server will verify that any pushed commit is signed. It does not matter
which key is used (we do not keep a list of GPG public keys).

For any changes that are not minor, you **SHOULD** create an issue at
https://bugs.gnunet.org and work in a branch that you may
push to the project's server.
This will allow us to track the motivation and execution of a change
and list it in the release notes.

See :ref:`dev-style-workflow` for details on the style and workflow.

Copyright Assignment
====================

We require a formal copyright assignment for GNUnet contributors to
GNUnet e.V.; nevertheless, we do allow pseudonymous contributions. By
signing the copyright agreement and submitting your code (or
documentation) to us, you agree to share the rights to your code with
GNUnet e.V.; GNUnet e.V. receives non-exclusive ownership rights, and in
particular is allowed to dual-license the code. You retain non-exclusive
rights to your contributions, so you can also share your contributions
freely with other projects.

GNUnet e.V. will publish all accepted contributions under the AGPLv3 or
any later version. The association may decide to publish contributions
under additional licenses (dual-licensing).

We do not intentionally remove your name from your contributions;
however, due to extensive editing it is not always trivial to attribute
contributors properly. If you find that you significantly contributed to
a file (or the project as a whole) and are not listed in the respective
authors file or section, please do let us know.

`Download Copyright Assignment here. <https://www.gnunet.org/static/pdf/copyright.pdf>`__

.. _Contributing-to-the-Reference-Manual:

Reference Manual
================

-  Keep line length below 74 characters, except for URLs. URLs break in
   the PDF output when they contain linebreaks.

-  Do not use tab characters (see chapter 2.1 texinfo manual)

-  Write texts in the third person perspective.

.. _Contributing-testcases:
