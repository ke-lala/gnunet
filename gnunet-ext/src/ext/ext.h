/*
      This file is part of GNUnet
      Copyright (C) 2012-2013 Christian Grothoff (and other contributing authors)

      GNUnet is free software; you can redistribute it and/or modify
      it under the terms of the GNU General Public License as published
      by the Free Software Foundation; either version 3, or (at your
      option) any later version.

      GNUnet is distributed in the hope that it will be useful, but
      WITHOUT ANY WARRANTY; without even the implied warranty of
      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
      General Public License for more details.

      You should have received a copy of the GNU General Public License
      along with GNUnet; see the file COPYING.  If not, write to the
      Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
      Boston, MA 02110-1301, USA.
 */
/**
 * @file ext/ext.h
 * @brief example IPC messages between EXT API and GNS service
 * @author Matthias Wachs
 */

#include "gnunet_ext_service.h"


GNUNET_NETWORK_STRUCT_BEGIN

/**
 * Message from client to GNS service to lookup records.
 */
struct GNUNET_EXT_ExampleMessage
{
  /**
   * Header including size and type in NBO
   */
  struct GNUNET_MessageHeader header;

  /* Add more fields here ... */
};

GNUNET_NETWORK_STRUCT_END
