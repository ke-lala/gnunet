package org.gnu.gnunet.ipc

import android.content.Context
import android.content.pm.PackageManager
import android.os.Binder
import android.util.Base64
import java.security.MessageDigest
import java.security.cert.CertificateFactory
import java.security.cert.X509Certificate

private val allowedPackages = setOf("org.gnunet.gnunetmessenger")

// SHA-256 der Signing-Zertifikate erlaubter Clients (Base64 der DER-Bytes)
private val ALLOWED_CERT_SHA256: Set<String> = setOf(
    // TODO: hier echten Fingerprint der Client-App(s) eintragen
)

private fun X509Certificate.sha256Base64(): String {
    val md = MessageDigest.getInstance("SHA-256")
    return Base64.encodeToString(md.digest(encoded), Base64.NO_WRAP)
}

private fun Context.packageCertsBase64(pkg: String): List<String> {
    val info = packageManager.getPackageInfo(pkg, PackageManager.GET_SIGNING_CERTIFICATES)
    val cf = CertificateFactory.getInstance("X.509")
    return info.signingInfo.apkContentsSigners.map { sig ->
        val cert = cf.generateCertificate(sig.toByteArray().inputStream()) as X509Certificate
        cert.sha256Base64()
    }
}

fun Context.enforceAllowedCaller() {
    val uid = Binder.getCallingUid()
    val pkgs = packageManager.getPackagesForUid(uid)?.toList().orEmpty()
    if (pkgs.isEmpty()) throw SecurityException("No package for uid=$uid")
    else if ((pkgs intersect allowedPackages).isEmpty()) {
        throw SecurityException("Caller not allowed. uid=$uid pkgs=$pkgs")
    }
    /*val ok = pkgs.any { pkg -> runCatching { packageCertsBase64(pkg) }.getOrNull()?.any { it in ALLOWED_CERT_SHA256 } == true }
    if (!ok) throw SecurityException("Caller not allowed. uid=$uid pkgs=$pkgs")*/
}