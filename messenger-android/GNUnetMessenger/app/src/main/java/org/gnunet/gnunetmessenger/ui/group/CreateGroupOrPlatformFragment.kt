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
 * @file GNUnetMessenger/app/src/main/java/org/gnunet/gnunetmessenger/ui/group/CreateGroupOrPlatformFragment.kt
 */

package org.gnunet.gnunetmessenger.ui.group

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import android.widget.EditText
import android.widget.Toast
import androidx.fragment.app.Fragment
import androidx.navigation.fragment.findNavController
import androidx.navigation.fragment.navArgs
import org.gnunet.gnunetmessenger.MainActivity
import org.gnunet.gnunetmessenger.R
import org.gnunet.gnunetmessenger.service.GnunetChat

class CreateGroupOrPlatformFragment : Fragment() {

    private val args: CreateGroupOrPlatformFragmentArgs by navArgs()
    private lateinit var gnunetChat: GnunetChat
    private lateinit var mainActivity: MainActivity
    private lateinit var nameEdit: EditText



    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View {
        return inflater.inflate(R.layout.fragment_create_group_or_platform, container, false)
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        mainActivity = activity as MainActivity
        gnunetChat = mainActivity.getGnunetChatInstance()



        nameEdit = view.findViewById(R.id.editName)
        val continueBtn = view.findViewById<Button>(R.id.btn_continue)

        continueBtn.setOnClickListener {
            val name = nameEdit.text.toString()
            if (name.isBlank()) {
                Toast.makeText(requireContext(), "Name must not be empty", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }

            if (args.isGroup) {
                val action = CreateGroupOrPlatformFragmentDirections
                    .actionCreateGroupOrPlatformFragmentToSelectGroupMembersFragment(groupName = name)
                findNavController().navigate(action)
            } else {
                processPlatform(name)
            }
        }
    }

    private fun processPlatform(name: String) {
        val handle = mainActivity.getChatHandle()
        gnunetChat.createGroup(handle, name)

        Toast.makeText(requireContext(), "Platform created", Toast.LENGTH_SHORT).show()
        findNavController().popBackStack()
    }
}