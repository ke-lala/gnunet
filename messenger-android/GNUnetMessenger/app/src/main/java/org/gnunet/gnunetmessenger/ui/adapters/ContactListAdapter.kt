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
 * @file GNUnetMessenger/app/src/main/java/org/gnunet/gnunetmessenger/ui/adapters/ContactListAdapter.kt
 */


package org.gnunet.gnunetmessenger.ui.adapters

import android.graphics.Color
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.recyclerview.widget.RecyclerView
import org.gnunet.gnunetmessenger.R
import org.gnunet.gnunetmessenger.model.ChatContact

class ContactListAdapter(
    private val onClick: ((ChatContact) -> Unit)? = null,
    private val multiSelect: Boolean = true
) : RecyclerView.Adapter<ContactListAdapter.ViewHolder>() {

    private val contacts = mutableListOf<ChatContact>()
    private val selectedContacts = mutableSetOf<ChatContact>()

    fun submitList(list: List<ChatContact>) {
        contacts.clear()
        contacts.addAll(list)
        selectedContacts.clear()
        notifyDataSetChanged()
    }

    fun getSelectedContacts(): List<ChatContact> = selectedContacts.toList()

    inner class ViewHolder(view: View) : RecyclerView.ViewHolder(view) {
        val nameText: TextView = view.findViewById(R.id.contactName)

        init {
            view.setOnClickListener {
                val contact = contacts[adapterPosition]
                toggleSelection(contact)
                notifyDataSetChanged()
                onClick?.invoke(contact)
            }
        }

        fun bind(contact: ChatContact, isSelected: Boolean) {
            nameText.text = contact.name
            itemView.setBackgroundColor(
                if (isSelected) Color.LTGRAY else Color.TRANSPARENT
            )
        }
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
        val view = LayoutInflater.from(parent.context)
            .inflate(R.layout.item_contact, parent, false)
        return ViewHolder(view)
    }

    override fun getItemCount(): Int = contacts.size

    override fun onBindViewHolder(holder: ViewHolder, position: Int) {
        val contact = contacts[position]
        holder.bind(contact, selectedContacts.contains(contact))
    }

    private fun toggleSelection(contact: ChatContact) {
        if (multiSelect) {
            if (selectedContacts.contains(contact)) {
                selectedContacts.remove(contact)
            } else {
                selectedContacts.add(contact)
            }
        } else {
            if (!selectedContacts.contains(contact)) {
                selectedContacts.clear()
                selectedContacts.add(contact)
            } else {
                selectedContacts.clear()
            }
        }
    }
}