# libgnunetchat

A client-side library for applications to utilize the Messenger service of GNUnet.

[![Linux Build](https://github.com/TheJackiMonster/libgnunetchat/actions/workflows/linux.yml/badge.svg?branch=master)](https://github.com/TheJackiMonster/libgnunetchat/actions/workflows/linux.yml)

## Features

This library is an abstraction layer using the client API from different [GNUnet](https://www.gnunet.org) services to provide the functionality of a typical messenger application. The goal is to make developing such applications easier and independent of the GUI toolkit. So people can develop different interfaces being compatible with eachother despite visual differences, a few missing features or differences in overall design.

Implementing all those typical features of a messenger application requires more than only a service to exchange messages. Therefore this library utilizes multiple different services provided by GNUnet to achieve this goal:

 - [ARM](https://docs.gnunet.org/doxygen/d4/d56/group__arm.html) to automatically start all required services without manual setup from a user
 - [FS](https://docs.gnunet.org/doxygen/d1/db9/group__fs.html) to upload, download and share files via the network in a secure way
 - [GNS](https://docs.gnunet.org/doxygen/d5/d60/group__GNS.html) to resolve published records of open lobbies, potentially exchanging contact credentials and opening a chat
 - [IDENTITY](https://docs.gnunet.org/doxygen/d0/d2f/group__identity.html) to create, delete and manage accounts as well as providing information to verify another users identity
 - [MESSENGER](https://docs.gnunet.org/doxygen/d6/d08/group__messenger.html) to open, close and manage any kind of chats as well as exchanging messages in a decentralized and secure way with other users
 - [NAMESTORE](https://docs.gnunet.org/doxygen/d3/da4/group__namestore.html) to store contact and group chat information locally and to publish records of lobbies accessible via GNS
 - [RECLAIM](https://docs.gnunet.org/doxygen/de/dea/group__reclaim.html) to claim and issue tickets regarding contacts and share private information with selected users
 - [REGEX](https://docs.gnunet.org/doxygen/d0/d57/group__regex.html) to publish peer information allowing other peers to quickly form a public group chat around a certain topic

## Build & Installation

The following dependencies are required and need to be installed to build the library:

 - [gnunet](https://git.gnunet.org/gnunet.git/): For using GNUnet services and its datatypes

Then you can simply use [Meson](https://mesonbuild.com/) as follows:
```
meson setup build      # Configure the build files for your system
ninja -C build         # Build the library using those build files
ninja -C build install # Install the library
```

Here is a list of some useful build commands using Meson and [Ninja](https://ninja-build.org/):

 - `meson compile -C build` to just compile everything with configured parameters
 - `rm -r build` to cleanup build files in case you want to recompile
 - `meson install -C build` to install the compiled files (you might need sudo privileges)
 - `meson dist -C build` to create a tar file for distribution
 - `ninja -C build docs` to build Doxygen documentation ([Doxygen](https://www.doxygen.nl/index.html) is required to do that)
 - `meson test -C build` to test the library with automated unit tests ([Check](https://libcheck.github.io/check/) is required to do that)
 - `ninja -C build uninstall` to uninstall a previous installation (you might need sudo privileges)

If you want to change the installation location, use the `--prefix=` parameter in the initial meson command. Also you can enable optimized release builds by adding `--buildtype=release` as parameter. In case you installed GNUnet to a custom prefix which is not part of the directories pkg-config is looking at, you can adjust `PKG_CONFIG_PATH` with your selected prefix to build properly.

## Contribution

If you want to contribute to this project as well, the following options are available:

 * Contribute directly to the [source code](https://git.gnunet.org/libgnunetchat.git/) with patches to fix issues or provide new features.
 * Open issues in the [bug tracker](https://bugs.gnunet.org/bug_report_page.php) to report bugs, issues or missing features.
 * Contact the authors of the software if you need any help to contribute (testing is always an option).

The list of all previous authors can be viewed in the provided [file](AUTHORS).

## Applications

There are a few applications using this library already. So users can choose from any of them picking their favourite interface for messaging:

 * [Messenger-GTK](https://git.gnunet.org/messenger-gtk.git/): A GTK based GUI for the Messenger service of GNUnet.
 * [messenger-cli](https://git.gnunet.org/messenger-cli.git): A CLI for the Messenger service of GNUnet.
