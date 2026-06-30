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
 * @file GNUnetMessenger/app/src/main/java/org/gnunet/gnunetmessenger/ui/BaseProfileFragment.kt
 */

package org.gnunet.gnunetmessenger.ui

import android.os.Bundle
import android.view.View
import android.widget.Button
import android.widget.EditText
import android.widget.ImageView
import android.widget.TextView
import androidx.fragment.app.Fragment
import org.gnunet.gnunetmessenger.R

abstract class BaseProfileFragment : Fragment() {

    protected lateinit var avatarImage: ImageView
    protected lateinit var nameText: TextView
    protected lateinit var nameEdit: EditText
    protected lateinit var saveNameButton: Button
    protected lateinit var publicKeyText: TextView
    protected lateinit var qrCodeImage: ImageView
    //protected lateinit var listAttributesButton: Button
    protected lateinit var shareAttributesButton: Button
    protected lateinit var shareIdentityButton: Button
    protected lateinit var blockContactButton: Button

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        avatarImage = view.findViewById(R.id.avatarImage)
        nameText = view.findViewById(R.id.nameText)
        nameEdit = view.findViewById(R.id.nameEdit)
        saveNameButton = view.findViewById(R.id.btn_save_name)
        publicKeyText = view.findViewById(R.id.publicKeyText)
        qrCodeImage = view.findViewById(R.id.qrCodeImage)
        //listAttributesButton = view.findViewById(R.id.btn_list_attributes)
        shareAttributesButton = view.findViewById(R.id.btn_share_attributes)
        shareIdentityButton = view.findViewById(R.id.btn_share_identity)
        blockContactButton = view.findViewById(R.id.btn_block_contact)

        setupUI()
    }

    abstract fun setupUI()
}