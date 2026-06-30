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
 * @file GNUnetMessenger/app/src/main/java/org/gnunet/gnunetmessenger/ui/adapters/AccountAdapter.kt
 */

package org.gnunet.gnunetmessenger.ui.adapters

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ImageView
import android.widget.TextView
import androidx.recyclerview.widget.DiffUtil
import androidx.recyclerview.widget.ListAdapter
import androidx.recyclerview.widget.RecyclerView
import org.gnunet.gnunetmessenger.R
import org.gnunet.gnunetmessenger.model.ChatAccount
import org.gnunet.gnunetmessenger.util.AvatarStorageUtil

class AccountAdapter(
    private val onClick: (ChatAccount) -> Unit
) : ListAdapter<ChatAccount, AccountAdapter.AccountViewHolder>(DIFF) {

    companion object {
        val DIFF = object : DiffUtil.ItemCallback<ChatAccount>() {
            override fun areItemsTheSame(oldItem: ChatAccount, newItem: ChatAccount) =
                oldItem.name == newItem.name

            override fun areContentsTheSame(oldItem: ChatAccount, newItem: ChatAccount) =
                oldItem == newItem
        }
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): AccountViewHolder {
        val view = LayoutInflater.from(parent.context)
            .inflate(R.layout.item_account, parent, false)
        return AccountViewHolder(view)
    }

    override fun onBindViewHolder(holder: AccountViewHolder, position: Int) {
        holder.bind(getItem(position))
    }

    inner class AccountViewHolder(view: View) : RecyclerView.ViewHolder(view) {
        private val nameView: TextView = view.findViewById(R.id.account_name)
        private val avatarView: ImageView = view.findViewById(R.id.account_avatar)


        fun bind(account: ChatAccount) {
            nameView.text = account.name
            val context = itemView.context
            val avatarBitmap = AvatarStorageUtil.loadAvatar(context, account.key)

            if (avatarBitmap != null) {
                avatarView.setImageBitmap(avatarBitmap)
            } else {
                avatarView.setImageResource(R.drawable.ic_account_circle)
            }

            itemView.setOnClickListener { onClick(account) }
        }
    }
}
