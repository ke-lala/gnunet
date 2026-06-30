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
 * @file GNUnetMessenger/app/src/main/java/org/gnunet/gnunetmessenger/ui/account/AttributeListFragment.kt
 */

package org.gnunet.gnunetmessenger.ui.account

import android.os.Bundle
import androidx.fragment.app.Fragment
import org.gnunet.gnunetmessenger.ui.adapters.AttributeAdapter
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.navigation.fragment.navArgs
import androidx.recyclerview.widget.LinearLayoutManager
import org.gnunet.gnunetmessenger.MainActivity
import org.gnunet.gnunetmessenger.databinding.FragmentAttributeListBinding


class AttributeListFragment : Fragment() {

    private val args: AttributeListFragmentArgs by navArgs()
    private lateinit var binding: FragmentAttributeListBinding
    private val attributes = mutableListOf<Pair<String, String>>()
    private lateinit var adapter: AttributeAdapter

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View {
        val activity = activity as MainActivity
        val gnunetChat = activity.getGnunetChatInstance()
        val handle = activity.getChatHandle()
        val contact = args.contact
        val editable = args.editable

        binding = FragmentAttributeListBinding.inflate(inflater, container, false)


        adapter = AttributeAdapter(attributes)
        if (editable)
            gnunetChat.getAttributes(handle) { key, value ->
                attributes.add(key to value)
                adapter.notifyItemInserted(attributes.size - 1)
            }
        else
            gnunetChat.getContactAttributes(contact!!) { key, value ->
                attributes.add(key to value)
                adapter.notifyItemInserted(attributes.size - 1)
            }
        binding.attributeList.layoutManager = LinearLayoutManager(requireContext())
        binding.attributeList.adapter = adapter

        if (editable) {
            binding.editKey.visibility = View.VISIBLE
            binding.editValue.visibility = View.VISIBLE
            binding.btnAddAttribute.visibility = View.VISIBLE

            binding.btnAddAttribute.setOnClickListener {
                val key = binding.editKey.text.toString()
                val value = binding.editValue.text.toString()
                if (key.isNotBlank()) {
                    attributes.add(Pair(key, value))
                    adapter.notifyItemInserted(attributes.size - 1)
                    gnunetChat.setAttribute(handle, key, value)
                    binding.editKey.text.clear()
                    binding.editValue.text.clear()
                }
            }
        } else {
            binding.editKey.visibility = View.GONE
            binding.editValue.visibility = View.GONE
            binding.btnAddAttribute.visibility = View.GONE
        }

        return binding.root
    }
}
