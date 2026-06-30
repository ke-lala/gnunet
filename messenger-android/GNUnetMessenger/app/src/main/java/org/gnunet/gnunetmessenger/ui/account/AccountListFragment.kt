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
 * @file GNUnetMessenger/app/src/main/java/org/gnunet/gnunetmessenger/ui/account/AccountListFragment.kt
 */

package org.gnunet.gnunetmessenger.ui.account

import android.os.Bundle
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import android.widget.ProgressBar
import android.widget.TextView
import androidx.core.view.isGone
import androidx.core.view.isVisible
import androidx.fragment.app.Fragment
import androidx.lifecycle.lifecycleScope
import androidx.navigation.fragment.findNavController
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import kotlinx.coroutines.Job
import kotlinx.coroutines.launch
import org.gnunet.gnunetmessenger.MainActivity
import org.gnunet.gnunetmessenger.R
import org.gnunet.gnunetmessenger.model.ChatAccount
import org.gnunet.gnunetmessenger.ui.adapters.AccountAdapter

class AccountListFragment : Fragment() {

    private lateinit var recycler: RecyclerView
    private lateinit var createButton: Button
    private lateinit var loadingIndicator: ProgressBar
    private lateinit var statusText: TextView
    private lateinit var adapter: AccountAdapter

    private val accounts = mutableListOf<ChatAccount>()
    private var refreshCollectorJob: Job? = null

    companion object {
        private const val TAG = "AccountListFragment"
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        val view = inflater.inflate(R.layout.fragment_account_list, container, false)
        val activity = requireActivity() as MainActivity

        recycler = view.findViewById(R.id.account_recycler)
        createButton = view.findViewById(R.id.btn_create_account)
        loadingIndicator = view.findViewById(R.id.account_loading_indicator)
        statusText = view.findViewById(R.id.account_status_text)

        adapter = AccountAdapter { selectedAccount ->
            viewLifecycleOwner.lifecycleScope.launch {
                try {
                    // Multi-handle: spawn-or-reuse a per-account session.
                    // Old sessions stay live in the background so a lobby
                    // host survives the switch and can complete the pairing
                    // handshake when the joiner arrives.
                    activity.switchToSession(selectedAccount)
                } catch (t: Throwable) {
                    Log.e(TAG, "switchToSession failed", t)
                    showError(getString(R.string.account_connect_failed))
                    return@launch
                }

                val action =
                    AccountListFragmentDirections.actionAccountListFragmentToAccountOverviewFragment(
                        account = selectedAccount
                    )
                findNavController().navigate(action)
            }
        }

        recycler.layoutManager = LinearLayoutManager(context)
        recycler.adapter = adapter

        createButton.setOnClickListener {
            val action =
                AccountListFragmentDirections.actionAccountListFragmentToCreateAccountFragment()
            findNavController().navigate(action)
        }

        showLoading(getString(R.string.connecting_to_gnunet))

        return view
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        val activity = requireActivity() as MainActivity

        viewLifecycleOwner.lifecycleScope.launch {
            try {
                val handle = activity.awaitInitialDataReady()
                Log.d(TAG, "Initial refresh received, loading accounts for handle=${handle.pointer}")

                createButton.isEnabled = true
                reloadAccounts(showLoading = true)

                refreshCollectorJob?.cancel()
                refreshCollectorJob = viewLifecycleOwner.lifecycleScope.launch {
                    activity.accountRefreshFlow().collect {
                        Log.d(TAG, "Account refresh event received")
                        reloadAccounts(showLoading = false)
                    }
                }
            } catch (t: Throwable) {
                Log.e(TAG, "Failed to initialize account list", t)
                showError(getString(R.string.gnunet_connection_failed))
            }
        }
    }

    override fun onDestroyView() {
        refreshCollectorJob?.cancel()
        refreshCollectorJob = null
        super.onDestroyView()
    }

    private suspend fun reloadAccounts(showLoading: Boolean) {
        val activity = requireActivity() as MainActivity
        val gnunetChat = activity.getGnunetChatInstance()
        val handle = activity.getChatHandle()

        if (showLoading) {
            showLoading(getString(R.string.loading_accounts))
        }

        try {
            Log.d(TAG, "listAccounts(): handle=${handle.pointer}")
            val refreshedAccounts = gnunetChat.listAccounts(handle).map { account ->
                account.key = gnunetChat.getProfileKey(handle)
                account
            }

            accounts.clear()
            accounts.addAll(refreshedAccounts)
            adapter.submitList(accounts.toList())

            loadingIndicator.isGone = true
            createButton.isEnabled = true

            if (accounts.isEmpty()) {
                recycler.isGone = true
                statusText.isVisible = true
                statusText.text = getString(R.string.no_accounts_available)
            } else {
                statusText.isGone = true
                recycler.isVisible = true
            }
        } catch (t: Throwable) {
            Log.e(TAG, "reloadAccounts failed", t)
            showError(getString(R.string.account_list_load_failed))
        }
    }

    private fun showLoading(message: String) {
        loadingIndicator.isVisible = true
        statusText.isVisible = true
        statusText.text = message
        recycler.isGone = true
        createButton.isEnabled = false
    }

    private fun showError(message: String) {
        loadingIndicator.isGone = true
        statusText.isVisible = true
        statusText.text = message
        recycler.isGone = true
        createButton.isEnabled = false
    }
}