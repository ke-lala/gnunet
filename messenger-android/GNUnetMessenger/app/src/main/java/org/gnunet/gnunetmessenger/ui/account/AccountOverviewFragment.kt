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
 * @file GNUnetMessenger/app/src/main/java/org/gnunet/gnunetmessenger/ui/account/AccountOverviewFragment.kt
 */

package org.gnunet.gnunetmessenger.ui.account

import android.os.Bundle
import android.view.*
import androidx.drawerlayout.widget.DrawerLayout
import androidx.fragment.app.Fragment
import androidx.fragment.app.activityViewModels
import androidx.navigation.fragment.findNavController
import androidx.recyclerview.widget.LinearLayoutManager
import org.gnunet.gnunetmessenger.MainActivity
import org.gnunet.gnunetmessenger.databinding.FragmentAccountOverviewBinding
import org.gnunet.gnunetmessenger.ui.adapters.ChatListAdapter
import org.gnunet.gnunetmessenger.viewmodel.ChatOverviewViewModel

class AccountOverviewFragment : Fragment() {

    private var _binding: FragmentAccountOverviewBinding? = null
    private val binding get() = _binding!!

    private lateinit var drawerLayout: DrawerLayout
    private lateinit var adapter: ChatListAdapter

    private val viewModel: ChatOverviewViewModel by activityViewModels()

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = FragmentAccountOverviewBinding.inflate(inflater, container, false)

        val args = AccountOverviewFragmentArgs.fromBundle(requireArguments())
        (activity as MainActivity).setCurrentAccount(args.account)

        drawerLayout = binding.accountDrawerLayout

        adapter = ChatListAdapter(emptyList()) { selectedChat ->
            val action = AccountOverviewFragmentDirections
                .actionAccountOverviewFragmentToChatFragment(chatContext = selectedChat.chatContext)
            findNavController().navigate(action)
        }

        binding.chatListRecyclerView.layoutManager = LinearLayoutManager(requireContext())
        binding.chatListRecyclerView.adapter = adapter

        // Observer auf LiveData aus dem ViewModel
        viewModel.chats.observe(viewLifecycleOwner) { chatList ->
            adapter.submitList(chatList)
        }

        return binding.root
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}