/*
   This file is part of GNUnet.
   Copyright (C) 2021--2025 GNUnet e.V.

   GNUnet is free software: you can redistribute it and/or modify it
   under the terms of the GNU Affero General Public License as published
   by the Free Software Foundation, either version 3 of the License,
   or (at your option) any later version.

   GNUnet is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   SPDX-License-Identifier: AGPL3.0-or-later
 */
/*
 * @author t3sserakt
 * @file GNUnetMessenger/app/src/main/java/org/gnunet/gnunetmessenger/ui/account/AccountAdapter.kt
 */

package org.gnunet.gnunetmessenger.ui.account

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ArrayAdapter
import android.widget.Button
import android.widget.ProgressBar
import android.widget.Spinner
import androidx.fragment.app.Fragment
import androidx.navigation.fragment.findNavController
import org.gnunet.gnunetmessenger.MainActivity
import org.gnunet.gnunetmessenger.R

class LobbyCreateFragment : Fragment() {

    private val lifetimes = listOf("Off", "4 weeks", "1 week", "1 day", "8 hours", "5 minutes", "30 seconds")

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View {
        val view = inflater.inflate(R.layout.fragment_lobby_create, container, false)

        val spinner = view.findViewById<Spinner>(R.id.lifetime_spinner)
        val adapter = ArrayAdapter(requireContext(), android.R.layout.simple_spinner_item, lifetimes)
        val activity = activity as MainActivity
        val gnunetChat = activity.getGnunetChatInstance()
        val handle = activity.getChatHandle()

        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
        spinner.adapter = adapter

        view.findViewById<Button>(R.id.btn_cancel).setOnClickListener {
            findNavController().popBackStack()
        }

        view.findViewById<Button>(R.id.btn_generate).setOnClickListener {
            val selectedLifetime = spinner.selectedItem.toString()
            val progressBar = view.findViewById<ProgressBar>(R.id.progress_bar)
            progressBar.visibility = View.VISIBLE
            gnunetChat.lobbyOpen(handle){ lobbyPubKey ->
                progressBar.post {
                    progressBar.visibility = View.GONE
                    val action =
                        LobbyCreateFragmentDirections.actionLobbyCreateFragmentToLobbyDisplayFragment(
                            lobbyId = lobbyPubKey,
                            lifetime = selectedLifetime
                        )
                    findNavController().navigate(action)
                }
            }

        }

        return view
    }
}
