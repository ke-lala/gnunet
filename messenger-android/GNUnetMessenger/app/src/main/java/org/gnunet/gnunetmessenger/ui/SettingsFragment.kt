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
 * @file GNUnetMessenger/app/src/main/java/org/gnunet/gnunetmessenger/ui/SettingsFragment.kt
 */

package org.gnunet.gnunetmessenger.ui

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.CompoundButton
import androidx.fragment.app.Fragment
import androidx.fragment.app.activityViewModels
import androidx.recyclerview.widget.LinearLayoutManager
import org.gnunet.gnunetmessenger.MainActivity
import org.gnunet.gnunetmessenger.databinding.FragmentSettingsBinding
import org.gnunet.gnunetmessenger.model.ChatContact
import org.gnunet.gnunetmessenger.model.ChatSummary
import org.gnunet.gnunetmessenger.service.GnunetChat
import org.gnunet.gnunetmessenger.ui.adapters.BlockedContactAdapter
import org.gnunet.gnunetmessenger.viewmodel.ChatOverviewViewModel

class SettingsFragment : Fragment() {

    private var _binding: FragmentSettingsBinding? = null
    private val binding get() = _binding!!

    private lateinit var gnunetChat: GnunetChat
    private lateinit var adapter: BlockedContactAdapter

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = FragmentSettingsBinding.inflate(inflater, container, false)
        gnunetChat = (requireActivity() as MainActivity).getGnunetChatInstance()
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        val prefs = requireContext().getSharedPreferences("settings", 0)
        val viewModel: ChatOverviewViewModel by activityViewModels()

        binding.switchNotifications.isChecked = prefs.getBoolean("notifications", true)
        binding.switchReceipts.isChecked = prefs.getBoolean("receipts", true)

        binding.switchNotifications.setOnCheckedChangeListener { _: CompoundButton, isChecked: Boolean ->
            prefs.edit().putBoolean("notifications", isChecked).apply()
        }

        binding.switchReceipts.setOnCheckedChangeListener { _: CompoundButton, isChecked: Boolean ->
            prefs.edit().putBoolean("receipts", isChecked).apply()
        }

        val blockedContacts = getBlockedContacts(viewModel) // returns List<ChatContact>
        adapter = BlockedContactAdapter(blockedContacts)

        binding.blockedContactsList.layoutManager = LinearLayoutManager(requireContext())
        binding.blockedContactsList.adapter = adapter
    }

    private fun getBlockedContacts(viewModel: ChatOverviewViewModel): List<ChatContact> {
        val blockedContacts = mutableListOf<ChatContact>()
        viewModel.chats.value?.forEach { chatSummary: ChatSummary ->
            if (!chatSummary.chatContext.isGroup &&
                !chatSummary.chatContext.isPlatform &&
                chatSummary.contact!!.blocked)
                blockedContacts.add(chatSummary.contact)
        }

        return blockedContacts
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}