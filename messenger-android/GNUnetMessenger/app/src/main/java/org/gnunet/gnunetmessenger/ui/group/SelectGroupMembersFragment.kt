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
 * @file GNUnetMessenger/app/src/main/java/org/gnunet/gnunetmessenger/ui/group/SelectGroupMembersFragment.kt
 */

package org.gnunet.gnunetmessenger.ui.group

import android.os.Bundle
import android.view.View
import android.widget.Button
import android.widget.TextView
import androidx.fragment.app.Fragment
import androidx.fragment.app.activityViewModels
import androidx.lifecycle.Observer
import androidx.navigation.fragment.findNavController
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import org.gnunet.gnunetmessenger.MainActivity
import org.gnunet.gnunetmessenger.R
import org.gnunet.gnunetmessenger.model.ChatContact
import org.gnunet.gnunetmessenger.model.ChatHandle
import org.gnunet.gnunetmessenger.service.GnunetChat
import org.gnunet.gnunetmessenger.ui.adapters.ContactListAdapter
import org.gnunet.gnunetmessenger.viewmodel.ContactListViewModel

class SelectGroupMembersFragment : Fragment(R.layout.fragment_select_group_members) {

    private lateinit var recyclerView: RecyclerView
    private lateinit var adapter: ContactListAdapter
    private lateinit var gnunetChat: GnunetChat
    private lateinit var handle: ChatHandle
    private lateinit var mainActivity: MainActivity
    private val viewModel: ContactListViewModel by activityViewModels()

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        mainActivity = requireActivity() as MainActivity
        gnunetChat = mainActivity.getGnunetChatInstance()
        handle = mainActivity.getChatHandle()

        recyclerView = view.findViewById(R.id.contactRecyclerView)
        recyclerView.layoutManager = LinearLayoutManager(requireContext())

        adapter = ContactListAdapter()
        recyclerView.adapter = adapter

        val hint = view.findViewById<TextView>(R.id.selectionHint)
        viewModel.contacts.observe(viewLifecycleOwner, Observer { contactList ->
            adapter.submitList(contactList)
            hint.text = if (contactList.isNullOrEmpty()) {
                "You have no contacts yet — share your identity via a lobby first. You can also create the group now and invite members later."
            } else {
                "Invite members (optional — you can also invite later from the member list)"
            }
        })

        // Refresh contacts so the list is fresh whenever this screen opens.
        mainActivity.loadChats()

        view.findViewById<Button>(R.id.btn_confirm_selection).setOnClickListener {
            val selected = adapter.getSelectedContacts()
            val groupName = arguments?.getString("groupName") ?: "New Group"

            val group = gnunetChat.createGroup(handle, groupName)
            selected.forEach { chatContact: ChatContact ->
                gnunetChat.inviteContactToGroup(group, chatContact)
            }
            mainActivity.loadChats()
            findNavController().popBackStack()
            findNavController().popBackStack()
        }
    }
}